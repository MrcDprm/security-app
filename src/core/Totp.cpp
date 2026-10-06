#include "core/Totp.h"

#include <QDateTime>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QUrlQuery>

namespace Totp {

std::optional<QByteArray> decodeBase32(const QString &text)
{
    // Base32: A-Z ve 2-7, her karakter 5 bit. Boşluk ve "=" dolgusu yok sayılır.
    static const QString alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    QByteArray bytes;
    quint32 buffer = 0;
    int bits = 0;
    for (const QChar c : text.toUpper()) {
        if (c == ' ' || c == '-' || c == '=')
            continue;
        const int value = alphabet.indexOf(c);
        if (value < 0)
            return std::nullopt;
        buffer = (buffer << 5) | static_cast<quint32>(value);
        bits += 5;
        if (bits >= 8) {
            bytes += static_cast<char>((buffer >> (bits - 8)) & 0xFF);
            bits -= 8;
        }
    }
    return bytes;
}

QString normalizeSecret(const QString &input)
{
    const QString trimmed = input.trimmed();
    // QR koddan okunan adres: otpauth://totp/Site:kullanici?secret=JBSW...&issuer=Site
    if (trimmed.startsWith("otpauth://", Qt::CaseInsensitive))
        return QUrlQuery(QUrl(trimmed)).queryItemValue("secret").toUpper();
    QString secret = trimmed.toUpper();
    secret.remove(' ');
    secret.remove('-');
    return secret;
}

bool isValidSecret(const QString &secret)
{
    const auto key = decodeBase32(secret);
    return key && key->size() >= 10 && secret.size() <= 256; // en az 80 bit (RFC 4226 önerisi 128+)
}

QString code(const QByteArray &key, qint64 unixTime, int digits, int period)
{
    // Zaman dilimi numarası 8 baytlık büyük uçlu (big-endian) sayı olarak imzalanır
    const quint64 counter = static_cast<quint64>(unixTime / period);
    QByteArray message(8, 0);
    for (int i = 7, shift = 0; i >= 0; --i, shift += 8)
        message[i] = static_cast<char>((counter >> shift) & 0xFF);
    const QByteArray hash = QMessageAuthenticationCode::hash(message, key, QCryptographicHash::Sha1);

    // Dinamik kısaltma: son baytın alt 4 biti, hash'ten okunacak 4 baytın yerini söyler
    const int offset = hash[hash.size() - 1] & 0x0F;
    const quint32 binary = (static_cast<quint32>(hash[offset] & 0x7F) << 24)
                           | (static_cast<quint32>(hash[offset + 1] & 0xFF) << 16)
                           | (static_cast<quint32>(hash[offset + 2] & 0xFF) << 8)
                           | static_cast<quint32>(hash[offset + 3] & 0xFF);
    quint32 modulo = 1;
    for (int i = 0; i < digits; ++i)
        modulo *= 10;
    return QString("%1").arg(binary % modulo, digits, 10, QChar('0'));
}

QString codeNow(const QString &secret)
{
    if (!isValidSecret(secret))
        return QString();
    return code(*decodeBase32(secret), QDateTime::currentSecsSinceEpoch());
}

int secondsLeft(qint64 unixTime, int period)
{
    return period - static_cast<int>(unixTime % period);
}

} // namespace Totp
