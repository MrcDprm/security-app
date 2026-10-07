#pragma once

#include <QWidget>

class PasswordField;
class QLabel;
class QProgressBar;
class QPushButton;

// İlk açılış: yeni kasa oluşturma (ana şifre + tekrarı), var olan kasayı açma ya da demo kasayı deneme.
class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    WelcomePage(const QString &defaultPath, QWidget *parent);
    void showError(const QString &text);
    void clearFields(); // kasa açılınca ana şifre kutularda kalmasın

signals:
    void createRequested(const QString &path, const QString &password);
    void openRequested(const QString &path);
    void demoRequested();

private:
    void validate();
    void chooseLocation();

    QString m_path;
    PasswordField *m_password, *m_confirm;
    QProgressBar *m_strength;
    QLabel *m_strengthText, *m_location, *m_error;
    QPushButton *m_create;
};
