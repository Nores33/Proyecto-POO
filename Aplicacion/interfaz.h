#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMediaPlayer>
#include <QSlider>
#include <QPushButton>
#include <QLineEdit>
#include <QAudioOutput>
#include <QList>

class QLabel;
class GrabarAudio;

namespace Ui {
class Interfaz;
}

struct HistorialItem {
    int id;
    QString nombreArchivo;
    QString transcripcion;
};

class Interfaz : public QWidget
{
    Q_OBJECT

public:
    explicit Interfaz(QWidget *parent = nullptr, const QString& usuarioActual = "", const QString& tokenJWT = "");
    ~Interfaz();

    void setUsuario(const QString& usuarioNuevo);

public slots:
    void recargarConfiguracionBackend();
    void on_cambiarClave_clicked();
    void on_cambiarClaveReply();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void on_loadButton_clicked();
    void on_microButton_clicked();
    void on_exportTxtButton_clicked();
    void on_exportWordButton_clicked();
    void on_exportPdfButton_clicked();
    void on_toolButton_clicked();
    void on_logoutButton_clicked();
    void on_usuarioAtras_clicked();
    void audioGrabado(QString archivo);
    void on_historialTranscripcion_clicked(const QModelIndex &index);
    void on_saveTranscriptionButton_clicked();
    void on_searchHistorial_textChanged(const QString &text);
    void on_btnPlayPause_clicked();
    void on_barraProgreso_sliderMoved(int position);
    void on_reproductor_positionChanged(qint64 position);
    void on_reproductor_durationChanged(qint64 duration);
    void on_reproductor_stateChanged(QMediaPlayer::PlaybackState state);
    void on_exportButton_clicked();

private:
    void agregarAlHistorial(const QString& texto);
    void actualizarListaHistorial(const QString& filtro = QString());
    void prepararReproductorParaAudio(const QString& rutaAudio);
    void mostrarTranscripcionCompleta(const QString& titulo, const QString& texto);

    Ui::Interfaz *ui;
    QString usuario;
    QString token; // JWT
    QString rutaAudioCargado;
    QWidget* panelUsuario;
    QStandardItemModel* transcripcionesModel;
    QList<HistorialItem> historialTranscripciones;
    QLabel* userLabel = nullptr;
    QString OPENAI_API_KEY;

    bool grabando = false;
    GrabarAudio* grabarAudioPtr = nullptr;

    QLineEdit* searchHistorial = nullptr;

    QMediaPlayer* reproductorAudio = nullptr;
    QAudioOutput* audioOutput = nullptr;
    bool usuarioMoviendoSlider = false;
};

#endif // INTERFAZ_H
