#include "ui/MainWindow.h"

#include "app/Settings.h"
#include "core/DemoVault.h"
#include "services/AutoLock.h"
#include "services/SecureClipboard.h"
#include "ui/AboutDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/UiHelpers.h"
#include "ui/UnlockPage.h"
#include "ui/VaultPage.h"
#include "ui/WelcomePage.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QStackedWidget>

MainWindow::MainWindow(Settings &settings, const QString &dataFolder)
    : m_settings(settings), m_dataFolder(dataFolder), m_vaultPath(settings.lastVault()),
      m_clipboard(new SecureClipboard(this)), m_autoLock(new AutoLock(this))
{
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);
    qApp->installEventFilter(m_autoLock); // uygulamanın tüm fare/klavye olayları sayacı sıfırlar
    connect(m_autoLock, &AutoLock::timedOut, this, &MainWindow::lock);
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, [this] {
        if (!m_session.isUnlocked())
            return;
        m_vault->tick();
        const int left = m_clipboard->secondsLeft();
        m_vault->showStatus(left > 0 ? I18n::t("clipboard_countdown").replace("{0}", QString::number(left)) : QString());
    });
    m_tick.start();
    connect(m_clipboard, &SecureClipboard::cleared, this, [this] {
        if (m_vault)
            m_vault->showStatus(I18n::t("clipboard_cleared"));
    });
    buildPages();
    showStart();
    resize(1180, 740);
}

MainWindow::~MainWindow()
{
    qApp->removeEventFilter(m_autoLock);
}

void MainWindow::buildPages()
{
    setWindowTitle(I18n::t("app_name"));
    while (m_stack->count() > 0) {
        QWidget *page = m_stack->widget(0);
        m_stack->removeWidget(page);
        page->deleteLater();
    }
    m_welcome = new WelcomePage(m_dataFolder + "/" + I18n::t("default_vault_name") + ".vault", m_stack);
    m_unlock = new UnlockPage(m_stack);
    m_vault = new VaultPage(m_session, m_stack);
    for (QWidget *page : std::initializer_list<QWidget *>{m_welcome, m_unlock, m_vault})
        m_stack->addWidget(page);

    connect(m_welcome, &WelcomePage::createRequested, this, &MainWindow::createVault);
    connect(m_welcome, &WelcomePage::openRequested, this, &MainWindow::openVault);
    connect(m_welcome, &WelcomePage::demoRequested, this, &MainWindow::openDemo);
    connect(m_unlock, &UnlockPage::unlockRequested, this, &MainWindow::unlock);
    connect(m_unlock, &UnlockPage::newVaultRequested, this, [this] { m_stack->setCurrentWidget(m_welcome); });
    connect(m_unlock, &UnlockPage::otherVaultRequested, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, I18n::t("open_existing"), m_dataFolder,
                                                          I18n::t("vault_files") + " (*.vault)");
        if (!path.isEmpty())
            openVault(path);
    });
    connect(m_vault, &VaultPage::copyRequested, this, &MainWindow::copy);
    connect(m_vault, &VaultPage::lockRequested, this, &MainWindow::lock);
    connect(m_vault, &VaultPage::settingsRequested, this, &MainWindow::openSettings);
    connect(m_vault, &VaultPage::themeRequested, this, &MainWindow::toggleTheme);
    connect(m_vault, &VaultPage::languageRequested, this, &MainWindow::toggleLanguage);
    connect(m_vault, &VaultPage::aboutRequested, this, [this] { AboutDialog(m_dataFolder, this).exec(); });
}

void MainWindow::showStart()
{
    // Açık kasa varsa kasa, daha önce açılmış bir kasa dosyası varsa kilit ekranı, yoksa karşılama
    if (m_session.isUnlocked()) {
        m_vault->reload();
        m_stack->setCurrentWidget(m_vault);
    } else if (!m_vaultPath.isEmpty() && QFileInfo(m_vaultPath).isFile()) {
        m_unlock->setVault(m_vaultPath);
        m_unlock->reset();
        m_stack->setCurrentWidget(m_unlock);
    } else {
        m_stack->setCurrentWidget(m_welcome);
    }
}

void MainWindow::createVault(const QString &path, const QString &password)
{
    if (QFileInfo::exists(path) && !Ui::ask(this, I18n::t("overwrite_vault").replace("{0}", QFileInfo(path).fileName())))
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const QString error = m_session.create(path, password);
    QApplication::restoreOverrideCursor();
    if (!error.isEmpty()) {
        m_welcome->showError(I18n::error(error));
        return;
    }
    m_vaultPath = path;
    opened();
}

void MainWindow::openVault(const QString &path)
{
    lock();
    m_vaultPath = path;
    m_unlock->setVault(path);
    m_unlock->reset();
    m_stack->setCurrentWidget(m_unlock);
}

void MainWindow::openDemo()
{
    // Demo kasa her seferinde yeniden oluşturulur: denemeler sonraki açılışı etkilemez
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const QString path = m_dataFolder + "/demo.vault";
    const QString error = DemoVault::create(m_session, path);
    QApplication::restoreOverrideCursor();
    if (!error.isEmpty()) {
        Ui::warn(this, I18n::error(error));
        return;
    }
    m_vaultPath = path;
    opened();
}

void MainWindow::unlock(const QString &password)
{
    const QString error = m_session.unlock(m_vaultPath, password);
    if (!error.isEmpty()) {
        m_unlock->unlockFailed(I18n::error(error));
        return;
    }
    opened();
}

void MainWindow::opened()
{
    m_settings.setLastVault(m_vaultPath);
    // Ana şifre, kasa açıkken gizli sayfalardaki kutularda beklemesin
    m_welcome->clearFields();
    m_unlock->reset();
    m_vault->reload();
    m_stack->setCurrentWidget(m_vault);
    m_autoLock->start(m_settings.autoLockMinutes());
}

void MainWindow::lock()
{
    if (!m_session.isUnlocked())
        return;
    m_autoLock->stop();
    m_clipboard->clearNow(); // panoda bizim kopyaladığımız şifre duruyorsa silinir
    m_session.lock();
    m_vault->clear();
    // Kilitlenince açık olan pencereler (kayıt formu, ayarlar) kapanır: kasa verisi ekranda kalmasın
    for (QWidget *window : QApplication::topLevelWidgets())
        if (auto *dialog = qobject_cast<QDialog *>(window); dialog && dialog->isVisible())
            dialog->reject();
    m_unlock->setVault(m_vaultPath);
    m_unlock->reset();
    m_stack->setCurrentWidget(m_unlock);
}

void MainWindow::copy(const QString &text, const QString &what)
{
    if (text.isEmpty())
        return;
    m_clipboard->copy(text, m_settings.clipboardSeconds());
    m_vault->showStatus(I18n::t(("copied_" + what).toLatin1().constData()));
}

void MainWindow::toggleTheme()
{
    const bool dark = !Theme::isDark();
    m_settings.setDarkTheme(dark);
    Theme::apply(*qApp, dark);
    buildPages();
    showStart();
}

void MainWindow::toggleLanguage()
{
    const QString language = I18n::language() == "tr" ? "en" : "tr";
    m_settings.setLanguage(language);
    I18n::setLanguage(language);
    buildPages();
    showStart();
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_session, m_settings, this);
    const bool accepted = dialog.exec() == QDialog::Accepted;
    if (!m_session.isUnlocked())
        return; // ayarlar açıkken otomatik kilit devreye girdiyse
    if (accepted)
        m_autoLock->start(m_settings.autoLockMinutes());
    if (dialog.vaultChanged())
        m_vault->reload();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && isMinimized() && m_settings.lockOnMinimize())
        lock();
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_clipboard->clearNow();
    m_session.lock();
    event->accept();
}
