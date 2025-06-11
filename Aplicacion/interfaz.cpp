#include "interfaz.h"
#include "ui_interfaz.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QPdfWriter>
#include <QPainter>
#include <QDir>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "grabaraudio.h"
#include "login.h"
#include "adminapi.h"
#include "errorhandler.h"
#include <QtGlobal>

Interfaz::Interfaz(QWidget *parent, const QString& usuarioActual, const QString& tokenJWT)
    : QWidget(parent)
    , ui(new Ui::Interfaz)
    , usuario(usuarioActual)
    , token(tokenJWT)
    , panelUsuario(nullptr)
    , transcripcionesModel(new QStandardItemModel(this))
    , grabando(false)
    , grabarAudioPtr(nullptr)
    , searchHistorial(nullptr)
    , reproductorAudio(nullptr)
    , audioOutput(nullptr)
    , usuarioMoviendoSlider(false)
{
    ui->setupUi(this);
    ui->transcriptionEdit->setAcceptDrops(false);

    // --- PETICIÓN ASÍNCRONA DE API KEY AL BACKEND ---
    OPENAI_API_KEY.clear();
    connect(AdminAPI::getInstancia(), &AdminAPI::openAIKeyReady, this, [this](const QString& apiKey){
        OPENAI_API_KEY = apiKey;
    });
    connect(AdminAPI::getInstancia(), &AdminAPI::openAIKeyError, this, [this](const QString& err){
        ErrorHandler::showError(this, "Error obteniendo API Key: " + err);
    });
    AdminAPI::getInstancia()->solicitarOpenAIKey();

    // --- HISTORIAL FASTAPI ---
    connect(AdminAPI::getInstancia(), &AdminAPI::historialLeido, this, [this](const QJsonArray& array){
        historialTranscripciones.clear();
        for (const QJsonValue& v : array) {
            const QJsonObject obj = v.toObject();
            HistorialItem item;
            item.id = obj.value("id").toInt();
            item.nombreArchivo = obj.value("nombre_archivo").toString();
            item.transcripcion = obj.value("transcripcion").toString();
            historialTranscripciones.append(item);
        }
        actualizarListaHistorial(searchHistorial ? searchHistorial->text() : QString());
    });
    connect(AdminAPI::getInstancia(), &AdminAPI::historialError, this, [this](const QString& msg){
        ErrorHandler::showError(this, "Historial: " + msg);
    });
    connect(AdminAPI::getInstancia(), &AdminAPI::historialAgregado, this, [this](){
        AdminAPI::getInstancia()->leerHistorial();
        ErrorHandler::showInfo(this, "Transcripción guardada en el historial y backend.");
    });
    connect(AdminAPI::getInstancia(), &AdminAPI::historialBorrado, this, [this](){
        AdminAPI::getInstancia()->leerHistorial();
        ErrorHandler::showInfo(this, "Transcripción eliminada.");
    });

    // Búsqueda en historial
    QVBoxLayout* leftLayout = qobject_cast<QVBoxLayout*>(ui->verticalWidget->layout());
    if (leftLayout) {
        searchHistorial = new QLineEdit(ui->verticalWidget);
        searchHistorial->setPlaceholderText("Buscar en historial...");
        searchHistorial->setObjectName("searchHistorial");
        leftLayout->insertWidget(1, searchHistorial);
        connect(searchHistorial, &QLineEdit::textChanged, this, &Interfaz::on_searchHistorial_textChanged);
    }

    ui->listView->setModel(transcripcionesModel);
    connect(ui->listView, &QListView::clicked, this, &Interfaz::on_historialTranscripcion_clicked);

    connect(AdminAPI::getInstancia(), &AdminAPI::whisperTranscripcionLista, this, [this](const QString &texto){
        ui->transcriptionEdit->setPlainText(texto);
    });

    // Panel de usuario modal
    panelUsuario = new QWidget(this, Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    panelUsuario->setWindowModality(Qt::WindowModal);
    panelUsuario->setWindowTitle("Usuario");
    panelUsuario->setFixedSize(270, 230);
    QVBoxLayout* layout = new QVBoxLayout(panelUsuario);

    QLabel* iconLabel = new QLabel(panelUsuario);
    iconLabel->setText("👤");
    iconLabel->setStyleSheet("font-size: 54px; color: #6EA5D0; margin-top: 14px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    userLabel = new QLabel(usuario, panelUsuario);
    userLabel->setStyleSheet("font-weight: bold; font-size: 20px; color: #EEEEEE; margin-bottom: 18px;");
    userLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(userLabel);

    QLabel* cambiarClaveLabel = new QLabel(panelUsuario);
    cambiarClaveLabel->setText("<a href=\"#\">Cambiar contraseña</a>");
    cambiarClaveLabel->setTextFormat(Qt::RichText);
    cambiarClaveLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    cambiarClaveLabel->setOpenExternalLinks(false);
    cambiarClaveLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(cambiarClaveLabel);
    layout->addStretch(1);

    QHBoxLayout* botonesLayout = new QHBoxLayout();
    QPushButton* btnAtras = new QPushButton("Atrás", panelUsuario);
    QPushButton* logoutButton = new QPushButton("Cerrar sesión", panelUsuario);
    btnAtras->setFixedWidth(90);
    logoutButton->setFixedWidth(160);
    botonesLayout->addWidget(btnAtras);
    botonesLayout->addSpacing(14);
    botonesLayout->addWidget(logoutButton);
    botonesLayout->setAlignment(Qt::AlignCenter);
    layout->addLayout(botonesLayout);

    connect(btnAtras, &QPushButton::clicked, this, &Interfaz::on_usuarioAtras_clicked);
    connect(logoutButton, &QPushButton::clicked, this, &Interfaz::on_logoutButton_clicked);
    connect(cambiarClaveLabel, &QLabel::linkActivated, this, &Interfaz::on_cambiarClave_clicked);

    panelUsuario->setLayout(layout);
    panelUsuario->hide();

    setAcceptDrops(true);

    // --- Reproductor de audio ---
    reproductorAudio = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(1.0);
    reproductorAudio->setAudioOutput(audioOutput);

    connect(ui->barraProgreso, &QSlider::sliderMoved, this, &Interfaz::on_barraProgreso_sliderMoved);
    connect(reproductorAudio, &QMediaPlayer::positionChanged, this, &Interfaz::on_reproductor_positionChanged);
    connect(reproductorAudio, &QMediaPlayer::durationChanged, this, &Interfaz::on_reproductor_durationChanged);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(reproductorAudio, &QMediaPlayer::playbackStateChanged, this, &Interfaz::on_reproductor_stateChanged);
#else
    connect(reproductorAudio, QOverload<QMediaPlayer::State>::of(&QMediaPlayer::stateChanged), this, &Interfaz::on_reproductor_stateChanged);
#endif

    ui->btnPlayPause->setEnabled(false);
    ui->barraProgreso->setEnabled(false);
    usuarioMoviendoSlider = false;

    // Leer historial al iniciar
    AdminAPI::getInstancia()->leerHistorial();
}

Interfaz::~Interfaz()
{
    qDebug() << "[Interfaz] Cerrando interfaz";
    if (panelUsuario) {
        panelUsuario->deleteLater();
    }
    delete ui;
}

void Interfaz::setUsuario(const QString& usuarioNuevo)
{
    usuario = usuarioNuevo;
    if (userLabel) {
        userLabel->setText(usuario);
    }
    qDebug() << "[Interfaz] Usuario actualizado a:" << usuario;
    AdminAPI::getInstancia()->leerHistorial();
}

void Interfaz::agregarAlHistorial(const QString& texto)
{
    if (texto.trimmed().isEmpty()) return;
}

void Interfaz::actualizarListaHistorial(const QString& filtro)
{
    transcripcionesModel->clear();
    for (const HistorialItem& item : historialTranscripciones) {
        if (filtro.isEmpty() || item.nombreArchivo.contains(filtro, Qt::CaseInsensitive)) {
            transcripcionesModel->appendRow(new QStandardItem(item.nombreArchivo));
        }
    }
}

void Interfaz::on_historialTranscripcion_clicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    int count = 0;
    int realIdx = -1;
    QString filtro = searchHistorial ? searchHistorial->text() : QString();
    for (int i = 0; i < historialTranscripciones.size(); ++i) {
        if (filtro.isEmpty() || historialTranscripciones[i].nombreArchivo.contains(filtro, Qt::CaseInsensitive)) {
            if (count == row) {
                realIdx = i;
                break;
            }
            ++count;
        }
    }
    if (realIdx < 0 || realIdx >= historialTranscripciones.size()) return;

    HistorialItem item = historialTranscripciones[realIdx];

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(item.nombreArchivo);
    msgBox.setText("¿Qué deseas hacer?");
    msgBox.setInformativeText(item.transcripcion.left(512) + (item.transcripcion.size()>512 ? "..." : ""));

    QPushButton *btnVer = msgBox.addButton("Ver", QMessageBox::ActionRole);
    QPushButton *btnExportar = msgBox.addButton("Exportar", QMessageBox::ActionRole);
    QPushButton *btnEliminar = msgBox.addButton("Eliminar", QMessageBox::DestructiveRole);
    QPushButton *btnAtras = msgBox.addButton("Atrás", QMessageBox::RejectRole);

    msgBox.setDefaultButton(btnAtras);
    msgBox.exec();

    if (msgBox.clickedButton() == btnVer) {
        mostrarTranscripcionCompleta(item.nombreArchivo, item.transcripcion);
    } else if (msgBox.clickedButton() == btnExportar) {
        ui->transcriptionEdit->setPlainText(item.transcripcion);
        on_exportButton_clicked();
    } else if (msgBox.clickedButton() == btnEliminar) {
        AdminAPI::getInstancia()->borrarHistorial(item.id);
    }
}

void Interfaz::mostrarTranscripcionCompleta(const QString& titulo, const QString& texto) {
    QDialog dialog(this);
    dialog.setWindowTitle(titulo);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QPlainTextEdit* edit = new QPlainTextEdit(&dialog);
    edit->setPlainText(texto);
    edit->setReadOnly(true);
    edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QPushButton* btnCerrar = new QPushButton("Cerrar", &dialog);
    connect(btnCerrar, &QPushButton::clicked, &dialog, &QDialog::accept);

    layout->addWidget(edit);
    layout->addWidget(btnCerrar);

    dialog.setLayout(layout);
    dialog.exec();
}


void Interfaz::on_saveTranscriptionButton_clicked()
{
    QString texto = ui->transcriptionEdit->toPlainText().trimmed();
    if (texto.isEmpty()) {
        ErrorHandler::showWarning(this, "No hay transcripción para guardar.");
        return;
    }

    bool ok = false;
    QString nombre = QInputDialog::getText(this, tr("Guardar transcripción"),
                                           tr("Poné un nombre para la transcripción:"), QLineEdit::Normal,
                                           "", &ok);
    if (!ok || nombre.trimmed().isEmpty()) {
        ErrorHandler::showInfo(this, "No se guardó la transcripción (no se ingresó nombre).");
        return;
    }
    AdminAPI::getInstancia()->agregarHistorial(nombre.trimmed(), texto);
}

void Interfaz::on_exportButton_clicked()
{
    QMessageBox exportBox(this);
    exportBox.setWindowTitle("Exportar transcripción");
    exportBox.setText("Elegí el formato de exportación para la transcripción actual:");

    QPushButton* btnTxt = exportBox.addButton("📄 TXT", QMessageBox::ActionRole);
    QPushButton* btnWord = exportBox.addButton("📃 WORD", QMessageBox::ActionRole);
    QPushButton* btnPdf = exportBox.addButton("📕 PDF", QMessageBox::ActionRole);
    QPushButton* btnCancelar = exportBox.addButton("Cancelar", QMessageBox::RejectRole);

    exportBox.setDefaultButton(btnCancelar);
    exportBox.exec();

    if (exportBox.clickedButton() == btnTxt) {
        on_exportTxtButton_clicked();
    } else if (exportBox.clickedButton() == btnWord) {
        on_exportWordButton_clicked();
    } else if (exportBox.clickedButton() == btnPdf) {
        on_exportPdfButton_clicked();
    }
}

void Interfaz::on_microButton_clicked()
{
    if (!grabando) {
        grabando = true;
        ui->microButton->setText("⏹️");
        ui->microButton->setToolTip("Detener grabación");

        // Quita el verde de ambos botones (archivo y micro)
        ui->loadButton->setStyleSheet("");
        // Micrófono NO verde al grabar
        ui->microButton->setStyleSheet("font-size: 20px; min-width: 32px; min-height: 32px; max-width: 40px; max-height: 40px; border-radius: 20px; background-color: transparent;");

        qDebug() << "[Interfaz] Iniciando grabación de audio";
        grabarAudioPtr = new GrabarAudio(this);
        connect(grabarAudioPtr, &GrabarAudio::audioGrabado, this, &Interfaz::audioGrabado);
        grabarAudioPtr->startGrabacion();
    } else {
        grabando = false;
        ui->microButton->setText("🎤");
        ui->microButton->setToolTip("Grabar Audio");

        // Al terminar la grabación, vuelve al color normal (el verde sólo si audio se envía)
        ui->microButton->setStyleSheet("font-size: 20px; min-width: 32px; min-height: 32px; max-width: 40px; max-height: 40px; border-radius: 20px; background-color: transparent;");

        qDebug() << "[Interfaz] Deteniendo grabación de audio";
        if (grabarAudioPtr) grabarAudioPtr->stopGrabacion();
    }
}

void Interfaz::audioGrabado(QString archivo)
{
    rutaAudioCargado = archivo;

    bool ok = false;
    QString nombre = QInputDialog::getText(this, tr("Guardar grabación"),
                                           tr("Nombre del audio:"), QLineEdit::Normal,
                                           "", &ok);
    if (ok && !nombre.trimmed().isEmpty()) {
        QFileInfo info(archivo);
        QString nuevoPath = info.absolutePath() + "/" + nombre + "." + info.suffix();
        if (QFile::rename(archivo, nuevoPath)) {
            rutaAudioCargado = nuevoPath;
            qDebug() << "[Interfaz] Archivo de audio renombrado a:" << nuevoPath;
        }

        ui->transcriptionEdit->setPlainText("Transcribiendo audio, por favor espere...");
        AdminAPI::getInstancia()->transcribirAudioConWhisper(rutaAudioCargado, OPENAI_API_KEY);

        ErrorHandler::showInfo(this, "¡Audio enviado y transcribiendo!");

        prepararReproductorParaAudio(rutaAudioCargado);
        ui->microButton->setStyleSheet("font-size: 20px; min-width: 32px; min-height: 32px; max-width: 40px; max-height: 40px; border-radius: 20px; background-color: #4CAF50; color: white;");
    } else {
        QFile::remove(archivo);
        rutaAudioCargado.clear();
        ErrorHandler::showInfo(this, "Grabación descartada.");
        qDebug() << "[Interfaz] Grabación de audio descartada";
        ui->microButton->setStyleSheet("font-size: 20px; min-width: 32px; min-height: 32px; max-width: 40px; max-height: 40px; border-radius: 20px; background-color: transparent;");
    }

    if (grabarAudioPtr) {
        grabarAudioPtr->deleteLater();
        grabarAudioPtr = nullptr;
    }
}

void Interfaz::on_loadButton_clicked()
{
    QString archivo = QFileDialog::getOpenFileName(this, "Seleccionar archivo de audio", "", "Audio (*.wav *.mp3)");
    if (!archivo.isEmpty()) {
        QFileInfo info(archivo);
        ErrorHandler::showInfo(this, "Archivo (" + info.fileName() + ") subido con éxito");

        // Cargar archivo verde, microfono NO
        ui->loadButton->setStyleSheet("background-color: #4CAF50; color: white;");
        ui->microButton->setStyleSheet("font-size: 20px; min-width: 32px; min-height: 32px; max-width: 40px; max-height: 40px; border-radius: 20px; background-color: transparent;");

        this->rutaAudioCargado = archivo;

        ui->transcriptionEdit->setPlainText("Transcribiendo audio, por favor espere...");
        AdminAPI::getInstancia()->transcribirAudioConWhisper(archivo, OPENAI_API_KEY);

        prepararReproductorParaAudio(archivo);

        qDebug() << "[Interfaz] Archivo de audio cargado y enviado a transcripción";
    }
}

void Interfaz::on_exportTxtButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Guardar como TXT", "", "Archivo TXT (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << ui->transcriptionEdit->toPlainText();
            file.close();
            ErrorHandler::showInfo(this, "Transcripción exportada a TXT.");
            qDebug() << "[Interfaz] Transcripción exportada como TXT:" << fileName;
        }
    }
}

void Interfaz::on_exportWordButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Guardar como Word", "", "Archivo Word (*.doc)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString html = ui->transcriptionEdit->toPlainText().replace("\n", "<br>");
            QTextStream out(&file);
            out << "<html><body>" << html << "</body></html>";
            file.close();
            ErrorHandler::showInfo(this, "Transcripción exportada a Word (.doc). Si quieres .docx real, usa una suite ofimática para convertir.");
            qDebug() << "[Interfaz] Transcripción exportada como Word:" << fileName;
        }
    }
}

void Interfaz::on_exportPdfButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Guardar como PDF", "", "Archivo PDF (*.pdf)");
    if (!fileName.isEmpty()) {
        QPdfWriter writer(fileName);
        writer.setPageMargins(QMarginsF(30, 30, 30, 30));
        QPainter painter(&writer);
        QRect rect(0, 0, writer.width(), writer.height());
        painter.drawText(rect, Qt::AlignLeft | Qt::TextWordWrap, ui->transcriptionEdit->toPlainText());
        painter.end();
        ErrorHandler::showInfo(this, "Transcripción exportada a PDF.");
        qDebug() << "[Interfaz] Transcripción exportada como PDF:" << fileName;
    }
}

void Interfaz::on_toolButton_clicked()
{
    if (userLabel) userLabel->setText(usuario);
    panelUsuario->show();
    panelUsuario->raise();
    qDebug() << "[Interfaz] Panel de usuario mostrado";
}

void Interfaz::on_usuarioAtras_clicked()
{
    if (panelUsuario) panelUsuario->hide();
    qDebug() << "[Interfaz] Panel de usuario ocultado";
}

void Interfaz::on_logoutButton_clicked()
{
    if (panelUsuario) panelUsuario->close();
    this->close();
    Login* ventanaLogin = new Login();
    ventanaLogin->show();
    qDebug() << "[Interfaz] Sesión cerrada, volviendo a pantalla de login";
}

void Interfaz::recargarConfiguracionBackend()
{
    AdminAPI::getInstancia()->reloadUrlBase();
    ErrorHandler::showInfo(this, "Se recargó la URL base del backend desde el archivo backend_url.conf");
    qDebug() << "[Interfaz] Configuración del backend recargada por el usuario";
}

void Interfaz::on_searchHistorial_textChanged(const QString &text)
{
    actualizarListaHistorial(text);
}

// reproductor de audio
void Interfaz::on_btnPlayPause_clicked()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (reproductorAudio->playbackState() == QMediaPlayer::PlayingState)
        reproductorAudio->pause();
    else
        reproductorAudio->play();
#else
    if (reproductorAudio->state() == QMediaPlayer::PlayingState)
        reproductorAudio->pause();
    else
        reproductorAudio->play();
#endif
}

void Interfaz::on_reproductor_stateChanged(QMediaPlayer::PlaybackState state)
{
    ui->btnPlayPause->setText(
        state == QMediaPlayer::PlayingState ? "⏸️" : "▶️"
        );
}

void Interfaz::on_barraProgreso_sliderMoved(int position)
{
    usuarioMoviendoSlider = true;
    reproductorAudio->setPosition(position);
    usuarioMoviendoSlider = false;
}

void Interfaz::on_reproductor_positionChanged(qint64 position)
{
    if (!usuarioMoviendoSlider) {
        ui->barraProgreso->setValue(static_cast<int>(position));
    }
}

void Interfaz::on_reproductor_durationChanged(qint64 duration)
{
    ui->barraProgreso->setMaximum(static_cast<int>(duration));
    ui->barraProgreso->setEnabled(duration > 0);
}

// LLAMA a esto para cargar un audio y habilitar el reproductor
void Interfaz::prepararReproductorParaAudio(const QString& rutaAudio)
{
    if (!QFile::exists(rutaAudio)) {
        reproductorAudio->stop();
        ui->btnPlayPause->setEnabled(false);
        ui->barraProgreso->setEnabled(false);
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    reproductorAudio->setSource(QUrl::fromLocalFile(rutaAudio));
#else
    reproductorAudio->setMedia(QUrl::fromLocalFile(rutaAudio));
#endif
    ui->btnPlayPause->setEnabled(true);
    ui->barraProgreso->setEnabled(true);
    reproductorAudio->pause();
    ui->barraProgreso->setValue(0);
}

void Interfaz::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void Interfaz::dropEvent(QDropEvent* event) {
    for (const QUrl &url : event->mimeData()->urls()) {
        QString filePath = url.toLocalFile();
        QFileInfo info(filePath);
        QString ext = info.suffix().toLower();

        if (ext == "wav" || ext == "mp3") {
            // Mensaje de confirmación con el nombre del archivo
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Confirmar archivo");
            msgBox.setText(QString("¿Deseas transcribir el archivo?\n\n%1").arg(info.fileName()));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Ok);

            int ret = msgBox.exec();
            if (ret == QMessageBox::Ok) {
                this->rutaAudioCargado = filePath;
                ui->transcriptionEdit->setPlainText("Transcribiendo audio, por favor espere...");
                AdminAPI::getInstancia()->transcribirAudioConWhisper(filePath, OPENAI_API_KEY);
                prepararReproductorParaAudio(filePath);
                ErrorHandler::showInfo(this, QString("Archivo %1 cargado y enviado a transcribir.").arg(info.fileName()));
            } else {
                ErrorHandler::showInfo(this, "Archivo descartado.");
            }
            break; // Solo uno por vez
        } else {
            ErrorHandler::showWarning(this, "Solo se permiten archivos .wav o .mp3.");
        }
    }
}

void Interfaz::on_cambiarClave_clicked()
{
    // Pide contraseña actual
    bool ok1 = false, ok2 = false;
    QString claveActual = QInputDialog::getText(this, "Cambiar contraseña",
                                                "Ingrese su contraseña actual:", QLineEdit::Password, "", &ok1);
    if (!ok1 || claveActual.isEmpty()) return;

    // Pide nueva contraseña
    QString nuevaClave = QInputDialog::getText(this, "Cambiar contraseña",
                                               "Ingrese su nueva contraseña:", QLineEdit::Password, "", &ok2);
    if (!ok2 || nuevaClave.isEmpty()) return;

    QNetworkReply* reply = AdminAPI::getInstancia()->cambiarContrasena(
        usuario, claveActual, nuevaClave
        );
    connect(reply, &QNetworkReply::finished, this, &Interfaz::on_cambiarClaveReply);
}

void Interfaz::on_cambiarClaveReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QByteArray respuesta = reply->readAll();
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(respuesta);
        QString mensaje = jsonDoc.object().value("mensaje").toString();
        ErrorHandler::showInfo(this, mensaje.isEmpty() ? "Contraseña actualizada correctamente." : mensaje);
    } else {
        ErrorHandler::showError(this, "No se pudo cambiar la contraseña: " + reply->errorString());
    }
    reply->deleteLater();
}
