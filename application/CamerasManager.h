#ifndef CAMERASMANAGER_H
#define CAMERASMANAGER_H

#include "Camera.h"
#include "LogEntry.h"
#include "videosaver.h"
#include <QObject>
#include <QVector>
#include <QMap>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <opencv2/opencv.hpp>
#include <QFileDialog>

/**
 * @class CamerasManager
 * @brief Central manager for multiple cameras
 *
 * This class manages multiple camera instances, handles their lifecycle,
 * coordinates frame acquisition, and provides centralized logging.
 */
class CamerasManager : public QObject
{
	Q_OBJECT

public:
	/**
	 * @brief Constructor
	 * @param parent Parent QObject
	 */
	explicit CamerasManager(QObject *parent = nullptr);

	/**
	 * @brief Destructor
	 */
	~CamerasManager();

	/**
	 * @brief Add a new camera to the manager
	 * @return The ID of the newly added camera
	 */
	int addCamera();

	/**
	 * @brief Remove a camera by ID
	 * @param cameraId ID of the camera to remove
	 * @return true if successful
	 */
	bool removeCamera(int cameraId);

	/**
	 * @brief Get the number of cameras
	 * @return Number of cameras managed
	 */
	int getCameraCount() const
	{
		return m_cameras.size();
	}

	/**
	 * @brief Get a camera by ID
	 * @param cameraId Camera ID
	 * @return Pointer to Camera object or nullptr if not found
	 */
	Camera *getCamera(int cameraId) const;

	/**
	 * @brief Get all camera IDs
	 * @return List of all camera IDs
	 */
	QVector<int> getCameraIds() const;

	/**
	 * @brief Connect all cameras
	 * @return true if all cameras connected successfully
	 */
	bool connectAll();

	/**
	 * @brief Disconnect all cameras
	 */
	void disconnectAll();

	/**
	 * @brief Start acquisition on all cameras
	 * @return true if all cameras started successfully
	 */
	bool startAll();

	/**
	 * @brief Stop acquisition on all cameras
	 */
	void stopAll();

	/**
	 * @brief Get the latest frame from a specific camera
	 * @param cameraId Camera ID
	 * @return OpenCV Mat with the frame
	 */
	cv::Mat getFrame(int cameraId);

	/**
	 * @brief Get frames from all cameras
	 * @return Map of camera ID to frame
	 */
	QMap<int, cv::Mat> getAllFrames();

	/**
	 * @brief Get parameters for a specific camera
	 * @param cameraId Camera ID
	 * @return CameraParameters struct
	 */
	CameraParameters getCameraParameters(int cameraId);

	/**
	 * @brief Get the log history
	 * @return Vector of all log entries
	 */
	const QVector<LogEntry> &getLogHistory() const
	{
		return m_log_history;
	}

	/**
	 * @brief Clear the log history
	 */
	void clearLogs();

	/**
	 * @brief Configure the directory where log file will be stored
	 * @param directory Absolute path to the chosen directory
	 */
	void setLogDirectory( const QString& directory );

	/**
	 * @brief Start recording camera parameters to CSV files
	 * @param directory Directory where CSV files will be stored
	 * @param intervalMs Interval in milliseconds between writes (default 500ms)
	 */
	void startParameterLogging( const QString& directory, int intervalMs = 500 );

	/**
	 * @brief Stop recording camera parameters
	 */
	void stopParameterLogging();

	/**
	 * @brief Set exposure time for a specific camera
	 * @param cameraId Camera ID
	 * @param value Exposure time in µs
	 */
	void setExposureTime(int cameraId, double value);

	/**
	 * @brief Set gain for a specific camera
	 * @param cameraId Camera ID
	 * @param value Gain factor
	 */
	void setGain(int cameraId, double value);

	/**
	 * @brief Set power status for a specific camera
	 * @param cameraId Camera ID
	 * @param on true to power on
	 */
	void setPowerStatus(int cameraId, bool on);

	/**
	 * @brief Enable/disable automatic frame updates
	 * @param enabled true to enable
	 * @param intervalMs Update interval in milliseconds
	 */
	void setAutoUpdate(bool enabled, int intervalMs = 33); // ~30 FPS default

	void startRecording(QString directory, VideoFormat format = VideoFormat::AVI);

	void stopRecording();

signals:
	/**
	 * @brief Emitted when a camera is added
	 * @param cameraId ID of the added camera
	 */
	void cameraAdded(int cameraId);

	/**
	 * @brief Emitted when a camera is removed
	 * @param cameraId ID of the removed camera
	 */
	void cameraRemoved(int cameraId);

	/**
	 * @brief Emitted when frames are updated
	 */
	void framesUpdated();

	/**
	 * @brief Emitted when a new log entry is added
	 * @param entry The log entry
	 */
	void logAdded(const LogEntry &entry);

	/**
	 * @brief Emitted when camera parameters are updated
	 * @param cameraId ID of the camera
	 */
	void parametersUpdated(int cameraId);

private slots:
	/**
	 * @brief Handle errors from individual cameras
	 */
	void onCameraError(int cameraId, int errorCode, const QString &message);

	/**
	 * @brief Handle connection status changes
	 */
	void onConnectionStatusChanged(int cameraId, bool connected);

	/**
	 * @brief Auto-update timer callback
	 */
	void onAutoUpdateTimer();

	/**
	 * @brief Parameter logging timer callback
	 */
	void onParameterLogTimer();

private:
	/**
	 * @brief Add a log entry
	 */
	void addLog(LogLevel level, const QString &message, int cameraId = -1);

	/**
	 * @brief Write a log entry to the active log file
	 */
	void writeLogToFile( const LogEntry& entry );

	/**
	 * @brief Write camera parameters to CSV file
	 */
	void writeParametersToFile();

	void createParameterLogFile(int cameraId);

	QMap<int, Camera*> m_cameras;	///< Map of camera ID to Camera objects
	int m_next_camera_id;			///< Next available camera ID
    int m_interval_ms = 33;          ///< Interval for frame updates
	QVector<LogEntry> m_log_history; ///< Log history
	QTimer* m_auto_update_timer;		///< Timer for automatic frame updates
	bool m_auto_update_enabled;		///< Auto-update enabled flag
    VideoSaver m_videoSaver;        ///< Writer for saving files
	QFile m_log_file;                 ///< File handle for persisting logs
	QString m_log_directory;          ///< Selected directory for log file
	QTimer* m_parameter_log_timer;    ///< Timer for parameter logging
	bool m_parameter_logging_enabled; ///< Parameter logging enabled flag
    QFile* m_param_file = nullptr;              ///< Single CSV file for all camera parameters
	QString m_param_directory;        ///< Directory for parameter CSV file
	int m_param_log_interval;         ///< Interval for parameter logging in ms
};

#endif // CAMERASMANAGER_H
