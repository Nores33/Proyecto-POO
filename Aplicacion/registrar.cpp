#include "registrar.h"
#include "ui_registrar.h"
#include "adminapi.h"
#include "login.h"
#include "errorhandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QRegularExpression>

Registrar::Registrar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Registrar)
{
    ui->setupUi(this);
    setWindowTitle("Registro de Usuario");
    qDebug() << "[Registrar] Ventana de registro inicializada";
}

Registrar::~Registrar()
{
    qDebug() << "[Registrar] Cerrando ventana de registro";
    delete ui;
}

void Registrar::on_signupButton_clicked()
{
    QString nombre = ui->nameEdit->text();
    QString apellido = ui->surnameEdit->text();
    QString usuario = ui->usernameEdit->text();
    QString contrasena = ui->passwordEdit->text();
    QString correo = ui->mailEdit->text();

    if (nombre.isEmpty() || apellido.isEmpty() || usuario.isEmpty() || contrasena.isEmpty() || correo.isEmpty()) {
        ErrorHandler::showWarning(this, "Por favor, completa todos los campos.");
        qDebug() << "[Registrar] Intento de registro con campos vacíos";
        return;
    }

    // Validación de email
    QRegularExpression emailRegex(R"((^[\w\.-]+@[\w\.-]+\.\w{2,4}$))");
    if (!emailRegex.match(correo).hasMatch()) {
        ErrorHandler::showWarning(this, "El correo electrónico no es válido.");
        return;
    }

    // Validación de contraseña (mínimo 8, mayúscula, minúscula, número)
    QRegularExpression passRegex(R"((?=.*[a-z])(?=.*[A-Z])(?=.*\d).{8,})");
    if (!passRegex.match(contrasena).hasMatch()) {
        ErrorHandler::showWarning(this, "La contraseña debe tener al menos 8 caracteres, mayúscula, minúscula y un número.");
        return;
    }

    ui->signupButton->setEnabled(false);

    if (replyActual) {
        disconnect(replyActual, nullptr, this, nullptr);
        replyActual->deleteLater();
    }
    qDebug() << "[Registrar] Solicitando registro para usuario:" << usuario;
    replyActual = AdminAPI::getInstancia()->registrarUsuario(nombre, apellido, usuario, contrasena, correo);
    connect(replyActual, &QNetworkReply::finished, this, &Registrar::onRegisterReply);
}

void Registrar::onRegisterReply()
{
    ui->signupButton->setEnabled(true);

    if (!replyActual)
        return;

    QByteArray respuesta = replyActual->readAll();

    if (replyActual->error() == QNetworkReply::NoError) {
        ErrorHandler::showInfo(this, "¡Usuario registrado correctamente!");
        qDebug() << "[Registrar] Registro exitoso";
        Login *ventanaLogin = new Login();
        ventanaLogin->show();
        this->close();
    } else {
        // SOLO mensaje de warning propio, SIN llamar a handleNetworkError
        QJsonDocument jsonDoc = QJsonDocument::fromJson(respuesta);
        QString msg = "No se pudo registrar el usuario.";
        if (!jsonDoc.isNull() && jsonDoc.isObject() && jsonDoc.object().contains("detail")) {
            msg = jsonDoc.object().value("detail").toVariant().toString();
        }
        ErrorHandler::showWarning(this, msg);
        qWarning() << "[Registrar] Error de registro:" << msg;
    }
    replyActual->deleteLater();
    replyActual = nullptr;
}

void Registrar::on_cancelButton_clicked()
{
    qDebug() << "[Registrar] Cancelar registro, volviendo a login";
    Login *ventanaLogin = new Login();
    ventanaLogin->show();
    this->close();
}
