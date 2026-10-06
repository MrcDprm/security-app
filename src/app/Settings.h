#pragma once

#include <QString>

// Kullanıcı ayarları kullanıcı klasöründeki settings.ini dosyasında tutulur (şifre ya da anahtar asla değil).
// Dosyadan okunan değerlere güvenilmez: bilinmeyen ya da sınır dışı değer yerine varsayılan kullanılır.
class Settings
{
public:
    explicit Settings(const QString &dataFolder);

    QString language() const;
    void setLanguage(const QString &language);
    bool darkTheme() const;
    void setDarkTheme(bool dark);
    int autoLockMinutes() const;       // 1..60, varsayılan 5
    void setAutoLockMinutes(int minutes);
    int clipboardSeconds() const;      // 10..120, varsayılan 30
    void setClipboardSeconds(int seconds);
    bool lockOnMinimize() const;
    void setLockOnMinimize(bool enabled);
    QString lastVault() const;         // son açılan kasa dosyası; yoksa boş
    void setLastVault(const QString &path);

private:
    QString m_path;
};
