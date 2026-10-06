#include "core/CsvImport.h"

#include "core/Totp.h"

#include <QUrl>

namespace {

// İçe aktarılan alanlar da kasa dosyasındaki sınırlara uymak zorunda
constexpr int MAX_TITLE = 200, MAX_USER = 300, MAX_PASSWORD = 1000, MAX_URL = 2000, MAX_NOTES = 20000;

int column(const QStringList &header, const QStringList &names)
{
    for (const QString &name : names) {
        const int index = header.indexOf(name);
        if (index >= 0)
            return index;
    }
    return -1;
}

QString cell(const QStringList &row, int index)
{
    return index >= 0 && index < row.size() ? row[index].trimmed() : QString();
}

} // namespace

namespace CsvImport {

QList<QStringList> parse(const QString &text)
{
    QList<QStringList> rows;
    QStringList row;
    QString field;
    bool quoted = false;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar c = text[i];
        if (quoted) {
            if (c == '"' && i + 1 < text.size() && text[i + 1] == '"') {
                field += '"'; // "" → "
                ++i;
            } else if (c == '"') {
                quoted = false;
            } else {
                field += c;
            }
        } else if (c == '"') {
            quoted = true;
        } else if (c == ',') {
            row << field;
            field.clear();
        } else if (c == '\n' || c == '\r') {
            if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
                ++i;
            row << field;
            field.clear();
            rows << row;
            row.clear();
        } else {
            field += c;
        }
    }
    if (!field.isEmpty() || !row.isEmpty()) {
        row << field;
        rows << row;
    }
    return rows;
}

Result read(const QByteArray &data)
{
    Result result;
    if (data.size() > MAX_BYTES) {
        result.error = "csv_too_large";
        return result;
    }
    QString text = QString::fromUtf8(data);
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1); // UTF-8 BOM
    const QList<QStringList> rows = parse(text);
    if (rows.isEmpty()) {
        result.error = "csv_unknown_format";
        return result;
    }

    QStringList header;
    for (const QString &name : rows.first())
        header << name.trimmed().toLower();
    const int title = column(header, {"name", "title"});
    const int url = column(header, {"url", "login_uri"});
    const int user = column(header, {"username", "login_username"});
    const int password = column(header, {"password", "login_password"});
    const int notes = column(header, {"note", "notes"});
    const int totp = column(header, {"login_totp", "totp"});
    const int type = column(header, {"type"});
    const int favorite = column(header, {"favorite"});
    if (password < 0 || (url < 0 && title < 0)) {
        result.error = "csv_unknown_format";
        return result;
    }

    for (qsizetype i = 1; i < rows.size() && i <= MAX_ROWS; ++i) {
        const QStringList &row = rows[i];
        if (type >= 0 && cell(row, type) != "login") { // Bitwarden: kart, kimlik ve güvenli notlar atlanır
            ++result.skipped;
            continue;
        }
        Entry e;
        e.url = cell(row, url);
        e.username = cell(row, user);
        e.password = cell(row, password);
        e.notes = cell(row, notes);
        // 2FA anahtarı "otpauth://" adresi olarak da gelebilir; geçersizse kayıt alınır ama anahtar atılır
        e.totp = Totp::normalizeSecret(cell(row, totp));
        if (!e.totp.isEmpty() && !Totp::isValidSecret(e.totp))
            e.totp.clear();
        e.favorite = cell(row, favorite) == "1";
        e.title = cell(row, title);
        if (e.title.isEmpty()) // Firefox'ta ad yok: adresin alan adı kullanılır
            e.title = QUrl(e.url).host().isEmpty() ? e.url : QUrl(e.url).host();
        if ((e.username.isEmpty() && e.password.isEmpty()) || e.title.size() > MAX_TITLE
            || e.username.size() > MAX_USER || e.password.size() > MAX_PASSWORD || e.url.size() > MAX_URL
            || e.notes.size() > MAX_NOTES) {
            ++result.skipped;
            continue;
        }
        result.entries << e;
    }
    result.skipped += std::max<qsizetype>(0, rows.size() - 1 - MAX_ROWS);
    return result;
}

} // namespace CsvImport
