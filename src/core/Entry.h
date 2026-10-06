#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

// Kasadaki bir kayıt (bir sitenin ya da uygulamanın giriş bilgileri).
// Kimlik tahmin edilemez bir UUID'dir; sıra numarası (1, 2, 3) kullanılmaz.
enum class Category { General, Email, Social, Finance, Shopping, Work, Entertainment, Other };

struct Entry {
    QString id;
    QString title;
    QString username;
    QString password;
    QString url;
    QString notes;
    QString totp; // 2FA anahtarı (Base32); boşsa kod üretilmez
    Category category = Category::General;
    bool favorite = false;
    QDateTime created;
    QDateTime modified;
    QDateTime passwordChanged; // "eski şifre" uyarısı bu tarihe göre
};

struct Vault {
    QList<Entry> entries;
};