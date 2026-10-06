#pragma once

#include "core/Entry.h"

#include <QHash>
#include <QSet>

// Şifre sağlığı raporu: zayıf, tekrar kullanılan, eski ve sızıntıda görülen şifreler.
// Sonuçlar kayıt kimlikleriyle döner; arayüz listeyi bunlardan kurar.
namespace Health {

constexpr int OLD_AFTER_DAYS = 365;

struct Report {
    int checked = 0; // şifresi olan kayıt sayısı
    QSet<QString> weak;
    QSet<QString> reused;
    QSet<QString> old;
    QSet<QString> breached;
    int score = 100; // sorunsuz kayıtların yüzdesi
};

// breachCounts: kayıt kimliği → sızıntı sayısı (kontrol edilmeyenler listede yoktur)
Report analyze(const QList<Entry> &entries, const QHash<QString, int> &breachCounts, const QDateTime &now);

} // namespace Health
