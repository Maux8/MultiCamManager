#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QDateTime>
#include <QString>
#include <utility>

/**
 * @enum LogLevel
 * @brief Defines severity levels for log entries
 */
enum class LogLevel
{
	Info,	 ///< Informational message
	Warning, ///< Warning message
	Error,	 ///< Error message
	Debug	 ///< Debug message
};

/**
 * @struct LogEntry
 * @brief Represents a single log entry with timestamp and level
 */
struct LogEntry
{
	QDateTime timestamp; ///< When the log was created
	LogLevel level;		 ///< Severity level
	QString message;	 ///< Log message content
	int camera_id;		 ///< Associated camera ID (-1 for system-wide logs)

	/**
	 * @brief Constructor
	 */
	LogEntry( LogLevel lvl = LogLevel::Info, QString  msg = "", int camId = -1 ) :
		timestamp( QDateTime::currentDateTime() ), level( lvl ), message(std::move( msg )), camera_id( camId )
	{
	}

	/**
	 * @brief Convert log level to string
	 */
	QString levelToString() const
	{
		switch ( level )
		{
		case LogLevel::Info:
			return "INFO";
		case LogLevel::Warning:
			return "WARNING";
		case LogLevel::Error:
			return "ERROR";
		case LogLevel::Debug:
			return "DEBUG";
		default:
			return "UNKNOWN";
		}
	}

	/**
	 * @brief Format the log entry as a string
	 */
	QString toString() const
	{
		QString camStr = ( camera_id >= 0 ) ? QString( "[Cam %1] " ).arg( camera_id ) : "";
		return QString( "[%1] %2%3: %4" )
			.arg(timestamp.toString( "yyyy-MM-dd hh:mm:ss" ), levelToString(), camStr, message);
	}
};

#endif // LOGENTRY_H
