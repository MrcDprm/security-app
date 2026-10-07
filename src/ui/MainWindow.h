#pragma once

#include "core/VaultSession.h"

#include <QMainWindow>
#include <QTimer>

class AutoLock;
class SecureClipboard;
class Settings;
class QStackedWidget;
class UnlockPage;
class VaultPage;
class WelcomePage;

// Ana pencere üç ekran arasında geçer: karşılama (kasa yok), kilit (kasa var, kilitli), kasa (açık).
// Kasa oturumu, pano ve otomatik kilit burada yaşar; kilitlenince hepsi temizlenir.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Settings &settings, const QString &dataFolder);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void buildPages(); // dil ya da tema değişince ekranlar yeniden kurulur
    void showStart();
    void createVault(const QString &path, const QString &password);
    void openVault(const QString &path);
    void openDemo();
    void unlock(const QString &password);
    void opened();
    void lock();
    void copy(const QString &text, const QString &what);
    void toggleTheme();
    void toggleLanguage();
    void openSettings();

    Settings &m_settings;
    QString m_dataFolder;
    QString m_vaultPath;
    VaultSession m_session;
    SecureClipboard *m_clipboard;
    AutoLock *m_autoLock;
    QTimer m_tick;
    QStackedWidget *m_stack;
    WelcomePage *m_welcome = nullptr;
    UnlockPage *m_unlock = nullptr;
    VaultPage *m_vault = nullptr;
};
