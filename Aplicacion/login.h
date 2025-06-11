#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QNetworkReply>

namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

private slots:
    void on_LoginButton_clicked();
    void onLoginReply();
    void on_signupLabel_clicked();

private:
    Ui::Login *ui;
    QNetworkReply *replyActual;
    QString usuarioActual;
};

#endif // LOGIN_H
