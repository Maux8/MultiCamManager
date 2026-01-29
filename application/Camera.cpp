#include "Camera.h"
#include <QDebug>

Camera::Camera(const int id, QObject* parent ) :
    QObject( parent ), m_id( id ), m_simulator( new CameraSimulatorLib ), m_is_connected( false ), m_is_running( false )
{
}

Camera::~Camera()
{
	if ( m_is_running )
	{
		stop();
	}
	if ( m_is_connected )
	{
		disconnect();
	}
	delete m_simulator;
}

bool Camera::connect()
{
	if ( m_is_connected )
	{
		return true;
	}

	if ( m_simulator->connect() )
	{
		m_is_connected = true;
		emit connectionStatusChanged( m_id, true );
		qDebug() << "Camera" << m_id << "connected successfully";
		return true;
	}
	else
	{
		emit errorOccurred( m_id, -1, "Failed to connect camera" );
		return false;
	}
}

void Camera::disconnect()
{
	if ( !m_is_connected )
	{
		return;
	}

	if ( m_is_running )
	{
		stop();
	}

	m_simulator->disconnect();
	m_is_connected = false;
	emit connectionStatusChanged( m_id, false );
	qDebug() << "Camera" << m_id << "disconnected";
}

bool Camera::start()
{
	if ( !m_is_connected )
	{
		emit errorOccurred( m_id, -2, "Cannot start: camera not connected" );
		return false;
	}

	if ( m_is_running )
	{
		return true;
	}

	if ( m_simulator->start() )
	{
		m_is_running = true;
		qDebug() << "Camera" << m_id << "started acquisition";
		return true;
	}
	else
	{
		emit errorOccurred( m_id, -3, "Failed to start acquisition" );
		return false;
	}
}

void Camera::stop()
{
	if ( !m_is_running )
	{
		return;
	}

	m_simulator->stop();
	m_is_running = false;
	qDebug() << "Camera" << m_id << "stopped acquisition";
}

cv::Mat Camera::getFrame()
{
	if ( !m_is_running )
	{
		return {};
	}

	cv::Mat frame = m_simulator->getFrame();
	if ( !frame.empty() )
	{
		emit frameReady( m_id );
	}
    else if ( frame.empty()) {
        qDebug() << "[Camera] frame empty";
    }
	return frame;
}

CameraParameters Camera::getParameters()
{
	if ( !m_is_connected )
	{
		return m_parameters;
	}

	// Update parameters from simulator
	m_parameters.temperature = m_simulator->getTemperature();
	m_parameters.fps = m_simulator->getFPS();
	m_parameters.exposureTime = m_simulator->getExposureTime();
	m_parameters.gain = m_simulator->getGain();
	m_parameters.power_status = m_simulator->getPowerStatus();
	m_parameters.frame_counter = m_simulator->getFrameCounter();
	m_parameters.error_code = m_simulator->getErrorCode();

	return m_parameters;
}

void Camera::setExposureTime(const double value )
{
	if ( m_is_connected )
	{
		m_simulator->setExposureTime( value );
		m_parameters.exposureTime = value;
	}
}

void Camera::setGain(const double value )
{
	if ( m_is_connected )
	{
		m_simulator->setGain( value );
		m_parameters.gain = value;
	}
}

void Camera::setPowerStatus(const bool on )
{
	if ( m_is_connected )
	{
		m_simulator->setPowerStatus( on );
		m_parameters.power_status = on;
	}
}
