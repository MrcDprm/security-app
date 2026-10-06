#pragma once

#include "core/Entry.h"
#include "core/Strength.h"

#include <QString>

// Arayüz metinleri (Türkçe / İngilizce). Metinler anahtarla istenir: I18n::t("lock").
// {0}, {1}… yer tutucuları çağıran tarafta doldurulur.
namespace I18n {

void setLanguage(const QString &language); // "tr" ya da "en"
QString language();

QString t(const char *key);
QString error(const QString &key); // çekirdekten gelen hata anahtarları ("wrong_password")

QString category(Category value);
QString strength(Strength::Level value);
QString dateTime(const QDateTime &value);

} // namespace I18n
