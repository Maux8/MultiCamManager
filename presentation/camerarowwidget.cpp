#include "CameraRowWidget.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QStyle>
#include <QDebug>
#include <QFile>

CameraRowWidget::CameraRowWidget(QString name, int id, QWidget* parent)
    : QWidget(parent), m_camera_id(id)
{
    auto* box_lay = new QHBoxLayout(this);
    box_lay->setContentsMargins(6, 2, 6, 2);
    box_lay->setSpacing(8);

    // cam visibility Icon ---
    m_iconCam = new QPushButton(this);
    setVisibility(Visibility::Visible);
    //m_iconCam->setFlat(true);
    m_iconCam->setCursor(Qt::PointingHandCursor);
    m_iconCam->setIconSize(QSize(m_iconSize, m_iconSize));
    m_iconCam->setFixedSize(m_iconSize, m_iconSize);
    connect(m_iconCam, &QPushButton::clicked, this, &CameraRowWidget::on_visibility_clicked);

    // Name Label ---
    m_name = new QLabel(name, this);
    m_name->setMinimumWidth(0);

    // is Recording Icon ---
    m_isRecording = new QLabel(this);
    setRecordingState(Recording::NotRecording); // also sets the icon
    m_isRecording->setFixedSize(m_iconSize, m_iconSize);

    // status Icon ---
    m_status = new QLabel(this);
    setStatus(Status::Power_On);
    m_status->setFixedSize(m_iconSize, m_iconSize);

    box_lay->addWidget(m_iconCam);
    box_lay->addWidget(m_isRecording);
    box_lay->addWidget(m_status);
    box_lay->addWidget(m_name, 1);
}

void CameraRowWidget::setRecordingState(Recording currentState)
{
    if (currentState == Recording::Recording) {
        QPixmap pix_map(":/res/img/video_on.png");
        m_isRecording->setPixmap(pix_map);
    }
    else if (currentState == Recording::NotRecording) {
        QPixmap pix_map(":/res/img/video_off.png");
        m_isRecording->setPixmap(pix_map);
    }
}

void CameraRowWidget::setStatus(Status current_status)
{
    if (current_status == Status::Power_On) {
        QPixmap pix_map(":/res/img/power_on.png");
        m_status->setPixmap(pix_map);
    }
    else if (current_status == Status::Power_Off) {
        QPixmap pix_map(":/res/img/power_off.png");
        m_status->setPixmap(pix_map);
    }
}

void CameraRowWidget::setVisibility (Visibility current_visibility)
{
    if (current_visibility == Visibility::Visible) {
        QIcon icon(":/res/img/visible.png");
        m_iconCam->setIcon(icon);
    }
    else if (current_visibility == Visibility::Hidden) {
        QIcon icon(":/res/img/hide.png");
        m_iconCam->setIcon(icon);
    }
}

void CameraRowWidget::on_visibility_clicked() {
    m_visible = !m_visible;
    setVisibility(m_visible ? Visibility::Visible : Visibility::Hidden);
    emit visibilityToggled(m_camera_id, m_visible);
}
