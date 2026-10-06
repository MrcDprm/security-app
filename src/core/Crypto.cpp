#include "core/Crypto.h"

#include <sodium.h>

namespace Crypto {

static_assert(KEY_BYTES == crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
static_assert(SALT_BYTES == crypto_pwhash_SALTBYTES);
static_assert(NONCE_BYTES == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);

bool init()
{
    return sodium_init() >= 0;
}

KdfParams defaultParams()
{
    return {static_cast<quint32>(crypto_pwhash_OPSLIMIT_MODERATE),
            static_cast<quint32>(crypto_pwhash_MEMLIMIT_MODERATE)};
}

KdfParams fastParams()
{
    return {static_cast<quint32>(crypto_pwhash_OPSLIMIT_MIN), 8 * 1024 * 1024};
}

SecretKey::SecretKey()
    : m_data(static_cast<unsigned char *>(sodium_malloc(KEY_BYTES)))
{
}

SecretKey::~SecretKey()
{
    sodium_free(m_data); // önce sıfırlar, sonra serbest bırakır
}

QByteArray randomBytes(int count)
{
    QByteArray bytes(count, Qt::Uninitialized);
    randombytes_buf(bytes.data(), static_cast<size_t>(count));
    return bytes;
}

bool deriveKey(const QString &password, const QByteArray &salt, const KdfParams &params, SecretKey &key)
{
    if (!key.isValid() || salt.size() != SALT_BYTES)
        return false;
    QByteArray utf8 = password.toUtf8();
    const bool ok = crypto_pwhash(key.data(), KEY_BYTES, utf8.constData(), static_cast<unsigned long long>(utf8.size()),
                                  reinterpret_cast<const unsigned char *>(salt.constData()), params.ops, params.memory,
                                  crypto_pwhash_ALG_ARGON2ID13) == 0;
    wipe(utf8); // şifrenin UTF-8 kopyası bellekte kalmasın
    return ok;
}

QByteArray encrypt(const SecretKey &key, const QByteArray &plain, const QByteArray &aad, const QByteArray &nonce)
{
    QByteArray cipher(plain.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES, Qt::Uninitialized);
    unsigned long long length = 0;
    crypto_aead_xchacha20poly1305_ietf_encrypt(
        reinterpret_cast<unsigned char *>(cipher.data()), &length,
        reinterpret_cast<const unsigned char *>(plain.constData()), static_cast<unsigned long long>(plain.size()),
        reinterpret_cast<const unsigned char *>(aad.constData()), static_cast<unsigned long long>(aad.size()), nullptr,
        reinterpret_cast<const unsigned char *>(nonce.constData()), key.data());
    cipher.resize(static_cast<qsizetype>(length));
    return cipher;
}

std::optional<QByteArray> decrypt(const SecretKey &key, const QByteArray &cipher, const QByteArray &aad,
                                  const QByteArray &nonce)
{
    if (cipher.size() < crypto_aead_xchacha20poly1305_ietf_ABYTES || nonce.size() != NONCE_BYTES)
        return std::nullopt;
    QByteArray plain(cipher.size() - crypto_aead_xchacha20poly1305_ietf_ABYTES, Qt::Uninitialized);
    unsigned long long length = 0;
    // Doğrulama etiketi tutmazsa (yanlış anahtar ya da değiştirilmiş veri) -1 döner, hiçbir şey çözülmez
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            reinterpret_cast<unsigned char *>(plain.data()), &length, nullptr,
            reinterpret_cast<const unsigned char *>(cipher.constData()), static_cast<unsigned long long>(cipher.size()),
            reinterpret_cast<const unsigned char *>(aad.constData()), static_cast<unsigned long long>(aad.size()),
            reinterpret_cast<const unsigned char *>(nonce.constData()), key.data())
        != 0)
        return std::nullopt;
    plain.resize(static_cast<qsizetype>(length));
    return plain;
}

bool sameKey(const SecretKey &a, const SecretKey &b)
{
    return a.isValid() && b.isValid() && sodium_memcmp(a.data(), b.data(), KEY_BYTES) == 0;
}

void wipe(QByteArray &data)
{
    if (!data.isEmpty())
        sodium_memzero(data.data(), static_cast<size_t>(data.size()));
    data.clear();
}

} // namespace Crypto