#ifndef CAMERA_H
#define CAMERA_H

#include "../extern/CameraSimulator/CameraSimulatorLib.h"
#include "CameraParameters.h"
#include <QObject>
#include <QString>
#include <opencv2/opencv.hpp>

/**
 * @class Camera
 * @brief Wrapper for CameraSimulatorLib, represents a single camera
 *
 * This class manages a single camera instance, handling connection,
 * frame acquisition, and parameter management.
 */
class Camera : public QObject
{
	Q_OBJECT

public:
	/**
	 * @brief Constructor
	 * @param id Unique identifier for this camera
	 * @param parent Parent QObject
	 */
	explicit Camera( int id, QObject* parent = nullptr );

	/**
	 * @brief Destructor
	 */
	~Camera();

	/**
	 * @brief Get the camera ID
	 * @return Camera ID
	 */
	int getId() const
	{
		return m_id;
	}

	/**
	 * @brief Connect to the camera
	 * @return true if successful
	 */
	bool connect();

	/**
	 * @brief Disconnect from the camera
	 */
	void disconnect();

	/**
	 * @brief Start frame acquisition
	 * @return true if successful
	 */
	bool start();

	/**
	 * @brief Stop frame acquisition
	 */
	void stop();

	/**
	 * @brief Check if camera is connected
	 * @return true if connected
	 */
	bool isConnected() const
	{
		return m_is_connected;
	}

	/**
	 * @brief Check if camera is running (acquiring frames)
	 * @return true if running
	 */
	bool isRunning() const
	{
		return m_is_running;
	}

	/**
	 * @brief Get the current frame from the camera
	 * @return OpenCV Mat containing the frame
	 */
	cv::Mat getFrame();

	/**
	 * @brief Update and retrieve current camera parameters
	 * @return CameraParameters struct with current values
	 */
	CameraParameters getParameters();

	/**
	 * @brief Set exposure time
	 * @param value Exposure time in µs
	 */
	void setExposureTime( double value );

	/**
	 * @brief Set gain value
	 * @param value Gain factor
	 */
	void setGain( double value );

	/**
	 * @brief Set power status
	 * @param on true to power on, false to power off
	 */
	void setPowerStatus( bool on );

signals:
	/**
	 * @brief Emitted when a new frame is available
	 * @param cameraId ID of this camera
	 */
	void frameReady( int cameraId );

	/**
	 * @brief Emitted when an error occurs
	 * @param cameraId ID of this camera
	 * @param errorCode Error code
	 * @param message Error message
	 */
	void errorOccurred( int cameraId, int errorCode, const QString& message );

	/**
	 * @brief Emitted when connection status changes
	 * @param cameraId ID of this camera
	 * @param connected true if connected
	 */
	void connectionStatusChanged( int cameraId, bool connected );

private:
	int m_id;						 ///< Camera identifier
	CameraSimulatorLib* m_simulator; ///< Pointer to the simulator instance
	bool m_is_connected;				 ///< Connection status
	bool m_is_running;				 ///< Acquisition status
	CameraParameters m_parameters;	 ///< Current camera parameters
};

#endif // CAMERA_H
