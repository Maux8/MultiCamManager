#ifndef VIDEOSAVER_H
#define VIDEOSAVER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <map>

enum class VideoFormat
{
    AVI,
    MP4
};

class VideoSaver : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    VideoSaver(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~VideoSaver();

    /// @brief CamManager clarifies cam - Id relations
    void configureCameras(const QList<int> &cameraIds);

    /// @brief starts all recordings and opens one file per cam
    /// @param outputDir dir to save the files to
    /// @param fps for recordings
    /// @param format video format (AVI or MP4)
    void startRecording(const QString &outputDir, double fps, VideoFormat format = VideoFormat::AVI);

    /// @brief stops all recordings and closes files
    void stopRecording();

    /// @brief takes new frame from CamManager and writes it into VideoWriter
    /// @param cameraId id of Cam the current frame came from
    /// @param current frame
    void onNewFrame(int cameraId, const cv::Mat &frame);

    /// @brief true if recording
    bool isRecording() const { return m_isRecording; }

private:
    struct CameraStream
    {
        int cameraId;
        cv::VideoWriter writer;
        bool writerInitialized = false;
        cv::Size frameSize;
    };

    std::map<int, CameraStream> m_streams;
    bool m_isRecording = false;
    QString m_outputDir;
    double m_fps = 33.0;
    VideoFormat m_format = VideoFormat::AVI;
};

#endif // VIDEOSAVER_H
