#include "core/VaultSession.h"

#include <QFile>
#include <QSaveFile>
#include <QUuid>

QString VaultSession::create(const QString &path, const QString &password, const Crypto::KdfParams &params)
{
    lock();
    auto key = std::make_unique<Crypto::SecretKey>();
    VaultFile::Header header{params, Crypto::randomBytes(Crypto::SALT_BYTES), QByteArray()};
    if (!Crypto::deriveKey(password, header.salt, params, *key))
        return "crypto_failed";
    m_key = std::move(key);
    m_header = header;
    m_path = path;
    m_vault = Vault();
    const QString error = save();
    if (!error.isEmpty())
        lock();
    return error;
}

QString VaultSession::unlock(const QString &path, const QString &password)
{
    lock();
    QFile file(path);
    if (!file.exists())
        return "file_missing";
    if (file.size() > VaultFile::MAX_FILE_BYTES || !file.open(QIODevice::ReadOnly))
        return "not_a_vault";
    const QByteArray bytes = file.readAll();

    VaultFile::Header header;
    const QString headerError = VaultFile::parseHeader(bytes, header);
    if (!headerError.isEmpty())
        return headerError;

    auto key = std::make_unique<Crypto::SecretKey>();
    if (!Crypto::deriveKey(password, header.salt, header.params, *key))
        return "crypto_failed";
    // Yanlış şifre ile değiştirilmiş dosya ayırt edilemez: ikisinde de doğrulama etiketi tutmaz
    std::optional<QByteArray> json = Crypto::decrypt(*key, bytes.mid(VaultFile::HEADER_BYTES),
                                                      bytes.left(VaultFile::HEADER_BYTES), header.nonce);
    if (!json)
        return "wrong_password";
    std::optional<Vault> vault = VaultFile::fromJson(*json);
    Crypto::wipe(*json);
    if (!vault)
        return "corrupted";

    m_key = std::move(key);
    m_header = header;
    m_path = path;
    m_vault = *vault;
    return QString();
}

QString VaultSession::write(const QString &path) const
{
    if (!m_key)
        return "locked";
    // Her kayıtta yeni nonce: aynı anahtarla aynı nonce iki kez kullanılmamalı
    VaultFile::Header header = m_header;
    header.nonce = Crypto::randomBytes(Crypto::NONCE_BYTES);
    const QByteArray head = VaultFile::headerBytes(header);
    QByteArray json = VaultFile::toJson(m_vault);
    const QByteArray cipher = Crypto::encrypt(*m_key, json, head, header.nonce);
    Crypto::wipe(json);

    // Önce geçici dosyaya yazılır, sonra yerine konur: yazma yarıda kalırsa eski kasa bozulmaz
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(head + cipher) != head.size() + cipher.size() || !file.commit())
        return "save_failed";
    return QString();
}

QString VaultSession::save()
{
    return write(m_path);
}

QString VaultSession::saveCopy(const QString &path) const
{
    return write(path);
}

QString VaultSession::changePassword(const QString &current, const QString &next, const Crypto::KdfParams &params)
{
    if (!m_key)
        return "locked";
    // Mevcut şifre yeniden doğrulanır: açık bırakılmış bilgisayarda biri ana şifreyi değiştiremesin
    Crypto::SecretKey check;
    if (!Crypto::deriveKey(current, m_header.salt, m_header.params, check))
        return "crypto_failed";
    if (!Crypto::sameKey(check, *m_key))
        return "wrong_password";

    auto key = std::make_unique<Crypto::SecretKey>();
    VaultFile::Header header{params, Crypto::randomBytes(Crypto::SALT_BYTES), QByteArray()};
    if (!Crypto::deriveKey(next, header.salt, params, *key))
        return "crypto_failed";
    std::swap(m_key, key);
    std::swap(m_header, header);
    const QString error = save();
    if (!error.isEmpty()) { // yazılamadıysa eski anahtara dön: dosya hâlâ eski şifreyle şifreli
        std::swap(m_key, key);
        std::swap(m_header, header);
    }
    return error;
}

void VaultSession::lock()
{
    m_key.reset(); // SecretKey yıkıcısı anahtarı sıfırlar
    for (Entry &e : m_vault.entries)
        e.password.fill(QChar(0)); // QString'in tamamen silindiği garanti değil, ama açıkta kalan kopya azalır
    m_vault = Vault();
    m_path.clear();
}

QString VaultSession::add(Entry entry)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.created = entry.modified = entry.passwordChanged = now;
    m_vault.entries << entry;
    const QString error = save();
    if (!error.isEmpty())
        m_vault.entries.removeLast();
    return error;
}

QString VaultSession::update(Entry entry)
{
    for (Entry &existing : m_vault.entries) {
        if (existing.id != entry.id)
            continue;
        const Entry previous = existing;
        entry.created = existing.created;
        entry.modified = QDateTime::currentDateTimeUtc();
        entry.passwordChanged = entry.password == existing.password ? existing.passwordChanged : entry.modified;
        existing = entry;
        const QString error = save();
        if (!error.isEmpty())
            existing = previous;
        return error;
    }
    return "not_found";
}

QString VaultSession::remove(const QString &id)
{
    for (qsizetype i = 0; i < m_vault.entries.size(); ++i) {
        if (m_vault.entries[i].id != id)
            continue;
        const Entry removed = m_vault.entries.takeAt(i);
        const QString error = save();
        if (!error.isEmpty())
            m_vault.entries.insert(i, removed);
        return error;
    }
    return "not_found";
}
