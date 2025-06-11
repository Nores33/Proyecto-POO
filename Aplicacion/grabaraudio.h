#ifndef GRABARAUDIO_H
#define GRABARAUDIO_H

#include <QObject>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QAudioInput>

class GrabarAudio : public QObject
{
    Q_OBJECT

public:
    explicit GrabarAudio(QObject *parent = nullptr);
    ~GrabarAudio();

    void startGrabacion();
    void stopGrabacion();

signals:
    void audioGrabado(QString archivo);

private:
    QMediaCaptureSession *session;
    QMediaRecorder *recorder;
    QAudioInput *audioInput;
    QString rutaArchivoTemporal;
    bool grabando = false;
};

#endif // GRABARAUDIO_H
