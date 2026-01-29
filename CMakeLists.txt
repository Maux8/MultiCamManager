cmake_minimum_required(VERSION 3.16)

project(MultiCamManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)
find_package(OpenCV REQUIRED)

set(PROJECT_SOURCES
    main.cpp

    presentation/mainwindow.cpp
    presentation/mainwindow.h
    presentation/mainwindow.ui
    presentation/camerarowwidget.h
    presentation/camerarowwidget.cpp

    application/videosaver.cpp
    application/videosaver.h
    application/Camera.h
    application/Camera.cpp
    application/CamerasManager.h
    application/CamerasManager.cpp

    include/qcustomplot.cpp
    include/qcustomplot.h
)

# ---- Executable definieren ----
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MultiCamManager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
    )
else()
    add_executable(MultiCamManager
        ${PROJECT_SOURCES}
    )
endif()

# ---- CameraSimulatorLib (vorgebaute DLL) einbinden ----

add_library(CameraSimulatorLib SHARED IMPORTED)

if(WIN32)
    set_target_properties(CameraSimulatorLib PROPERTIES
        IMPORTED_LOCATION
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/CameraSimulatorLib.dll"
        IMPORTED_IMPLIB
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/CameraSimulatorLib.lib"
        INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator"
    )
elseif(APPLE)
    set_target_properties(CameraSimulatorLib PROPERTIES
        IMPORTED_LOCATION
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/libCameraSimulatorLib.dylib"
        INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator"
    )
else()
    set_target_properties(CameraSimulatorLib PROPERTIES
        IMPORTED_LOCATION
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/libCameraSimulatorLib.so"
        INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator"
    )
endif()

# ---- Sub Ordner includen ----
target_include_directories(MultiCamManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation
    ${CMAKE_CURRENT_SOURCE_DIR}/application
)


# ---- OpenCV & Simulator an MultiCamManager hängen ----

target_include_directories(MultiCamManager PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(MultiCamManager PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    ${OpenCV_LIBS}
    CameraSimulatorLib
)

# Ensure the executable can find imported dylibs beside it on macOS
if(APPLE)
    set_target_properties(MultiCamManager PROPERTIES
        BUILD_RPATH "@executable_path"
        INSTALL_RPATH "@executable_path"
        MACOSX_RPATH ON
    )
endif()

# DLL/SO nach dem Build neben die EXE kopieren
if(WIN32)
    add_custom_command(TARGET MultiCamManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/CameraSimulatorLib.dll"
            $<TARGET_FILE_DIR:MultiCamManager>
    )
elseif(APPLE)
    add_custom_command(TARGET MultiCamManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/libCameraSimulatorLib.dylib"
            $<TARGET_FILE_DIR:MultiCamManager>
    )
else()
    add_custom_command(TARGET MultiCamManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extern/CameraSimulator/libCameraSimulatorLib.so"
            $<TARGET_FILE_DIR:MultiCamManager>
    )
endif()

include(GNUInstallDirs)

install(TARGETS MultiCamManager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MultiCamManager)
endif()
