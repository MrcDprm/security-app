#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>

class QNetworkAccessManager;
class QNetworkReply;

// Have I Been Pwned "Pwned Passwords" sorgusu, k-anonimlik ile:
// şifrenin SHA-1 özeti alınır, sadece ilk 5 karakteri gönderilir; sunucu o önekle başlayan
// yüzlerce özetin sonunu döner, eşleşme bu bilgisayarda aranır. Şifre ya da tam özeti hiç gönderilmez.
class BreachCheck : public QObject
{
    Q_OBJECT

public:
    explicit BreachCheck(QObject *parent = nullptr);

    // (kayıt kimliği, şifre) çiftleri sırayla kontrol edilir
    void start(const QList<QPair<QString, QString>> &passwords);
    void cancel();
    bool isRunning() const { return m_reply != nullptr; }

    static QByteArray sha1Hex(const QString &password); // büyük harf onaltılık
    static int countIn(const QByteArray &body, const QByteArray &suffix); // yanıtta "SONEK:SAYI" satırını arar

signals:
    void progress(int done, int total);
    void found(const QString &id, int count); // count 0 ise sızıntıda yok
    void finished(bool ok);

private:
    void next();
    void handleReply();

    QNetworkAccessManager *m_network;
    QNetworkReply *m_reply = nullptr;
    QList<QPair<QString, QByteArray>> m_queue; // kimlik → SHA-1 (şifrenin kendisi kuyrukta tutulmaz)
    QHash<QByteArray, QByteArray> m_cache;     // önek → yanıt: aynı önek iki kez sorulmaz
    int m_total = 0;
    int m_done = 0;
};
