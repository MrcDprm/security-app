#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

// İki adımlı doğrulama kodları (TOTP, RFC 6238): Google Authenticator'ın ürettiği 6 haneli kodun aynısı.
// Kod = HMAC-SHA1(gizli anahtar, 30 saniyelik zaman dilimi numarası) → 6 haneye kısaltılır.
namespace Totp {

constexpr int PERIOD = 30;
constexpr int DIGITS = 6;

std::optional<QByteArray> decodeBase32(const QString &text);
QString normalizeSecret(const QString &input); // "otpauth://..." adresi ya da boşluklu anahtar → düz Base32
bool isValidSecret(const QString &secret);
QString code(const QByteArray &key, qint64 unixTime, int digits = DIGITS, int period = PERIOD);
QString codeNow(const QString &secret); // geçersiz anahtarda boş
int secondsLeft(qint64 unixTime, int period = PERIOD);

} // namespace Totp
