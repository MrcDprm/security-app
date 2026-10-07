#pragma once

#include <QTimer>
#include <QWidget>

class PasswordField;
class QLabel;
class QPushButton;

// Kilitli kasa: ana şifreyle açılır. Art arda yanlış denemelerde bekleme süresi artar
// (3. yanlıştan sonra 5 sn, sonra 10, 20… en fazla 60 sn).
class UnlockPage : public QWidget
{
    Q_OBJECT

public:
    explicit UnlockPage(QWidget *parent);
    void setVault(const QString &path);
    void unlockFailed(const QString &message);
    void reset();

signals:
    void unlockRequested(const QString &password);
    void otherVaultRequested();
    void newVaultRequested();

private:
    void submit();
    void tick();

    PasswordField *m_password;
    QLabel *m_file, *m_error;
    QPushButton *m_unlock;
    QTimer m_wait;
    int m_failures = 0;
    int m_waitSeconds = 0;
};
