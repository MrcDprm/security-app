#pragma once

#include "core/Crypto.h"
#include "core/Entry.h"
#include "core/VaultFile.h"

#include <memory>

// Açık kasa: dosya yolu, bellekteki anahtar ve kayıtlar. Kilitlenince anahtar ve kayıtlar silinir.
// Bütün işlemler hata anahtarı döndürür ("wrong_password"); boş dönüş başarı demektir.
class VaultSession
{
public:
    QString create(const QString &path, const QString &password,
                   const Crypto::KdfParams &params = Crypto::defaultParams());
    QString unlock(const QString &path, const QString &password);
    QString save();
    QString saveCopy(const QString &path) const; // şifreli yedek (aynı ana şifreyle açılır)
    QString changePassword(const QString &current, const QString &next,
                           const Crypto::KdfParams &params = Crypto::defaultParams());
    void lock();

    bool isUnlocked() const { return m_key != nullptr; }
    QString path() const { return m_path; }
    Vault &vault() { return m_vault; }
    const Vault &vault() const { return m_vault; }

    // Kayıt işlemleri: kimlik ve tarihler burada verilir, ardından kasa diske yazılır
    QString add(Entry entry);
    QString update(Entry entry);
    QString remove(const QString &id);

private:
    QString write(const QString &path) const;

    std::unique_ptr<Crypto::SecretKey> m_key;
    VaultFile::Header m_header;
    QString m_path;
    Vault m_vault;
};
