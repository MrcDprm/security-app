#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

// libsodium üzerine ince bir katman: ana şifreden anahtar türetme (Argon2id) ve
// kimliği doğrulanmış şifreleme (XChaCha20-Poly1305). Şifrelenmiş veride tek bir bit
// değişirse çözme başarısız olur; kurcalanmış dosya sessizce yanlış veri vermez.
namespace Crypto {

constexpr int KEY_BYTES = 32;
constexpr int SALT_BYTES = 16;
constexpr int NONCE_BYTES = 24;

bool init();

// Argon2id ayarları: işlem sayısı ve bellek (bayt). Dosyaya yazılır, ileride artırılabilir.
struct KdfParams {
    quint32 ops = 0;
    quint32 memory = 0;
};
KdfParams defaultParams(); // ~256 MB, bir saniyenin altında
KdfParams fastParams();    // testler için

// Anahtar, sayfa dosyasına yazılmayan (kilitli) ve korumalı bellekte durur; nesne yok olunca sıfırlanır.
// Kopyalanamaz: anahtarın bellekte birden fazla kopyası olmasın.
class SecretKey
{
public:
    SecretKey();
    ~SecretKey();
    SecretKey(const SecretKey &) = delete;
    SecretKey &operator=(const SecretKey &) = delete;

    unsigned char *data() { return m_data; }
    const unsigned char *data() const { return m_data; }
    bool isValid() const { return m_data != nullptr; }

private:
    unsigned char *m_data = nullptr;
};

QByteArray randomBytes(int count);
bool deriveKey(const QString &password, const QByteArray &salt, const KdfParams &params, SecretKey &key);

// aad: şifrelenmeyen ama değiştirilirse fark edilen ek veri (dosya başlığı)
QByteArray encrypt(const SecretKey &key, const QByteArray &plain, const QByteArray &aad, const QByteArray &nonce);
std::optional<QByteArray> decrypt(const SecretKey &key, const QByteArray &cipher, const QByteArray &aad,
                                  const QByteArray &nonce);

void wipe(QByteArray &data); // içeriği sıfırlar (derleyici bu yazmayı silemez)
bool sameKey(const SecretKey &a, const SecretKey &b); // sabit sürede karşılaştırır (zamanlama saldırısına kapalı)

} // namespace Crypto