#include "login.h"
#include "ui_login.h"
#include "adminapi.h"
#include "interfaz.h"
#include "registrar.h"
#include "errorhandler.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

Login::Login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Login)
    , replyActual(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Login");

    ui->signupLabel->setTextFormat(Qt::RichText);
    ui->signupLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->signupLabel->setOpenExternalLinks(false);
    connect(ui->signupLabel, &QLabel::linkActivated, this, &Login::on_signupLabel_clicked);

    connect(ui->loginButton, &QPushButton::clicked, this, &Login::on_LoginButton_clicked, Qt::UniqueConnection);

    // Recordar usuario usando QSettings (TP5/AppTranscriptor)
    QSettings settings("TP5", "AppTranscriptor");
    bool recordar = settings.value("login/recordar", false).toBool();
    if (recordar) {
        QString usuario = settings.value("login/usuarioRecordado", "").toString();
        ui->usernameEdit->setText(usuario);
        ui->rememberCheckBox->setChecked(true);
    } else {
        ui->usernameEdit->clear();
        ui->rememberCheckBox->setChecked(false);
    }
    qDebug() << "[Login] Ventana de login inicializada";
}

Login::~Login()
{
    qDebug() << "[Login] Cerrando ventana Login";
    delete ui;
}

void Login::on_LoginButton_clicked()
{
    QString usuario = ui->usernameEdit->text();
    QString contrasena = ui->passwordEdit->text();

    if (usuario.isEmpty() || contrasena.isEmpty()) {
        ErrorHandler::showWarning(this, "Por favor, ingresa usuario y contraseña.");
        qDebug() << "[Login] Intento de login con campos vacíos";
        return;
    }

    QSettings settings("TP5", "AppTranscriptor");
    if (ui->rememberCheckBox->isChecked()) {
        settings.setValue("login/usuarioRecordado", usuario);
        settings.setValue("login/recordar", true);
    } else {
        settings.remove("login/usuarioRecordado");
        settings.setValue("login/recordar", false);
    }

    ui->loginButton->setEnabled(false);

    if (replyActual) {
        disconnect(replyActual, nullptr, this, nullptr);
        replyActual->deleteLater();
    }
    usuarioActual = usuario;

    qDebug() << "[Login] Solicitando login para usuario:" << usuario;
    replyActual = AdminAPI::getInstancia()->iniciarSesion(usuario, contrasena);
    connect(replyActual, &QNetworkReply::finished, this, &Login::onLoginReply, Qt::UniqueConnection);
}

void Login::onLoginReply()
{
    ui->loginButton->setEnabled(true);

    if (!replyActual)
        return;

    QByteArray respuesta = replyActual->readAll();

    if (replyActual->error() == QNetworkReply::NoError) {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(respuesta, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            ErrorHandler::showWarning(this, "Respuesta del servidor no válida.");
            qWarning() << "[Login] Error de parseo JSON:" << parseError.errorString();
        } else {
            QJsonObject obj = jsonDoc.object();
            QString token = obj.value("access_token").toString();
            if (!token.isEmpty()) {
                qDebug() << "[Login] Login exitoso para usuario:" << usuarioActual;
                AdminAPI::getInstancia()->usuarioActual = usuarioActual;
                Interfaz *ventanaInterfaz = new Interfaz(nullptr, usuarioActual, token);
                ventanaInterfaz->show();
                this->close();
            } else if (obj.contains("detail")) {
                ErrorHandler::showWarning(this, obj.value("detail").toString());
                qWarning() << "[Login] Error del backend:" << obj.value("detail").toString();
            } else {
                ErrorHandler::showWarning(this, "Respuesta inesperada del servidor (sin token).");
                qWarning() << "[Login] Login: respuesta inesperada (sin token)";
            }
        }
    } else {
        // SOLO mensaje de warning propio, SIN llamar a handleNetworkError
        ErrorHandler::showWarning(this, "Usuario o contraseña incorrectos o error de conexión.");
        qWarning() << "[Login] Error de login para usuario:" << usuarioActual
                   << ", error:" << replyActual->errorString();
    }
    replyActual->deleteLater();
    replyActual = nullptr;
}

void Login::on_signupLabel_clicked()
{
    qDebug() << "[Login] Navegando a ventana de registro";
    Registrar *ventanaRegistrar = new Registrar();
    ventanaRegistrar->show();
    this->close();
}
