#include "videosaver.h"
#include <stdexcept>
#include <QDir>
#include <QDebug>

VideoSaver::VideoSaver(QObject *parent) : QObject(parent) {}

void VideoSaver::configureCameras(const QList<int> &cameraIds)
{
    m_streams.clear();
    for (int id : cameraIds)
    {
        CameraStream camStream;
        camStream.cameraId = id;
        m_streams[id] = std::move(camStream);
    }
}

VideoSaver::~VideoSaver()
{
    if (m_isRecording)
    {
        stopRecording();
    }
}

void VideoSaver::startRecording(const QString &outputDir, double fps, VideoFormat format)
{
    if (m_streams.empty())
        throw std::runtime_error("No cameras configured for VideoCapturer.");

    m_outputDir = outputDir;
    m_fps = fps;
    m_format = format;
    m_isRecording = true;

    QDir dir;
    if (!dir.exists(m_outputDir))
    {
        if (!dir.mkpath(m_outputDir))
        {
            throw std::runtime_error(
                std::string("Failed to create output directory: ") + m_outputDir.toStdString());
        }
    }

    // init writer at first frame to know resolution
    for (auto &[id, stream] : m_streams)
    {
        stream.writerInitialized = false;
    }

    qDebug() << "Recording Started";
}

void VideoSaver::stopRecording()
{
    if (!m_isRecording)
        return;

    // close all writers
    for (auto &[id, stream] : m_streams)
    {
        if (stream.writer.isOpened())
        {
            stream.writer.release();
        }
        stream.writerInitialized = false;
    }

    m_isRecording = false;
    qDebug() << "Recording Stopped";
}

void VideoSaver::onNewFrame(int cameraId, const cv::Mat &frame)
{
    if (!m_isRecording)
        return;

    auto it = m_streams.find(cameraId);
    if (it == m_streams.end())
        return;

    CameraStream &stream = it->second;

    if (!stream.writerInitialized)
    {
        stream.frameSize = cv::Size(frame.cols, frame.rows);

        QDir dir(m_outputDir);

        // file path : <outputDir>/camera_<id>.<extension>
        QString fileExtension = (m_format == VideoFormat::MP4) ? "mp4" : "avi";
        QString fileName = QString("camera_%1.%2").arg(cameraId).arg(fileExtension);
        QString fullPath = dir.filePath(fileName);

        // OpenCV needs std::string
        std::string pathStd = fullPath.toStdString();

        // Choose codec based on format
        int fourcc;
        if (m_format == VideoFormat::MP4)
        {
            // Try H.264 codec for MP4 (most compatible)
            fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4');
            // Alternative: cv::VideoWriter::fourcc('a', 'v', 'c', '1') or
            // cv::VideoWriter::fourcc('X', '2', '6', '4')
        }
        else
        {
            fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G'); // MJPEG codec for AVI
        }

        double fps = m_fps;
        // double fps = 33;
        bool ok = stream.writer.open(
            pathStd,
            fourcc,
            fps,
            stream.frameSize,
            frame.channels() == 3 // Farbvideo oder nicht
        );

        if (!ok)
        {
            throw std::runtime_error(
                "Failed to open VideoWriter for camera " + std::to_string(cameraId));
        }

        stream.writerInitialized = true;
    }

    // Frames hineinschreiben
    stream.writer.write(frame);
}
