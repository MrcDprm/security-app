#include "app/Settings.h"

#include <QFileInfo>
#include <QSettings>

namespace {

int bounded(const QVariant &value, int low, int high, int fallback)
{
    bool ok = false;
    const int number = value.toInt(&ok);
    return ok && number >= low && number <= high ? number : fallback;
}

} // namespace

Settings::Settings(const QString &dataFolder)
    : m_path(dataFolder + "/settings.ini")
{
}

QString Settings::language() const
{
    const QString value = QSettings(m_path, QSettings::IniFormat).value("language", "tr").toString();
    return value == "en" ? "en" : "tr";
}

void Settings::setLanguage(const QString &language)
{
    QSettings(m_path, QSettings::IniFormat).setValue("language", language == "en" ? "en" : "tr");
}

bool Settings::darkTheme() const
{
    return QSettings(m_path, QSettings::IniFormat).value("theme", "dark").toString() != "light";
}

void Settings::setDarkTheme(bool dark)
{
    QSettings(m_path, QSettings::IniFormat).setValue("theme", dark ? "dark" : "light");
}

int Settings::autoLockMinutes() const
{
    return bounded(QSettings(m_path, QSettings::IniFormat).value("auto_lock_minutes"), 1, 60, 5);
}

void Settings::setAutoLockMinutes(int minutes)
{
    QSettings(m_path, QSettings::IniFormat).setValue("auto_lock_minutes", std::clamp(minutes, 1, 60));
}

int Settings::clipboardSeconds() const
{
    return bounded(QSettings(m_path, QSettings::IniFormat).value("clipboard_seconds"), 10, 120, 30);
}

void Settings::setClipboardSeconds(int seconds)
{
    QSettings(m_path, QSettings::IniFormat).setValue("clipboard_seconds", std::clamp(seconds, 10, 120));
}

bool Settings::lockOnMinimize() const
{
    return QSettings(m_path, QSettings::IniFormat).value("lock_on_minimize", true).toBool();
}

void Settings::setLockOnMinimize(bool enabled)
{
    QSettings(m_path, QSettings::IniFormat).setValue("lock_on_minimize", enabled);
}

QString Settings::lastVault() const
{
    const QString path = QSettings(m_path, QSettings::IniFormat).value("last_vault").toString();
    return QFileInfo(path).isFile() ? path : QString(); // silinmiş ya da taşınmış dosya hatırlanmaz
}

void Settings::setLastVault(const QString &path)
{
    QSettings(m_path, QSettings::IniFormat).setValue("last_vault", path);
}
