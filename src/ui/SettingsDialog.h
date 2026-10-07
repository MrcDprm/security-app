#pragma once

#include <QDialog>

class QCheckBox;
class QSpinBox;
class Settings;
class VaultSession;

// Ayarlar: otomatik kilit süresi, panonun temizlenme süresi, simge durumuna küçültünce kilit;
// ana şifreyi değiştirme, şifreli yedek alma ve CSV'den içe aktarma.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(VaultSession &session, Settings &settings, QWidget *parent);
    bool vaultChanged() const { return m_vaultChanged; } // içe aktarmadan sonra liste yenilensin

private:
    void changeMasterPassword();
    void exportBackup();
    void importCsv();

    VaultSession &m_session;
    Settings &m_settings;
    QSpinBox *m_autoLock, *m_clipboard;
    QCheckBox *m_lockOnMinimize;
    bool m_vaultChanged = false;
};
