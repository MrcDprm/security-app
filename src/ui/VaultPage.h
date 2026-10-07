#pragma once

#include <QWidget>

class EntriesView;
class GeneratorView;
class HealthView;
class QLabel;
class QListWidget;
class QStackedWidget;
class TotpView;
class VaultSession;

// Açık kasa: solda menü (tümü, favoriler, kategoriler, 2FA kodları, sağlık, üretici), sağda seçilen sayfa.
class VaultPage : public QWidget
{
    Q_OBJECT

public:
    VaultPage(VaultSession &session, QWidget *parent);
    void reload();     // kasa açıldığında
    void clear();      // kilitlenince: listelerdeki kopyalar da silinir
    void tick();       // her saniye: 2FA kodları
    void showStatus(const QString &text);

signals:
    void copyRequested(const QString &text, const QString &what);
    void lockRequested();
    void settingsRequested();
    void themeRequested();
    void languageRequested();
    void aboutRequested();

private:
    void buildNavigation();
    void showPage(int row);

    VaultSession &m_session;
    QListWidget *m_navigation;
    QStackedWidget *m_pages;
    EntriesView *m_entries;
    TotpView *m_totp;
    HealthView *m_health;
    GeneratorView *m_generator;
    QLabel *m_status;
};
