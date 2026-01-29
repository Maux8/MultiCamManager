#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "camerarowwidget.h"

#include <QMessageBox>
#include <QPixmap>
#include <QImage>
#include <QMap>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QDebug>
#include <QFileDialog>
#include <QListWidget>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QLayout>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QDateTime>
#include <limits>

#include "../include/qcustomplot.h"
#include "../application/videosaver.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
      , m_cameraManager(new CamerasManager(this))
	  , m_start_time(QDateTime::currentMSecsSinceEpoch() / 1000.0)
	  , m_graph_update_timer(new QTimer(this)) {
    ui->setupUi(this);

    // Toolbar -----
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onSettingsTriggered);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::refresh);
    connect(ui->actionRecord, &QAction::triggered, this, &MainWindow::onRecordTriggered);
    connect(ui->actionFiles, &QAction::triggered, this, &MainWindow::onFilesClicked);
    connect(ui->actionCameras, &QAction::triggered, this, &MainWindow::onCamerasTriggered);
    connect(ui->actionFPS, &QAction::triggered, this, &MainWindow::onFPSTriggered);
    connect(ui->actionTemperatur, &QAction::triggered, this, &MainWindow::onFPSTriggered);

    //SettingsPanel
    connect(ui->addCameraButton,    &QPushButton::clicked, this, &MainWindow::onAddCameraButtonClicked);
    connect(ui->removeCameraButton, &QPushButton::clicked, this, &MainWindow::onRemoveCameraButtonClicked);
    connect(ui->refreshTrackedCamerasButton, &QPushButton::clicked, this, &MainWindow::onRefreshTrackedCamerasButtonClicked);
    connect(ui->trackedCamerasList, &QListWidget::itemChanged,this, &MainWindow::onTrackedCameraItemChanged);
    connect(ui->VideoFileButton, &QPushButton::clicked, this, &MainWindow::onVideoFileButtonClicked);
    connect(ui->LogFileButton, &QPushButton::clicked, this, &MainWindow::onLogFileButtonCLicked);

    //GraphingPanel
    connect(ui->increaseWindowButton, &QPushButton::clicked,this, &MainWindow::onIncreaseGraphWindowTriggered);
    connect(ui->decreaseWindowButton, &QPushButton::clicked,this, &MainWindow::onDecreaseGraphWindowTriggered);

    connect(m_cameraManager, &CamerasManager::framesUpdated, this, &MainWindow::updateFrame);

    connect(m_cameraManager, &CamerasManager::cameraAdded, this, [this](int cameraId){
        if (!m_cameraDisplayNames.contains(cameraId)) {
            m_cameraDisplayNames[cameraId] = QString("Camera %1").arg(cameraId);
        }
        ensureCameraTile(cameraId);
    });

    connect(m_cameraManager, &CamerasManager::cameraRemoved, this, [this](int cameraId){
        removeCameraTile(cameraId);
    });

    connect(m_graph_update_timer, &QTimer::timeout, this, &MainWindow::updateGraphs);

    ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // Add video format combobox to toolbar after Record button
    m_videoFormatComboBox = new QComboBox(this);
    m_videoFormatComboBox->addItem("AVI");
    m_videoFormatComboBox->addItem("MP4");
    m_videoFormatComboBox->setToolTip("Video Format");
    // Insert after the Record action by finding its position
    QList<QAction*> actions = ui->toolBar->actions();
    int recordIndex = actions.indexOf(ui->actionRecord);
    if (recordIndex >= 0 && recordIndex + 1 < actions.size()) {
        ui->toolBar->insertWidget(actions[recordIndex + 1], m_videoFormatComboBox);
    } else {
        ui->toolBar->addWidget(m_videoFormatComboBox);
    }



    // Siepanel connections
    connect(m_cameraManager, &CamerasManager::cameraAdded,this, &MainWindow::rebuildCameraSidePanel);
    connect(m_cameraManager, &CamerasManager::cameraRemoved,this, &MainWindow::rebuildCameraSidePanel);

    // trackedCameras Edit Setup
    ui->trackedCamerasList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->trackedCamerasList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    m_graph_update_timer->start(500);


    loadPersistentSettings();
    setupLogFile();
    // Camera Manager setup ------
    //m_cameraManager->addCamera();
    m_cameraManager->setAutoUpdate(true, 33); // ~30fps
    m_cameraManager->connectAll();
    m_cameraManager->startAll();

    setupFpsGraph();
    setupTemperatureGraph();
    m_graph_update_timer->start(500);
    // FPS graph
    ui->fpsGraph->yAxis->setRange(0, 120);

    // Temperature graph
    ui->temperatureGraph->yAxis->setRange(0, 100);


    // Files output default to documents
    QSettings settings("HTWBerlin", "MultiCamManager");
    QString default_dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_last_Output_dir = settings.value("lastOutputDir", default_dir).toString();

    //close all panels
    m_graphPanelOpen = false;
    if (ui->graphingSidePanel) {
        ui->graphingSidePanel->setEnabled(false);
        ui->graphingSidePanel->setVisible(false);
    }

    m_settings_sidepanelOpen = false;
    if (ui->settingsSidePanel) {
        ui->settingsSidePanel->setEnabled(false);
        ui->settingsSidePanel->setVisible(false);
    }

    m_camerasPanelOpen = false;
    if (ui->cameraSidePanel) {
        ui->cameraSidePanel->setEnabled(false);
        ui->cameraSidePanel->setVisible(false);
    }

    // set File Location in settingspage after settings loaded
    ui->VideoFileLocation->setText("Video Location: " + m_last_Output_dir);
    ui->LogFileLocation->setText("Log Location: " + m_logDirectory);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onRecordTriggered() {
    VideoFormat format = VideoFormat::AVI;
    if (m_videoFormatComboBox && m_videoFormatComboBox->currentText() == "MP4")
        format = VideoFormat::MP4;

    if (m_isRecording) {
        m_cameraManager->stopRecording();
        m_isRecording = false;
        ui->actionRecord->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
        ui->actionRecord->setText("Record");
    } else {
        QSettings settings("HTWBerlin", "MultiCamManager");
        if (settings.value("lastOutputDir").toString().isEmpty()) {
            QString directory = QFileDialog::getExistingDirectory(
                this, "Ordner auswählen", QString(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

            if (directory.isEmpty()) return;
            settings.setValue("lastOutputDir", directory);
        }

        m_last_Output_dir = settings.value("lastOutputDir").toString();
        m_isRecording = true;

        m_cameraManager->startRecording(m_last_Output_dir, format);

        ui->actionRecord->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStop));
        ui->actionRecord->setText("Stop");
    }

    rebuildCameraSidePanel();
}

// MainWindow.cpp
void MainWindow::toggleSidePanel(QWidget* panel, bool& isOpen, const char* debugName)
{
    qDebug() << "[DEBUG] toggleSidePanel() called for" << debugName;

    if (!panel) {
        qDebug() << "[ERROR]" << debugName << "is NULL!";
        return;
    }

    // toggle state
    isOpen = !isOpen;

    // apply state: hide removes it from layout; disable prevents interaction
    panel->setEnabled(isOpen);
    panel->setVisible(isOpen);

    qDebug() << "[DEBUG]" << debugName << "now open:" << isOpen;
}

void MainWindow::onFPSTriggered()
{
    qDebug() << "[DEBUG] onFPSTriggered() called";
    toggleSidePanel(ui->graphingSidePanel, m_graphPanelOpen, "graphingSidePanel");
}

void MainWindow::onSettingsTriggered()
{
    qDebug() << "[DEBUG] Settings button triggered";
    toggleSidePanel(ui->settingsSidePanel, m_settings_sidepanelOpen, "settingsSidePanel");
}

void MainWindow::onCamerasTriggered()
{
    qDebug() << "[DEBUG] Cameras button triggered";

    // toggle first
    toggleSidePanel(ui->cameraSidePanel, m_camerasPanelOpen, "cameraSidePanel");

    // if its being opened -> rebuild it
    if (m_camerasPanelOpen) {
        rebuildCameraSidePanel();
    }
}

void MainWindow::onTemperaturTriggered() {
}

void MainWindow::onFilesClicked() {
    if (m_last_Output_dir.isEmpty()) {
        m_last_Output_dir =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(m_last_Output_dir));
}

void MainWindow::setupLogFile()
{
    QSettings settings("HTWBerlin", "MultiCamManager");
    const QString default_dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString initial_dir = settings.value("lastLogDir", default_dir).toString();
    // do not show dialog when user alraedy entered a Log File location
    if (settings.value("lastLogDir") != "") {
        m_logDirectory = settings.value("lastLogDir").toString();
        if ( m_cameraManager )
        {
            m_cameraManager->setLogDirectory( m_logDirectory );
            // Start parameter logging with 500ms interval
            m_cameraManager->startParameterLogging( m_logDirectory, 500 );
        }
        return;
    }

    QString directory = QFileDialog::getExistingDirectory(
        this,
        "Select directory for log file and camera parameters",
        initial_dir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if ( directory.isEmpty() )
    {
        directory = initial_dir;
    }

    QDir dir( directory );
    if ( !dir.exists() )
    {
        dir.mkpath( "." );
    }

    m_logDirectory = dir.absolutePath();
    settings.setValue("lastLogDir", m_logDirectory);

    if ( m_cameraManager )
    {
        m_cameraManager->setLogDirectory( m_logDirectory );
        // Start parameter logging with 500ms interval
        m_cameraManager->startParameterLogging( m_logDirectory, 500 );
    }
}

void MainWindow::loadPersistentSettings()
{
    QSettings settings("HTWBerlin", "MultiCamManager");

    // Load last Video output directory
    QString default_dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_last_Output_dir = settings.value("lastOutputDir", default_dir).toString();

    // Read names array
    const int count = settings.beginReadArray("trackedCameraNames");
    qDebug() << "[Settings] Loading" << count << "camera display names";

    // Create that many cameras anew and assign names in the same order
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);

        const QString savedName = settings.value("name", QString("Camera %1").arg(i + 1)).toString();

        const int newId = m_cameraManager->addCamera();
        m_cameraDisplayNames[newId] = savedName;

        qDebug() << "[Settings] Created camera" << newId << "with display name" << savedName;
    }

    settings.endArray();

    // Refresh UI list from manager state (will use m_cameraDisplayNames)
    onRefreshTrackedCamerasButtonClicked();
}

void MainWindow::savePersistentCameras()
{
    setupFpsGraph();
    setupTemperatureGraph();

    QSettings settings("HTWBerlin", "MultiCamManager");

    // Optional: other settings
    settings.setValue("lastOutputDir", m_last_Output_dir);

    const QVector<int> ids = m_cameraManager->getCameraIds();

    settings.beginWriteArray("trackedCameraNames");
    for (int i = 0; i < ids.size(); ++i) {
        settings.setArrayIndex(i);

        const int id = ids[i];
        const QString name = m_cameraDisplayNames.value(id, QString("Camera %1").arg(id));

        settings.setValue("name", name);
    }
    settings.endArray();

    settings.sync();

    qDebug() << "[Settings] Saved" << ids.size() << "camera display names";
}

void MainWindow::onRefreshTrackedCamerasButtonClicked()
{
    qDebug() << "[Settings] Refresh tracked cameras list";

    ui->trackedCamerasList->clear();

    const QVector<int> cameraIds = m_cameraManager->getCameraIds();

    for (int id : cameraIds) {
        // Wenn wir schon einen Namen kennen, den benutzen, sonst Standard
        QString label;
        if (m_cameraDisplayNames.contains(id)) {
            label = m_cameraDisplayNames[id];
        } else {
            label = QString("Camera %1").arg(id);
            m_cameraDisplayNames[id] = label;
        }

        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, id);
        item->setFlags(item->flags()
                       | Qt::ItemIsEditable
                       | Qt::ItemIsSelectable
                       | Qt::ItemIsEnabled);

        ui->trackedCamerasList->addItem(item);
    }

    qDebug() << "[Settings] Tracked cameras list now has"
             << ui->trackedCamerasList->count() << "entries";
}

void MainWindow::onAddCameraButtonClicked()
{
    const int cameraId = m_cameraManager->addCamera();

    // Connect and start the new camera
    if (Camera* camera = m_cameraManager->getCamera(cameraId)) {
        camera->connect();
        camera->start();
    }

    // Standardname setzen, wenn noch keiner existiert
    if (!m_cameraDisplayNames.contains(cameraId)) {
        m_cameraDisplayNames[cameraId] = QString("Camera %1").arg(cameraId);
    }

    auto *item = new QListWidgetItem(m_cameraDisplayNames[cameraId]);

    // ID im Hintergrund speichern
    item->setData(Qt::UserRole, cameraId);

    // Item editierbar machen
    item->setFlags(item->flags()
                   | Qt::ItemIsEditable
                   | Qt::ItemIsSelectable
                   | Qt::ItemIsEnabled);

    ui->trackedCamerasList->addItem(item);
    ui->trackedCamerasList->setCurrentItem(item);

    qDebug() << "[Settings] Added camera with ID" << cameraId
             << "label =" << m_cameraDisplayNames[cameraId];

    savePersistentCameras();
}

void MainWindow::onRemoveCameraButtonClicked()
{
    QListWidgetItem* item = ui->trackedCamerasList->currentItem();
    if (!item) {
        qDebug() << "[Settings] No tracked camera selected to remove";
        return;
    }

    const int cameraId = item->data(Qt::UserRole).toInt();

    qDebug() << "[Settings] Try remove camera with ID" << cameraId;

    if (m_cameraManager->removeCamera(cameraId)) {
        m_cameraDisplayNames.remove(cameraId);  // Namen vergessen
        delete item;
        qDebug() << "[Settings] Camera" << cameraId
                 << "removed from manager and list";
    } else {
        qDebug() << "[Settings] Manager could not remove camera" << cameraId;
    }

    savePersistentCameras();
}

void MainWindow::onTrackedCameraItemChanged(QListWidgetItem* item)
{
    if (!item) return;

    const int cameraId = item->data(Qt::UserRole).toInt();
    const QString newName = item->text();

    m_cameraDisplayNames[cameraId] = newName;
    updateCameraTitle(cameraId);

    qDebug() << "[Settings] Camera" << cameraId
             << "renamed to" << newName;

    savePersistentCameras();
}

void MainWindow::updateCameraTitle(int cameraId)
{
    if (!m_cameraTiles.contains(cameraId)) {
        return;
    }

    const QString name = m_cameraDisplayNames.value(cameraId, QString("Camera %1").arg(cameraId));
    if (m_cameraTiles[cameraId].titleLabel) {
        m_cameraTiles[cameraId].titleLabel->setText(name);
    }
}

void MainWindow::ensureCameraTile(int cameraId)
{
    if (!ui->cameraGridLayout) {
        qDebug() << "[ERROR] cameraGridLayout is NULL!";
        return;
    }

    if (m_cameraTiles.contains(cameraId)) {
        updateCameraTitle(cameraId);
        return;
    }

    auto *tile = new QWidget(ui->cameraGridContainer);
    auto *tileLayout = new QVBoxLayout(tile);
    tileLayout->setContentsMargins(8, 8, 8, 8);
    tileLayout->setSpacing(6);

    auto *titleLabel = new QLabel(tile);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: 600; color:#ddd; padding:4px;");

    auto *imageLabel = new QLabel(tile);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(320, 240);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel->setScaledContents(false);
    imageLabel->setStyleSheet("background:#2b2b2b; border:1px solid #555; border-radius:4px; color:#999;");
    imageLabel->setText("Waiting...");

    tileLayout->addWidget(titleLabel);
    tileLayout->addWidget(imageLabel);

    m_cameraTiles.insert(cameraId, {tile, titleLabel, imageLabel});

    updateCameraTitle(cameraId);
    rebuildCameraGrid();
}

void MainWindow::removeCameraTile(int cameraId)
{
    if (!ui->cameraGridLayout) {
        qDebug() << "[ERROR] cameraGridLayout is NULL!";
        return;
    }

    if (!m_cameraTiles.contains(cameraId)) {
        return;
    }

    const CameraTile tile = m_cameraTiles.take(cameraId);
    if (tile.container) {
        ui->cameraGridLayout->removeWidget(tile.container);
        tile.container->deleteLater();
    }

    rebuildCameraGrid();
}

void MainWindow::rebuildCameraGrid()
{
    if (!ui->cameraGridLayout) {
        qDebug() << "[ERROR] cameraGridLayout is NULL!";
        return;
    }

    while (QLayoutItem* item = ui->cameraGridLayout->takeAt(0)) {
        if (item->widget()) {
            ui->cameraGridLayout->removeWidget(item->widget());
        }
        delete item;
    }

    int index = 0;
    const QList<int> ids = m_cameraTiles.keys();
    for (int id : ids) {
        CameraTile &tile = m_cameraTiles[id];
        if (!tile.container) {
            continue;
        }
        const int row = index / m_cameraGridColumns;
        const int col = index % m_cameraGridColumns;
        ui->cameraGridLayout->addWidget(tile.container, row, col);
        ++index;
    }

    for (int col = 0; col < m_cameraGridColumns; ++col) {
        ui->cameraGridLayout->setColumnStretch(col, 1);
    }
}

void MainWindow::updateFrame() {
    const QMap<int, cv::Mat> allFrames = m_cameraManager->getAllFrames();
    const QVector<int> cameraIds = m_cameraManager->getCameraIds();

    for (int id : cameraIds) {
        ensureCameraTile(id);
        updateCameraTitle(id);
    }

    for (auto it = m_cameraTiles.begin(); it != m_cameraTiles.end(); ++it) {
        const int cameraId = it.key();
        CameraTile &tile = it.value();

        if (!tile.imageLabel) {
            continue;
        }

        const auto frameIt = allFrames.find(cameraId);
        if (frameIt != allFrames.end() && !frameIt.value().empty()) {
            cv::Mat frameRgb;
            cv::cvtColor(frameIt.value(), frameRgb, cv::COLOR_BGR2RGB);

            // NOTE: QImage uses the cv::Mat buffer here. We copy() before painting to ensure
            // the image has its own storage and is safe to modify.
            QImage img(
                frameRgb.data,
                frameRgb.cols,
                frameRgb.rows,
                static_cast<int>(frameRgb.step),
                QImage::Format_RGB888
                );
            img = img.copy();

            // --- Overlay (top-left): FPS + Temperature ---
            // Values are provided by the simulator via Camera::getParameters().
            const CameraParameters params = m_cameraManager->getCameraParameters(cameraId);
            {
                QPainter painter(&img);
                painter.setRenderHint(QPainter::Antialiasing);

                QFont font = painter.font();
                font.setPointSize(30);
                font.setBold(true);
                painter.setFont(font);

                const int margin = 8;
                const int lineH = 35;

                const QString line1 = QString("FPS: %1").arg(params.fps, 0, 'f', 1);
                const QString line2 = QString("Temp: %1 \u00B0C").arg(params.temperature, 0, 'f', 1);

                // Background box sized to content (simple, robust sizing)
                QFontMetrics fm(font);
                const int w = std::max(fm.horizontalAdvance(line1), fm.horizontalAdvance(line2)) + 16;
                const int h = (lineH * 2) + 12;
                QRect bg(margin - 4, margin - 4, w, h);

                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(0, 0, 0, 150));
                painter.drawRoundedRect(bg, 4, 4);

                painter.setPen(Qt::white);
                painter.drawText(margin, margin + lineH, line1);
                painter.drawText(margin, margin + 2 * lineH, line2);
            }

            QPixmap pix = QPixmap::fromImage(img);
            pix = pix.scaled(
                tile.imageLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
                );
            tile.imageLabel->setPixmap(pix);
            tile.imageLabel->setText(QString());
        } else {
            tile.imageLabel->setPixmap(QPixmap());
            tile.imageLabel->setText("No frame");
        }
    }
}


QColor MainWindow::colorForCamera(int cameraId)
{
    if (m_camera_plot_colors.contains(cameraId))
        return m_camera_plot_colors[cameraId];

    static const QVector<QColor> palette = {
        Qt::blue, Qt::red, Qt::green, Qt::magenta,
        Qt::cyan, Qt::darkYellow, Qt::darkBlue,
        Qt::darkRed, Qt::darkGreen
    };

    QColor color = palette[m_camera_plot_colors.size() % palette.size()];
    m_camera_plot_colors[cameraId] = color;
    return color;
}

void MainWindow::setupFpsGraph()
{
    QCustomPlot* plot = ui->fpsGraph;
    plot->clearGraphs();

    plot->xAxis->setLabel("Time (s)");
    plot->yAxis->setLabel("FPS");

    plot->legend->setVisible(true);
    plot->legend->setFont(QFont("Helvetica", 9));
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);

    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    plot->setBackground(QBrush(Qt::white));

    const QVector<int> cameraIds = m_cameraManager->getCameraIds();
    for (int camId : cameraIds) {
        plot->addGraph();
        plot->graph(plot->graphCount() - 1)->setPen(QPen(colorForCamera(camId), 2));
        plot->graph(plot->graphCount() - 1)->setName(QString("Camera %1").arg(camId));
    }
}

void MainWindow::setupTemperatureGraph()
{
    QCustomPlot* plot = ui->temperatureGraph;
    plot->clearGraphs();

    plot->xAxis->setLabel("Time (s)");
    plot->yAxis->setLabel("Temperature (°C)");

    plot->legend->setVisible(true);
    plot->legend->setFont(QFont("Helvetica", 9));
    plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);

    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    plot->setBackground(QBrush(Qt::white));

    const QVector<int> cameraIds = m_cameraManager->getCameraIds();
    for (int camId : cameraIds) {
        plot->addGraph();
        plot->graph(plot->graphCount() - 1)->setPen(QPen(colorForCamera(camId), 2));
        plot->graph(plot->graphCount() - 1)->setName(QString("Camera %1").arg(camId));
    }
}

void MainWindow::onIncreaseGraphWindowTriggered()
{
    m_plot_window_seconds = qMin(m_plot_window_seconds + 5.0, 120.0);
    qDebug() << "[Graph] Window seconds =" << m_plot_window_seconds;
    ui->slidingWindowLabel->setText(QString("Window: %1 s").arg(m_plot_window_seconds, 0, 'f', 0));
    ui->fpsGraph->xAxis->setLabel(QString("Time (s) | Window: %1 s").arg(m_plot_window_seconds, 0, 'f', 0));
    ui->temperatureGraph->xAxis->setLabel(QString("Time (s) | Window: %1 s").arg(m_plot_window_seconds, 0, 'f', 0));
}

void MainWindow::onDecreaseGraphWindowTriggered()
{
    m_plot_window_seconds = qMax(m_plot_window_seconds - 5.0, 2.0);
    qDebug() << "[Graph] Window seconds =" << m_plot_window_seconds;
    ui->slidingWindowLabel->setText(QString("Window: %1 s").arg(m_plot_window_seconds, 0, 'f', 0));
    ui->fpsGraph->xAxis->setLabel(QString("Time (s) | Window: %1 s").arg(m_plot_window_seconds, 0, 'f', 0));
    ui->temperatureGraph->xAxis->setLabel(QString("Time (s) | Window: %1 s").arg(m_plot_window_seconds, 0, 'f', 0));
}

void MainWindow::ensureSeriesAlignedToTime(const QVector<int>& cameraIds)
{
    const int n = m_time_data.size();
    const double NaN = std::numeric_limits<double>::quiet_NaN();

    for (int camId : cameraIds) {
        auto &fpsVec = m_fps_data[camId];
        auto &tmpVec = m_temperature_data[camId];

        const int oldFps = fpsVec.size();
        if (oldFps < n) {
            fpsVec.resize(n);
            for (int i = oldFps; i < n; ++i) fpsVec[i] = NaN;
        }

        const int oldTmp = tmpVec.size();
        if (oldTmp < n) {
            tmpVec.resize(n);
            for (int i = oldTmp; i < n; ++i) tmpVec[i] = NaN;
        }
    }
}

void MainWindow::rebuildCameraSidePanel()
{
    ui->cameraListWidget->clear();

    const QVector<int> ids = m_cameraManager->getCameraIds();

    for (int id : ids)
    {
        QString name = QString("ID: %1 Name: ").arg(id) + m_cameraDisplayNames.value(
            id, QString("Camera %1").arg(id));

        auto* item = new QListWidgetItem(ui->cameraListWidget);
        item->setSizeHint(QSize(0, 28));

        auto* row = new CameraRowWidget(name, id, ui->cameraListWidget);
        row->setRecordingState(m_isRecording ? CameraRowWidget::Recording::Recording : CameraRowWidget::Recording::NotRecording);
        row->setStatus(m_cameraManager->getCameraParameters(id).power_status ? CameraRowWidget::Status::Power_On : CameraRowWidget::Status::Power_Off);
        ui->cameraListWidget->addItem(item);
        ui->cameraListWidget->setItemWidget(item, row);
        connect(row, &CameraRowWidget::visibilityToggled, this, &MainWindow::onCameraVisibilityToggled);
    }
}

void MainWindow::refresh() {
    rebuildCameraSidePanel();
    rebuildCameraGrid();
    updateGraphs();
}



void MainWindow::updateGraphs()
{
    if (!ui || !ui->fpsGraph || !ui->temperatureGraph || !m_cameraManager)
        return;

    const double now = (QDateTime::currentMSecsSinceEpoch() / 1000.0) - m_start_time;

    // --- X-axis window behavior (display only) ---
    const double window = m_plot_window_seconds;     // e.g. 30s, user-adjustable
    double xLower = 0.0;
    double xUpper = window;

    if (now > window) {
        xUpper = now;
        xLower = now - window;
    }

    // --- Append time sample (shared x-axis) ---
    m_time_data.append(now);

    // --- Append per-camera samples ---
    const QVector<int> cameraIds = m_cameraManager->getCameraIds();

    // Ensure vectors exist for all cameras (and colors etc. if you use them)


    // Append the newest values (one sample per camera)
    for (int camId : cameraIds) {
        const CameraParameters params = m_cameraManager->getCameraParameters(camId);
        m_fps_data[camId].append(params.fps);
        m_temperature_data[camId].append(params.temperature);
    }

    ensureSeriesAlignedToTime(cameraIds);

    // --- Prune only by HISTORY limit, not by current window ---
    // This prevents data loss when user reduces then increases window.
    const double historyMinTime = qMax(0.0, now - m_history_seconds);

    while (!m_time_data.isEmpty() && m_time_data.first() < historyMinTime) {
        m_time_data.removeFirst();
        for (int camId : cameraIds) {
            if (!m_fps_data[camId].isEmpty())
                m_fps_data[camId].removeFirst();
            if (!m_temperature_data[camId].isEmpty())
                m_temperature_data[camId].removeFirst();
        }
    }

    // --- Update FPS plot ---
    {
        QCustomPlot* plot = ui->fpsGraph;

        // Fixed Y range
        plot->yAxis->setRange(0, 120);

        for (int i = 0; i < cameraIds.size() && i < plot->graphCount(); ++i) {
            const int camId = cameraIds[i];
            plot->graph(i)->setData(m_time_data, m_fps_data[camId], true);
        }

        plot->xAxis->setRange(xLower, xUpper);
        plot->replot();
    }

    // --- Update Temperature plot ---
    {
        QCustomPlot* plot = ui->temperatureGraph;

        // Fixed Y range
        plot->yAxis->setRange(0, 100);

        for (int i = 0; i < cameraIds.size() && i < plot->graphCount(); ++i) {
            const int camId = cameraIds[i];
            plot->graph(i)->setData(m_time_data, m_temperature_data[camId], true);
        }

        plot->xAxis->setRange(xLower, xUpper);
        plot->replot();
    }
}

void MainWindow::onCameraVisibilityToggled(int cameraId, bool state) {
    m_cameraTiles[cameraId].container->setVisible(state);
}


void MainWindow::onVideoFileButtonClicked() {
    QString directory = QFileDialog::getExistingDirectory(
        this,
        "Select directory for video files",
        m_last_Output_dir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    if (directory.isEmpty()) {
        return;
    }
    QSettings settings("HTWBerlin", "MultiCamManager");
    settings.setValue("lastOutputDir", directory);
    m_last_Output_dir = directory;
    ui->VideoFileLocation->setText("Video File: " + m_last_Output_dir);
}

void MainWindow::onLogFileButtonCLicked() {
    QString directory = QFileDialog::getExistingDirectory(
        this,
        "Select directory for log file and camera parameters",
        m_logDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    if (directory.isEmpty()) {
        return;
    }
    QSettings settings("HTWBerlin", "MultiCamManager");
    settings.setValue("lastLogDir", directory);
    m_logDirectory = directory;
    ui->LogFileLocation->setText("Log File: " + m_logDirectory);
}
