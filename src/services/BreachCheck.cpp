#include "services/BreachCheck.h"

#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

// Adres sabittir; kullanıcıdan ya da dosyadan gelen bir adrese istek atılmaz
const QString API = "https://api.pwnedpasswords.com/range/";
constexpr int TIMEOUT_MS = 15000;
constexpr qint64 MAX_RESPONSE = 2 * 1024 * 1024;

} // namespace

BreachCheck::BreachCheck(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

QByteArray BreachCheck::sha1Hex(const QString &password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();
}

int BreachCheck::countIn(const QByteArray &body, const QByteArray &suffix)
{
    // Her satır "35 karakterlik sonek:sayı". Dolgu satırlarının sayısı 0'dır (Add-Padding).
    for (const QByteArray &line : body.split('\n')) {
        const QByteArray trimmed = line.trimmed();
        const qsizetype colon = trimmed.indexOf(':');
        if (colon > 0 && trimmed.left(colon).toUpper() == suffix)
            return trimmed.mid(colon + 1).toInt();
    }
    return 0;
}

void BreachCheck::start(const QList<QPair<QString, QString>> &passwords)
{
    cancel();
    m_queue.clear();
    for (const auto &[id, password] : passwords)
        if (!password.isEmpty())
            m_queue << qMakePair(id, sha1Hex(password));
    m_total = static_cast<int>(m_queue.size());
    m_done = 0;
    next();
}

void BreachCheck::cancel()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_queue.clear();
}

void BreachCheck::next()
{
    // Önbellekte yanıtı olanlar ağa çıkmadan sonuçlanır
    while (!m_queue.isEmpty() && m_cache.contains(m_queue.first().second.left(5))) {
        const auto [id, hash] = m_queue.takeFirst();
        emit found(id, countIn(m_cache.value(hash.left(5)), hash.mid(5)));
        emit progress(++m_done, m_total);
    }
    if (m_queue.isEmpty()) {
        emit finished(true);
        return;
    }
    QNetworkRequest request(QUrl(API + QString::fromLatin1(m_queue.first().second.left(5))));
    request.setRawHeader("User-Agent", "PasswordVault-desktop");
    request.setRawHeader("Add-Padding", "true"); // yanıt boyutundan önek tahmin edilemesin
    request.setTransferTimeout(TIMEOUT_MS);
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &BreachCheck::handleReply);
}

void BreachCheck::handleReply()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError || reply->size() > MAX_RESPONSE) {
        m_queue.clear();
        emit finished(false); // ağ yok, zaman aşımı ya da beklenmeyen yanıt
        return;
    }
    m_cache.insert(m_queue.first().second.left(5), reply->read(MAX_RESPONSE));
    next();
}
