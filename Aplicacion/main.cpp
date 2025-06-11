#include <QApplication>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include "login.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Carpeta donde está el ejecutable
    QString exeDir = QCoreApplication::applicationDirPath();
    // Sube tres niveles para buscar los archivos de configuración y estilos
    QDir projectDir(exeDir);
    projectDir.cdUp();projectDir.cdUp();projectDir.cdUp();
    QString stylePath = projectDir.absoluteFilePath("style.qss");

    QFile styleFile(stylePath);
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        a.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "[main] Archivo de estilos cargado de:" << stylePath;
    } else {
        qDebug() << "[main] No se pudo cargar el archivo de estilos en:" << stylePath;
    }

    // Muestra primero el login
    Login loginWindow;
    loginWindow.show();
    qDebug() << "[main] Mostrando ventana de login";

    return a.exec();
}
