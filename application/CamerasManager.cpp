#include "CamerasManager.h"
#include <QDebug>
#include <QDateTime>

CamerasManager::CamerasManager( QObject* parent )
    : QObject( parent ), m_next_camera_id( 0 ), m_auto_update_enabled( false ), m_parameter_logging_enabled( false ), m_videoSaver(VideoSaver(this))
{
	m_auto_update_timer = new QTimer( this );
	connect( m_auto_update_timer, &QTimer::timeout, this, &CamerasManager::onAutoUpdateTimer );

	m_parameter_log_timer = new QTimer( this );
	connect( m_parameter_log_timer, &QTimer::timeout, this, &CamerasManager::onParameterLogTimer );

    m_videoSaver.configureCameras(m_cameras.keys());

	addLog(LogLevel::Info, "CamerasManager initialized");
}

CamerasManager::~CamerasManager()
{
	stopAll();
	disconnectAll();
	stopParameterLogging();

	// Clean up all cameras
	for (const auto *camera : m_cameras)
	{
		delete camera;
	}
	m_cameras.clear();

	addLog(LogLevel::Info, "CamerasManager destroyed");
}

int CamerasManager::addCamera()
{
	const int cameraId = m_next_camera_id++;
	const auto camera = new Camera(cameraId, this);

	// Connect signals
	connect(camera, &Camera::errorOccurred, this, &CamerasManager::onCameraError);
	connect(camera, &Camera::connectionStatusChanged, this, &CamerasManager::onConnectionStatusChanged);

	m_cameras[cameraId] = camera;

	addLog(LogLevel::Info, QString("Camera added with ID %1").arg(cameraId), cameraId);
	emit cameraAdded(cameraId);
	m_videoSaver.configureCameras(m_cameras.keys());

	return cameraId;
}

bool CamerasManager::removeCamera(const int cameraId)
{
	if (!m_cameras.contains(cameraId))
	{
		addLog(LogLevel::Warning, QString("Cannot remove: Camera ID %1 not found").arg(cameraId));
		return false;
	}

	Camera *camera = m_cameras[cameraId];
	camera->stop();
	camera->disconnect();

	m_cameras.remove(cameraId);
	delete camera;

	addLog(LogLevel::Info, QString("Camera removed"), cameraId);
	emit cameraRemoved(cameraId);
	m_videoSaver.configureCameras(m_cameras.keys());

	return true;
}

Camera *CamerasManager::getCamera(int cameraId) const
{
	return m_cameras.value(cameraId, nullptr);
}

QVector<int> CamerasManager::getCameraIds() const
{
	return m_cameras.keys().toVector();
}

bool CamerasManager::connectAll()
{
	addLog(LogLevel::Info, "Connecting all cameras...");

	bool allSuccess = true;
	for (auto *camera : m_cameras)
	{
		if (!camera->connect())
		{
			allSuccess = false;
			addLog(LogLevel::Error, QString("Failed to connect camera %1").arg(camera->getId()), camera->getId());
		}
	}

	if (allSuccess)
	{
		addLog(LogLevel::Info, "All cameras connected successfully");
	}
	else
	{
		addLog(LogLevel::Warning, "Some cameras failed to connect");
	}

	return allSuccess;
}

void CamerasManager::disconnectAll()
{
	addLog(LogLevel::Info, "Disconnecting all cameras...");

	for (auto *camera : m_cameras)
	{
		camera->disconnect();
	}

	addLog(LogLevel::Info, "All cameras disconnected");
}

bool CamerasManager::startAll()
{
	addLog(LogLevel::Info, "Starting acquisition on all cameras...");

	bool allSuccess = true;
	for (auto *camera : m_cameras)
	{
		if (!camera->start())
		{
			allSuccess = false;
			addLog(LogLevel::Error, QString("Failed to start camera %1").arg(camera->getId()), camera->getId());
		}
	}

	if (allSuccess)
	{
		addLog(LogLevel::Info, "All cameras started successfully");
	}
	else
	{
		addLog(LogLevel::Warning, "Some cameras failed to start");
	}

	return allSuccess;
}

void CamerasManager::stopAll()
{
	addLog(LogLevel::Info, "Stopping acquisition on all cameras...");

	for (auto *camera : m_cameras)
	{
		camera->stop();
	}

	addLog(LogLevel::Info, "All cameras stopped");
}

cv::Mat CamerasManager::getFrame(const int cameraId)
{
	Camera *camera = getCamera(cameraId);
	if (!camera)
	{
		addLog(LogLevel::Warning, QString("Cannot get frame: Camera ID %1 not found").arg(cameraId));
		return {};
	}
	return camera->getFrame();
}

QMap<int, cv::Mat> CamerasManager::getAllFrames()
{
	QMap<int, cv::Mat> frames;

	for (auto it = m_cameras.begin(); it != m_cameras.end(); ++it)
	{
		int cameraId = it.key();

		if (Camera *camera = it.value(); camera->isRunning())
		{
			if (cv::Mat frame = camera->getFrame(); !frame.empty())
			{
				frames[cameraId] = frame;
			}
		}
	}

	return frames;
}

CameraParameters CamerasManager::getCameraParameters(const int cameraId)
{
	Camera *camera = getCamera(cameraId);
	if (!camera)
	{
		addLog(LogLevel::Warning, QString("Cannot get parameters: Camera ID %1 not found").arg(cameraId));
		return {};
	}

	return camera->getParameters();
}

void CamerasManager::clearLogs()
{
	m_log_history.clear();
	addLog(LogLevel::Info, "Log history cleared");
}

void CamerasManager::setLogDirectory( const QString& directory )
{
	if ( directory.isEmpty() )
	{
		qWarning() << "Log directory is empty";
		return;
	}

	QDir dir( directory );
	if ( !dir.exists() && !dir.mkpath( "." ) )
	{
		qWarning() << "Unable to create log directory" << directory;
		return;
	}

	const QString timestamp = QDateTime::currentDateTime().toString( "yyyyMMdd_hhmmss" );
	const QString filePath = dir.filePath( QString( "multicam_log_%1.txt" ).arg( timestamp ) );

	if ( m_log_file.isOpen() )
	{
		m_log_file.close();
	}

	m_log_file.setFileName( filePath );
	if ( !m_log_file.open( QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text ) )
	{
		qWarning() << "Failed to open log file" << filePath << ":" << m_log_file.errorString();
		m_log_file.setFileName( QString() );
		return;
	}

	m_log_directory = dir.absolutePath();

	QTextStream out( &m_log_file );
	out << "==== MultiCamManager Log started at "
		 << QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" )
		 << " ===="
		 << Qt::endl;

	out.flush();

	for ( const auto& entry : m_log_history )
	{
		writeLogToFile( entry );
	}

	addLog( LogLevel::Info, QString( "Log file created at %1" ).arg( filePath ) );
}

void CamerasManager::setExposureTime(const int cameraId, const double value )
{
	if (Camera *camera = getCamera(cameraId))
	{
		camera->setExposureTime(value);
		addLog(LogLevel::Info, QString("Exposure time set to %1 µs").arg(value), cameraId);
		emit parametersUpdated(cameraId);
	}
}

void CamerasManager::setGain(const int cameraId, const double value)
{
	if (Camera *camera = getCamera(cameraId))
	{
		camera->setGain(value);
		addLog(LogLevel::Info, QString("Gain set to %1").arg(value), cameraId);
		emit parametersUpdated(cameraId);
	}
}

void CamerasManager::setPowerStatus(const int cameraId, const bool on)
{
	if (Camera *camera = getCamera(cameraId))
	{
		camera->setPowerStatus(on);
		addLog(LogLevel::Info, QString("Power %1").arg(on ? "ON" : "OFF"), cameraId);
		emit parametersUpdated(cameraId);
	}
}

void CamerasManager::setAutoUpdate(const bool enabled, const int intervalMs)
{
	m_auto_update_enabled = enabled;

	if (enabled)
	{
		m_auto_update_timer->start(intervalMs);
		addLog(LogLevel::Info, QString("Auto-update enabled (%1 ms interval)").arg(intervalMs));
	}
	else
	{
		m_auto_update_timer->stop();
		addLog(LogLevel::Info, "Auto-update disabled");
	}
}

void CamerasManager::onCameraError(const int cameraId, const int errorCode, const QString &message)
{
	addLog(LogLevel::Error, QString("Error %1: %2").arg(errorCode).arg(message), cameraId);
}

void CamerasManager::onConnectionStatusChanged(const int cameraId, const bool connected)
{
	addLog(
		LogLevel::Info, QString("Connection status: %1").arg(connected ? "Connected" : "Disconnected"), cameraId);
}

void CamerasManager::onAutoUpdateTimer()
{
	if (m_auto_update_enabled)
	{
		emit framesUpdated();

		// Also emit parameter updates for monitoring
		for (const int cameraId : getCameraIds())
		{
			emit parametersUpdated(cameraId);

			if (m_videoSaver.isRecording())
			{
				m_videoSaver.onNewFrame(cameraId, getFrame(cameraId));
			}
		}
	}
}

void CamerasManager::startRecording(QString directory, VideoFormat format)
{
	if (!m_videoSaver.isRecording())
	{
		m_videoSaver.startRecording(directory, m_interval_ms, format);
	}
}

void CamerasManager::stopRecording()
{
	if (m_videoSaver.isRecording())
	{
		m_videoSaver.stopRecording();
	}
}
void CamerasManager::addLog(const LogLevel level, const QString &message, const int cameraId)
{
	const LogEntry entry( level, message, cameraId );
	m_log_history.append( entry );
	writeLogToFile( entry );
	emit logAdded( entry );

	// Also output to Qt debug console
	qDebug() << entry.toString();
}

void CamerasManager::writeLogToFile( const LogEntry& entry )
{
	if ( !m_log_file.isOpen() )
	{
		return;
	}

	QTextStream out( &m_log_file );
	out << entry.toString() << Qt::endl;
	m_log_file.flush();
}

void CamerasManager::startParameterLogging( const QString& directory, int intervalMs )
{
	if ( directory.isEmpty() )
	{
		qWarning() << "Parameter logging directory is empty";
		return;
	}

	QDir dir( directory );
	if ( !dir.exists() && !dir.mkpath( "." ) )
	{
		qWarning() << "Unable to create parameter logging directory" << directory;
		return;
	}

	m_param_directory = dir.absolutePath();
	m_param_log_interval = intervalMs;
	m_parameter_logging_enabled = true;

	// Close any existing parameter file
    if (m_param_file) {
        if (m_param_file->isOpen())
            m_param_file->close();

        delete m_param_file;
        m_param_file = nullptr;
    }

	// Create single CSV file for all cameras
	const QString timestamp = QDateTime::currentDateTime().toString( "yyyyMMdd_hhmmss" );
	const QString filePath = dir.filePath( QString( "all_cameras_params_%1.csv" ).arg( timestamp ) );

    m_param_file = new QFile( filePath, this );
	if ( !m_param_file->open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		qWarning() << "Failed to open parameter file" << filePath << ":" << m_param_file->errorString();
		delete m_param_file;
		m_param_file = nullptr;
		return;
	}

	// Write CSV header with camera_id column
	QTextStream out( m_param_file );
	out << "timestamp,camera_id,camera_name,fps,temperature" << Qt::endl;
	out.flush();

	// Start the timer
	m_parameter_log_timer->start( intervalMs );

	addLog( LogLevel::Info, QString( "Parameter logging started at %1 (interval: %2ms)" )
		.arg( m_param_directory ).arg( intervalMs ) );
}

void CamerasManager::stopParameterLogging()
{
	if ( m_parameter_log_timer )
	{
		m_parameter_log_timer->stop();
	}

	m_parameter_logging_enabled = false;

	// Close parameter file
	if ( m_param_file )
	{
		if ( m_param_file->isOpen() )
		{
			m_param_file->close();
		}
		delete m_param_file;
		m_param_file = nullptr;
	}

	addLog( LogLevel::Info, "Parameter logging stopped" );
}

void CamerasManager::onParameterLogTimer()
{
	if ( m_parameter_logging_enabled )
	{
		writeParametersToFile();
	}
}

void CamerasManager::writeParametersToFile()
{
	if ( !m_param_file || !m_param_file->isOpen() )
	{
		return;
	}

	const QDateTime now = QDateTime::currentDateTime();
	const QString timestamp = now.toString( "yyyy-MM-dd hh:mm:ss.zzz" );

	QTextStream out( m_param_file );

	for ( const auto& cameraId : getCameraIds() )
	{
		const CameraParameters params = getCameraParameters( cameraId );
		const QString cameraName = QString( "Camera %1" ).arg( cameraId );

		out << timestamp << "," << cameraId << "," << cameraName << "," << params.fps << "," << params.temperature << Qt::endl;
	}

	m_param_file->flush();
}

void CamerasManager::createParameterLogFile( int cameraId )
{
	// This method is no longer used with consolidated logging
	Q_UNUSED( cameraId );
}
