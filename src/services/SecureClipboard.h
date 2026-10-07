#pragma once

#include <QObject>
#include <QTimer>

// Panoya kopyalanan şifre belirli bir süre sonra silinir. Windows'un pano geçmişine (Win+V) ve
// cihazlar arası bulut panosuna düşmemesi için Windows'un tanıdığı "izleme dışı" işareti de eklenir.
class SecureClipboard : public QObject
{
    Q_OBJECT

public:
    explicit SecureClipboard(QObject *parent = nullptr);

    void copy(const QString &text, int clearAfterSeconds);
    void clearNow(); // kilitlenince ve çıkarken: pano hâlâ bizim kopyaladığımızı tutuyorsa silinir
    int secondsLeft() const;

signals:
    void cleared();

private:
    void clearIfOurs();

    QTimer m_timer;
    QString m_copiedHash; // panodaki metnin kendisi değil özeti tutulur
};
