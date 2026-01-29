/**
 * @file mainwindow.h
 * @brief Header file for the MainWindow class
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QMap>
#include <QGridLayout>
#include <QLabel>
#include <opencv2/opencv.hpp>
#include <QSettings>

#include "CamerasManager.h"

#include <QListWidget>
#include <QComboBox>

QT_BEGIN_NAMESPACE

namespace Ui {
	class MainWindow;
}

QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief The main window class of the application
 *
 * This class provides the main window interface for the Qt application,
 * inheriting from QMainWindow to provide the standard window functionality.
 */

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	/**
	 * @brief Constructor for MainWindow
	 * @param parent Pointer to the parent widget (default: nullptr)
	 */
	MainWindow(QWidget *parent = nullptr);

	/**
	 * @brief Destructor for MainWindow
	 */
	~MainWindow();

	void onNewFrame();

private:

	struct CameraTile {
		QWidget* container = nullptr;
		QLabel* titleLabel = nullptr;
		QLabel* imageLabel = nullptr;
	};

	void rebuildCameraGrid();
	void ensureCameraTile(int cameraId);
	void removeCameraTile(int cameraId);
	void updateCameraTitle(int cameraId);
    void setupLogFile();

private slots:
	void updateFrame();

    /**
     * @brief Triggered when the record action is activated.
     *
     * Starts video recording for all currently tracked cameras using the
     * configured output directory and recording settings.
     */
	void onRecordTriggered();

    /**
     * @brief Triggered when the settings action is activated.
     *
     * Opens or closes the settings side panel. If other side panels are currently open, they will be
     * closed before opening the settings panel.
     */
	void onSettingsTriggered();

    /**
     * @brief Triggered when the cameras action is activated.
     *
     * Opens or closes the camera side panel which displays camera-related
     * information and controls.
     */
	void onCamerasTriggered();

    /**
     * @brief Triggered when the FPS graph action is activated.
     *
     * Opens or closes the graphing side panel showing FPS-related graphs
     * and performance information.
     */
	void onFPSTriggered();

    /**
     * @brief Triggered when the temperature graph action is activated.
     *
     * Opens or closes the graphing side panel showing temperature-related
     * graphs and monitoring information.
     */
	void onTemperaturTriggered();

    void toggleSidePanel(QWidget* panel, bool& isOpen, const char* debugName);

    /**
     * @brief Triggered when the files action is activated.
     *
     * Opens a file or directory selection dialog or navigates to the
     * currently configured output directory.
     */
    void onFilesClicked();

    /**
     * @brief Adds a new camera to the CamerasManager.
     *
     * Creates a new camera instance via the CamerasManager and adds it
     * to the tracked cameras list in the UI.
     */
    void onAddCameraButtonClicked();

    /**
     * @brief Removes the currently selected tracked camera.
     *
     * Removes the selected camera from the CamerasManager and updates
     * the tracked cameras list accordingly.
     */
    void onRemoveCameraButtonClicked();

    /**
     * @brief Refreshes the tracked cameras list.
     *
     * Fetches all camera IDs from the CamerasManager and rebuilds the
     * tracked cameras list to reflect the current manager state.
     */
    void onRefreshTrackedCamerasButtonClicked();

    /**
     * @brief Handles renaming of a tracked camera item.
     *
     * Called when the user edits the display name of a camera in the
     * tracked cameras list. The camera ID remains unchanged and is
     * preserved internally.
     *
     * @param item Pointer to the modified list widget item.
     */
    void onTrackedCameraItemChanged(QListWidgetItem* item);

    void onVideoFileButtonClicked();

    void onLogFileButtonCLicked();

    void savePersistentCameras();

    void loadPersistentSettings();

    /**
     * @brief Initializes the FPS graph.
     *
     * Creates one graph per active camera in the FPS plot and assigns a
     * stable color to each camera. The x-axis represents time, the y-axis
     * represents frames per second.
     */
    void setupFpsGraph();

    /**
     * @brief Initializes the temperature graph.
     *
     * Creates one graph per active camera in the temperature plot and assigns
     * the same color used in the FPS graph to ensure visual consistency.
     * The x-axis represents time, the y-axis represents temperature in °C.
     */
    void setupTemperatureGraph();

    /**
     * @brief Periodically updates both FPS and temperature graphs.
     *
     * Fetches the current FPS and temperature values for all active cameras,
     * appends them to the internal data buffers and updates both plots.
     * A shared sliding time window is applied to both graphs.
     */
    void updateGraphs();

    /**
     * @brief Refreshes SidePanel, CamGrid and Graps
     *
     */
    void refresh();

    void rebuildCameraSidePanel();

    void ensureSeriesAlignedToTime(const QVector<int>& cameraIds);

    /**
 * @brief Returns a stable color for a given camera.
 *
 * Assigns and returns a unique, consistent color for the specified
 * camera ID. The same color is reused across all graphs (FPS and
 * temperature) to ensure visual consistency.
 *
 * Once a color has been assigned to a camera ID, it will remain
 * unchanged for the lifetime of the application.
 *
 * @param cameraId The internal camera identifier.
 * @return QColor assigned to the given camera.
 */
    QColor colorForCamera(int cameraId);

    /**
     * @brief Increases the sliding time window of the graphs.
     *
     * Expands the visible time range (in seconds) shown in both the FPS and
     * temperature plots.
     */
    void onIncreaseGraphWindowTriggered();

    /**
     * @brief Decreases the sliding time window of the graphs.
     *
     * Reduces the visible time range (in seconds) shown in both the FPS and
     * temperature plots.
     */
    void onDecreaseGraphWindowTriggered();

    void onCameraVisibilityToggled(int cameraId, bool state);

private:
	Ui::MainWindow *ui;

	CamerasManager *m_cameraManager;

    bool m_camerasPanelOpen = false;
    double m_camerasPanelWidthFactor = 0.20;   // 20% der Fensterbreite

    bool m_graphPanelOpen = false;
    double m_graphPanelWidthFactor = 0.25;     // 25%

    bool m_settings_sidepanelOpen = false;
    double m_settingsPanelWidthFactor = 0.30;  // 30%
    QString m_last_Output_dir;
    QString m_logDirectory;

    QMap<int, QString> m_cameraDisplayNames;

    bool m_isRecording = false;
    QComboBox *m_videoFormatComboBox = nullptr;

    // Graph Data
    QVector<double> m_time_data; ///< Time values (in seconds) for x-axis (shared for all cameras)

    // Per-camera y-data (same time vector is used for all)
    QMap<int, QVector<double>> m_fps_data; ///< FPS values per camera-id (left plot)
    QMap<int, QVector<double>> m_temperature_data; ///< Temperature values per camera-id (right plot)

    double m_start_time; ///< Start time for graph (in seconds since epoch)
    QTimer* m_graph_update_timer; ///< Timer to update the graph periodically

    // Sliding window (shared for FPS and Temperature)
    double m_plot_window_seconds = 30.0; ///< Max time range (seconds) shown from current time backwards

    // Stable per-camera color mapping (same color in FPS and Temperature plot)
    QMap<int, QColor> m_camera_plot_colors; ///< Color per camera-id for consistent plotting

    double m_plot_window_step_seconds = 5.0; ///< Step size for UI-controlled window changes
    double m_plot_window_min_seconds  = 2.0; ///< Lower bound for window
    double m_plot_window_max_seconds  = 120.0; ///< Upper bound for window

    double m_history_seconds = 600.0; // keep last 10 minutes in memory


	QMap<int, CameraTile> m_cameraTiles;
	int m_cameraGridColumns = 2;
};
#endif // MAINWINDOW_H
