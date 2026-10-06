#pragma once

#include "core/Crypto.h"
#include "core/Entry.h"

#include <optional>

// Kasa dosyasının biçimi:
//   "PVLT" | sürüm (1 bayt) | Argon2 işlem (4) | Argon2 bellek (4) | tuz (16) | nonce (24) | şifreli içerik
// Başlığın tamamı şifrelemeye "ek veri" (AAD) olarak katılır: başlıkta tek bayt değişse dosya açılmaz.
// Şifreli içerik, kayıtların JSON hâlidir.
namespace VaultFile {

constexpr quint8 VERSION = 1;
constexpr int HEADER_BYTES = 4 + 1 + 4 + 4 + Crypto::SALT_BYTES + Crypto::NONCE_BYTES;
constexpr qint64 MAX_FILE_BYTES = 64 * 1024 * 1024;

struct Header {
    Crypto::KdfParams params;
    QByteArray salt;
    QByteArray nonce;
};

QByteArray headerBytes(const Header &header);
// Hata anahtarı döner ("not_a_vault", "unsupported_version"); boşsa başlık geçerli
QString parseHeader(const QByteArray &file, Header &header);

QByteArray toJson(const Vault &vault);
std::optional<Vault> fromJson(const QByteArray &json); // her alan doğrulanır; bozuk yapı reddedilir

} // namespace VaultFile
