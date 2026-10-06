#include "core/VaultFile.h"

#include <QDataStream>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace {

const QByteArray MAGIC = "PVLT";
constexpr int MAX_ENTRIES = 10000;

// Alan sınırları: dosyadan okunan değer bundan uzunsa kasa bozuk sayılır
struct Limit {
    const char *name;
    int max;
};
constexpr Limit TEXT_FIELDS[] = {{"title", 200},  {"username", 300}, {"password", 1000},
                                 {"url", 2000},   {"notes", 20000},  {"totp", 256}};

// Argon2 ayarları da dosyadan geldiği için sınırlı: aşırı büyük bellek isteyen bir dosya programı kilitlemesin
constexpr quint32 MIN_OPS = 1, MAX_OPS = 20;
constexpr quint32 MIN_MEMORY = 8u * 1024 * 1024, MAX_MEMORY = 1024u * 1024 * 1024;

QString isoDate(const QDateTime &date)
{
    return date.toUTC().toString(Qt::ISODate);
}

QDateTime readDate(const QJsonValue &value)
{
    const QDateTime date = QDateTime::fromString(value.toString(), Qt::ISODate);
    return date.isValid() ? date : QDateTime::currentDateTimeUtc();
}

} // namespace

namespace VaultFile {

QByteArray headerBytes(const Header &header)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.writeRawData(MAGIC.constData(), MAGIC.size());
    stream << VERSION << header.params.ops << header.params.memory;
    stream.writeRawData(header.salt.constData(), header.salt.size());
    stream.writeRawData(header.nonce.constData(), header.nonce.size());
    return bytes;
}

QString parseHeader(const QByteArray &file, Header &header)
{
    if (file.size() < HEADER_BYTES || !file.startsWith(MAGIC))
        return "not_a_vault";
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.skipRawData(MAGIC.size());
    quint8 version = 0;
    stream >> version >> header.params.ops >> header.params.memory;
    if (version != VERSION)
        return "unsupported_version";
    if (header.params.ops < MIN_OPS || header.params.ops > MAX_OPS || header.params.memory < MIN_MEMORY
        || header.params.memory > MAX_MEMORY)
        return "not_a_vault";
    header.salt = file.mid(4 + 1 + 4 + 4, Crypto::SALT_BYTES);
    header.nonce = file.mid(4 + 1 + 4 + 4 + Crypto::SALT_BYTES, Crypto::NONCE_BYTES);
    return QString();
}

QByteArray toJson(const Vault &vault)
{
    QJsonArray entries;
    for (const Entry &e : vault.entries) {
        entries.append(QJsonObject{
            {"id", e.id},
            {"title", e.title},
            {"username", e.username},
            {"password", e.password},
            {"url", e.url},
            {"notes", e.notes},
            {"totp", e.totp},
            {"category", static_cast<int>(e.category)},
            {"favorite", e.favorite},
            {"created", isoDate(e.created)},
            {"modified", isoDate(e.modified)},
            {"passwordChanged", isoDate(e.passwordChanged)},
        });
    }
    return QJsonDocument(QJsonObject{{"version", 1}, {"entries", entries}}).toJson(QJsonDocument::Compact);
}

std::optional<Vault> fromJson(const QByteArray &json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return std::nullopt;
    const QJsonValue list = document.object().value("entries");
    if (!list.isArray() || list.toArray().size() > MAX_ENTRIES)
        return std::nullopt;

    Vault vault;
    for (const QJsonValue &item : list.toArray()) {
        if (!item.isObject())
            return std::nullopt;
        const QJsonObject o = item.toObject();
        for (const Limit &field : TEXT_FIELDS) {
            const QJsonValue value = o.value(field.name);
            if (!value.isUndefined() && (!value.isString() || value.toString().size() > field.max))
                return std::nullopt;
        }
        Entry e;
        // Geçersiz ya da eksik kimlik yerine yenisi verilir
        e.id = QUuid::fromString(o.value("id").toString()).isNull() ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                                                                    : o.value("id").toString();
        e.title = o.value("title").toString();
        e.username = o.value("username").toString();
        e.password = o.value("password").toString();
        e.url = o.value("url").toString();
        e.notes = o.value("notes").toString();
        e.totp = o.value("totp").toString();
        const int category = o.value("category").toInt();
        e.category = category >= 0 && category <= static_cast<int>(Category::Other) ? static_cast<Category>(category)
                                                                                     : Category::General;
        e.favorite = o.value("favorite").toBool();
        e.created = readDate(o.value("created"));
        e.modified = readDate(o.value("modified"));
        e.passwordChanged = readDate(o.value("passwordChanged"));
        vault.entries << e;
    }
    return vault;
}

} // namespace VaultFile
