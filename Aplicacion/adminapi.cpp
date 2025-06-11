#include "adminapi.h"
#include "errorhandler.h"
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

AdminAPI* AdminAPI::instancia = nullptr;

void AdminAPI::cargarUrlBase() {
    // Ubicación: tres niveles arriba de la carpeta del ejecutable
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir confDir(exeDir);
    confDir.cdUp(); confDir.cdUp(); confDir.cdUp();
    QString configPath = confDir.absoluteFilePath("backend_url.conf");

    QFile confFile(configPath);
    QString fileUrl;

    if (confFile.exists()) {
        if (confFile.open(QFile::ReadOnly | QFile::Text)) {
            fileUrl = QString::fromUtf8(confFile.readAll()).trimmed();
            confFile.close();
        }
    }

    // Si no hay url en el archivo, se la pedimos al usuario y la guardamos
    while (fileUrl.isEmpty()) {
        bool ok = false;
        QString inputUrl = QInputDialog::getText(nullptr, "Configurar Backend", "Ingrese la URL o IP del backend (ej: http://192.168.1.123:8000):", QLineEdit::Normal, "", &ok);
        if (!ok || inputUrl.trimmed().isEmpty()) {
            QMessageBox::critical(nullptr, "Sin backend", "No se puede continuar sin URL de backend.");
            qApp->exit(1);
            return;
        }
        fileUrl = inputUrl.trimmed();
        // Guardar la URL en el archivo, lo crea si no existe
        if (confFile.open(QFile::WriteOnly | QFile::Text)) {
            confFile.write(fileUrl.toUtf8());
            confFile.close();
        } else {
            QMessageBox::critical(nullptr, "Error", "No se pudo crear backend_url.conf");
            qApp->exit(1);
            return;
        }
    }

    urlBase = fileUrl;
    qDebug() << "[AdminAPI] URL base cargada de backend_url.conf:" << urlBase;
}

AdminAPI::AdminAPI() : QObject() {
    cargarUrlBase();
}

AdminAPI* AdminAPI::getInstancia() {
    if (!instancia) {
        instancia = new AdminAPI();
        qDebug() << "[AdminAPI] Instancia creada";
    }
    return instancia;
}

void AdminAPI::reloadUrlBase() {
    QString prevUrl = urlBase;
    cargarUrlBase();
    if (urlBase != prevUrl) {
        qDebug() << "[AdminAPI] URL base recargada:" << urlBase;
        emit urlBaseReloaded(urlBase);
    } else {
        qDebug() << "[AdminAPI] URL base recargada, sin cambios.";
    }
}

// --- NUEVO: Cambiar backend en caliente ---
void AdminAPI::forzarNuevaUrlBase() {
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir confDir(exeDir);
    confDir.cdUp(); confDir.cdUp(); confDir.cdUp();
    QString configPath = confDir.absoluteFilePath("backend_url.conf");

    bool ok = false;
    QString nuevaUrl = QInputDialog::getText(nullptr, "Cambiar Backend", "Ingrese la nueva URL o IP del backend (ej: http://192.168.1.123:8000):", QLineEdit::Normal, urlBase, &ok);
    if (ok && !nuevaUrl.trimmed().isEmpty()) {
        QFile confFile(configPath);
        if (confFile.open(QFile::WriteOnly | QFile::Text)) {
            confFile.write(nuevaUrl.trimmed().toUtf8());
            confFile.close();
            urlBase = nuevaUrl.trimmed();
            emit urlBaseReloaded(urlBase);
            QMessageBox::information(nullptr, "Listo", "URL del backend actualizada a:\n" + urlBase);
        } else {
            QMessageBox::critical(nullptr, "Error", "No se pudo guardar backend_url.conf");
        }
    }
}

// --- NUEVO: Manejo de errores de conexión ---
void AdminAPI::handleNetworkError(QNetworkReply* reply) {
    QString errorMsg = reply->errorString();
    int ret = QMessageBox::critical(nullptr, "Error de conexión",
                                    "No se pudo conectar con el backend:\n" + errorMsg +
                                        "\n\n¿Desea cambiar la URL/IP del backend?",
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        forzarNuevaUrlBase();
    }
}

// --- MODIFICADO: Conexión automática de error ---
// Solo muestra el diálogo para cambiar la IP si es un error de red real (no 401/403)
void AdminAPI::conectarErrorReply(QNetworkReply* reply) {
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            // Solo mostrar el diálogo de cambiar IP si es un error de red real
            QNetworkReply::NetworkError err = reply->error();
            if (err == QNetworkReply::HostNotFoundError ||
                err == QNetworkReply::ConnectionRefusedError ||
                err == QNetworkReply::TimeoutError ||
                err == QNetworkReply::UnknownNetworkError) {
                handleNetworkError(reply);
            }
            // Para otros errores (como autenticación), NO mostrar el diálogo, el manejo va en el slot de la ventana
        }
        reply->deleteLater();
    });
}

// Usuarios y autenticación
QNetworkReply* AdminAPI::iniciarSesion(const QString &usuario, const QString &contrasena) {
    qDebug() << "[AdminAPI] iniciarSesion usuario:" << usuario;
    QUrl url(urlBase + "/auth/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("username", usuario);
    params.addQueryItem("password", contrasena);
    QNetworkReply* reply = networkManager.post(request, params.query(QUrl::FullyEncoded).toUtf8());
    conectarErrorReply(reply);
    return reply;
}

QNetworkReply* AdminAPI::registrarUsuario(const QString &nombre, const QString &apellido, const QString &usuario,
                                          const QString &contrasena, const QString &correo) {
    qDebug() << "[AdminAPI] registrarUsuario:" << usuario;
    QUrl url(urlBase + "/auth/registro");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("nombre", nombre);
    params.addQueryItem("apellido", apellido);
    params.addQueryItem("usuario", usuario);
    params.addQueryItem("clave", contrasena);
    params.addQueryItem("mail", correo);
    QNetworkReply* reply = networkManager.post(request, params.query(QUrl::FullyEncoded).toUtf8());
    conectarErrorReply(reply);
    return reply;
}

QNetworkReply* AdminAPI::cambiarContrasena(const QString &usuario, const QString &contrasenaActual, const QString &nuevaContrasena) {
    qDebug() << "[AdminAPI] cambiarContrasena usuario:" << usuario;
    QUrl url(urlBase + "/auth/cambiar-clave");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem("usuario", usuario);
    params.addQueryItem("clave_actual", contrasenaActual);
    params.addQueryItem("nueva_clave", nuevaContrasena);
    QNetworkReply* reply = networkManager.post(request, params.query(QUrl::FullyEncoded).toUtf8());
    conectarErrorReply(reply);
    return reply;
}

// HISTORIAL FASTAPI

void AdminAPI::leerHistorial() {
    QUrl url(urlBase + "/auth/historial");
    QUrlQuery query;
    query.addQueryItem("usuario", usuarioActual);
    url.setQuery(query);
    QNetworkRequest request(url);

    QNetworkReply* reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isArray()) {
                emit historialLeido(doc.array());
            } else {
                emit historialError("Error de formato al leer historial.");
            }
        } else {
            emit historialError(reply->errorString());
            // Solo llamar a handleNetworkError si es error de red real:
            QNetworkReply::NetworkError err = reply->error();
            if (err == QNetworkReply::HostNotFoundError ||
                err == QNetworkReply::ConnectionRefusedError ||
                err == QNetworkReply::TimeoutError ||
                err == QNetworkReply::UnknownNetworkError) {
                handleNetworkError(reply);
            }
        }
        reply->deleteLater();
    });
}

void AdminAPI::agregarHistorial(const QString& nombreArchivo, const QString& transcripcion) {
    QUrl url(urlBase + "/auth/historial");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("usuario", usuarioActual);
    params.addQueryItem("nombre_archivo", nombreArchivo);
    params.addQueryItem("transcripcion", transcripcion);

    QNetworkReply* reply = networkManager.post(request, params.query(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit historialAgregado();
        } else {
            emit historialError(reply->errorString());
            // Solo llamar a handleNetworkError si es error de red real:
            QNetworkReply::NetworkError err = reply->error();
            if (err == QNetworkReply::HostNotFoundError ||
                err == QNetworkReply::ConnectionRefusedError ||
                err == QNetworkReply::TimeoutError ||
                err == QNetworkReply::UnknownNetworkError) {
                handleNetworkError(reply);
            }
        }
        reply->deleteLater();
    });
}

void AdminAPI::borrarHistorial(int historial_id) {
    QUrl url(urlBase + "/auth/historial/" + QString::number(historial_id));
    QUrlQuery query;
    query.addQueryItem("usuario", usuarioActual);
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply* reply = networkManager.deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            emit historialBorrado();
        } else {
            emit historialError(reply->errorString());
            // Solo llamar a handleNetworkError si es error de red real:
            QNetworkReply::NetworkError err = reply->error();
            if (err == QNetworkReply::HostNotFoundError ||
                err == QNetworkReply::ConnectionRefusedError ||
                err == QNetworkReply::TimeoutError ||
                err == QNetworkReply::UnknownNetworkError) {
                handleNetworkError(reply);
            }
        }
        reply->deleteLater();
    });
}

// OpenAI Key

void AdminAPI::solicitarOpenAIKey() {
    QUrl url(urlBase + "/auth/openai-key");
    QUrlQuery query;
    query.addQueryItem("usuario", usuarioActual);
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply* reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray respuesta = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(respuesta);
            QString apiKey = doc.object().value("openai_key").toString();
            if (apiKey.isEmpty()) {
                emit openAIKeyError("No se recibió la API Key.");
            } else {
                emit openAIKeyReady(apiKey);
            }
        } else {
            emit openAIKeyError(reply->errorString());
            // Solo llamar a handleNetworkError si es error de red real:
            QNetworkReply::NetworkError err = reply->error();
            if (err == QNetworkReply::HostNotFoundError ||
                err == QNetworkReply::ConnectionRefusedError ||
                err == QNetworkReply::TimeoutError ||
                err == QNetworkReply::UnknownNetworkError) {
                handleNetworkError(reply);
            }
        }
        reply->deleteLater();
    });
}

QNetworkReply* AdminAPI::transcribirAudioConWhisper(const QString &rutaAudio, const QString &apiKey) {
    qDebug() << "[AdminAPI] transcribirAudioConWhisper archivo:" << rutaAudio;
    QUrl url("https://api.openai.com/v1/audio/transcriptions");
    QNetworkRequest request(url);

    QString authHeader = "Bearer " + apiKey;
    request.setRawHeader("Authorization", authHeader.toUtf8());

    QFile *file = new QFile(rutaAudio);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "[AdminAPI] Error: No se pudo abrir el archivo de audio:" << rutaAudio;
        emit whisperTranscripcionLista("Error: no se pudo abrir el archivo de audio.");
        delete file;
        return nullptr;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"audio.wav\""));
    audioPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    audioPart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(audioPart);

    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"model\""));
    modelPart.setBody("whisper-1");
    multiPart->append(modelPart);

    QNetworkReply *reply = networkManager.post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray respuesta = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(respuesta);
            QString texto = doc.object().value("text").toString();
            emit whisperTranscripcionLista(texto);
        } else {
            emit whisperTranscripcionLista("No se pudo transcribir el audio: " + reply->errorString());
            // Solo llamar a handleNetworkError si es error de red real:
            QNetworkReply::NetworkError err = reply->error();
            if (err == QNetworkReply::HostNotFoundError ||
                err == QNetworkReply::ConnectionRefusedError ||
                err == QNetworkReply::TimeoutError ||
                err == QNetworkReply::UnknownNetworkError) {
                handleNetworkError(reply);
            }
        }
        reply->deleteLater();
    });

    return reply;
}
