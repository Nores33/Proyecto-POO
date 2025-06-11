#ifndef ADMINAPI_H
#define ADMINAPI_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonArray>

class AdminAPI : public QObject {
    Q_OBJECT
private:
    static AdminAPI *instancia;
    QNetworkAccessManager networkManager;
    QString urlBase;

    AdminAPI();
    void cargarUrlBase();

public:
    static AdminAPI *getInstancia();
    void reloadUrlBase();
    QString getUrlBase() const { return urlBase; }
    QString usuarioActual;

    // Métodos de red (usuarios, autenticación)
    QNetworkReply* iniciarSesion(const QString &usuario, const QString &contrasena);
    QNetworkReply* registrarUsuario(const QString &nombre, const QString &apellido, const QString &usuario, const QString &contrasena, const QString &correo);
    QNetworkReply* cambiarContrasena(const QString &usuario, const QString &contrasenaActual, const QString &nuevaContrasena);

    // Historial con FastAPI
    void leerHistorial();
    void agregarHistorial(const QString& nombreArchivo, const QString& transcripcion);
    void borrarHistorial(int historial_id);

    // OpenAI Key vía API
    void solicitarOpenAIKey();

    // Transcripción
    QNetworkReply* transcribirAudioConWhisper(const QString &rutaAudio, const QString &apiKey);

    // Manejo dinámico de la URL del backend
    void forzarNuevaUrlBase();
    void handleNetworkError(QNetworkReply* reply);
    void conectarErrorReply(QNetworkReply* reply);

signals:
    void whisperTranscripcionLista(const QString &texto);
    void urlBaseReloaded(const QString& nuevaUrlBase);

    // Para OpenAI Key
    void openAIKeyReady(const QString& apiKey);
    void openAIKeyError(const QString& error);

    // Para historial
    void historialLeido(const QJsonArray& historial);
    void historialAgregado();
    void historialModificado();
    void historialBorrado();
    void historialError(const QString& msg);
};

#endif // ADMINAPI_H
