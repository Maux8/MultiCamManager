#pragma once
#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QPushButton>

class CameraRowWidget : public QWidget
{
    Q_OBJECT
public:
    enum class Status { Power_On, Power_Off };
    enum class Recording {Recording, NotRecording};
    enum class Visibility { Visible, Hidden };

    explicit CameraRowWidget(QString name, int camera_id, QWidget* parent=nullptr);

    void setRecordingState(Recording current_status);
    void setStatus(Status current_status);
    void setVisibility(Visibility current_visibility);

signals:
    void visibilityToggled(int cameraId, bool state);

private:
    void on_visibility_clicked();

private:
    QPushButton* m_iconCam{};
    QLabel* m_name{};
    QLabel* m_isRecording{};
    QLabel* m_status{};

    bool m_visible = true;
    int m_iconSize = 18;
    int m_camera_id = -1;

};
