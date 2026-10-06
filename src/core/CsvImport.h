#pragma once

#include "core/Entry.h"

#include <QStringList>

// Tarayıcıların ve Bitwarden'ın dışa aktardığı CSV dosyalarından kayıt okuma.
// Sütunlar başlık satırındaki adlarından bulunur; sıra farklı olsa da çalışır.
//   Chrome / Edge: name, url, username, password, note
//   Firefox:       url, username, password, ...
//   Bitwarden:     folder, favorite, type, name, notes, ..., login_uri, login_username, login_password, login_totp
namespace CsvImport {

constexpr qint64 MAX_BYTES = 20 * 1024 * 1024;
constexpr int MAX_ROWS = 10000;

struct Result {
    QList<Entry> entries;
    int skipped = 0;  // boş, kayıt dışı (Bitwarden kart/not) ya da çok uzun alanlı satırlar
    QString error;    // "csv_unknown_format", "csv_too_large"
};

QList<QStringList> parse(const QString &text); // RFC 4180: tırnaklı alan, "" kaçışı, alan içinde satır sonu
Result read(const QByteArray &data);

} // namespace CsvImport
