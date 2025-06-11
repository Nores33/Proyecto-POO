#include "grabaraudio.h"
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QFile>

/**
 * @brief Constructor. Inicializa los componentes de grabación.
 */
GrabarAudio::GrabarAudio(QObject *parent)
    : QObject(parent)
{
    session = new QMediaCaptureSession(this);
    audioInput = new QAudioInput(this);
    recorder = new QMediaRecorder(this);

    session->setAudioInput(audioInput);
    session->setRecorder(recorder);

    connect(recorder, &QMediaRecorder::recorderStateChanged, this, [this](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::StoppedState && !rutaArchivoTemporal.isEmpty()) {
            QFile file(rutaArchivoTemporal);
            if (file.exists() && file.size() > 0) {
                qDebug() << "[GrabarAudio] Grabación finalizada con éxito:" << rutaArchivoTemporal;
                emit audioGrabado(rutaArchivoTemporal);
            } else {
                qDebug() << "[GrabarAudio] Error: El archivo no se creó correctamente.";
            }
        }
    });
}

GrabarAudio::~GrabarAudio()
{
    stopGrabacion();
}

void GrabarAudio::startGrabacion()
{
    QString nombreArchivo = QString("grabacion_temp_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());
    rutaArchivoTemporal = QDir::temp().filePath(nombreArchivo);

    recorder->setOutputLocation(QUrl::fromLocalFile(rutaArchivoTemporal));
    recorder->record();
    grabando = true;
    qDebug() << "[GrabarAudio] Grabando en:" << rutaArchivoTemporal;
}

void GrabarAudio::stopGrabacion()
{
    if (grabando) {
        recorder->stop();
        grabando = false;
        qDebug() << "[GrabarAudio] Grabación detenida. Archivo:" << rutaArchivoTemporal;
    }
}
