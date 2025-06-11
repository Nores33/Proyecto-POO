#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QMessageBox>
#include <QDebug>
#include <QWidget>

class ErrorHandler {
public:
    // Muestra error crítico y lo loguea
    static void showError(QWidget* parent, const QString& msg) {
        QMessageBox::critical(parent, "Error", msg);
        qWarning() << "[ERROR]" << msg;
    }
    // Muestra advertencia y lo loguea
    static void showWarning(QWidget* parent, const QString& msg) {
        QMessageBox::warning(parent, "Advertencia", msg);
        qWarning() << "[WARN]" << msg;
    }
    // Muestra información y lo loguea
    static void showInfo(QWidget* parent, const QString& msg) {
        QMessageBox::information(parent, "Información", msg);
        qDebug() << "[INFO]" << msg;
    }
};

#endif // ERRORHANDLER_H2
