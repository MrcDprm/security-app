#include "ui/SettingsDialog.h"

#include "app/Settings.h"
#include "core/CsvImport.h"
#include "core/VaultSession.h"
#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QStandardPaths>
#include <QUuid>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(VaultSession &session, Settings &settings, QWidget *parent)
    : QDialog(parent), m_session(session), m_settings(settings)
{
    setWindowTitle(I18n::t("settings"));

    auto *security = new QGroupBox(I18n::t("security"), this);
    auto *form = new QFormLayout(security);
    m_autoLock = new QSpinBox(security);
    m_autoLock->setRange(1, 60);
    m_autoLock->setSuffix(" " + I18n::t("minutes"));
    m_autoLock->setValue(settings.autoLockMinutes());
    m_clipboard = new QSpinBox(security);
    m_clipboard->setRange(10, 120);
    m_clipboard->setSuffix(" " + I18n::t("seconds"));
    m_clipboard->setValue(settings.clipboardSeconds());
    m_lockOnMinimize = new QCheckBox(I18n::t("lock_on_minimize"), security);
    m_lockOnMinimize->setChecked(settings.lockOnMinimize());
    form->addRow(I18n::t("auto_lock_after"), m_autoLock);
    form->addRow(I18n::t("clear_clipboard_after"), m_clipboard);
    form->addRow(QString(), m_lockOnMinimize);

    auto *vault = new QGroupBox(I18n::t("vault"), this);
    auto *vaultLayout = new QVBoxLayout(vault);
    auto *path = new QLabel(QDir::toNativeSeparators(session.path()), vault);
    path->setObjectName("muted");
    path->setTextFormat(Qt::PlainText);
    path->setWordWrap(true);
    path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *change = new QPushButton(I18n::t("change_master"), vault);
    auto *backup = new QPushButton(I18n::t("export_backup"), vault);
    auto *import = new QPushButton(I18n::t("import_csv"), vault);
    vaultLayout->addWidget(path);
    for (QPushButton *button : {change, backup, import})
        vaultLayout->addWidget(button);

    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(security);
    layout->addWidget(vault);
    layout->addWidget(buttons);
    setMinimumWidth(460);

    connect(change, &QPushButton::clicked, this, &SettingsDialog::changeMasterPassword);
    connect(backup, &QPushButton::clicked, this, &SettingsDialog::exportBackup);
    connect(import, &QPushButton::clicked, this, &SettingsDialog::importCsv);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        m_settings.setAutoLockMinutes(m_autoLock->value());
        m_settings.setClipboardSeconds(m_clipboard->value());
        m_settings.setLockOnMinimize(m_lockOnMinimize->isChecked());
        accept();
    });
}

void SettingsDialog::changeMasterPassword()
{
    QDialog dialog(this);
    dialog.setWindowTitle(I18n::t("change_master"));
    auto *current = new PasswordField(&dialog);
    auto *next = new PasswordField(&dialog);
    auto *repeat = new PasswordField(&dialog);
    auto *strength = new QProgressBar(&dialog);
    auto *strengthText = new QLabel(&dialog);
    strengthText->setObjectName("muted");
    auto *error = new QLabel(&dialog);
    error->setWordWrap(true);
    error->setStyleSheet("color: " + Theme::danger().name());
    auto *form = new QFormLayout(&dialog);
    form->addRow(I18n::t("current_password"), current);
    form->addRow(I18n::t("new_password"), next);
    form->addRow(QString(), strength);
    form->addRow(QString(), strengthText);
    form->addRow(I18n::t("repeat_password"), repeat);
    form->addRow(error);
    auto *buttons = new QDialogButtonBox(&dialog);
    QPushButton *ok = Ui::accentButton(I18n::t("change_master"), &dialog);
    buttons->addButton(ok, QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    form->addRow(buttons);
    dialog.setMinimumWidth(460);

    auto validate = [=] {
        Ui::showStrength(strength, strengthText, next->text());
        const bool strong = Strength::acceptableMaster(next->text());
        const bool match = next->text() == repeat->text();
        error->setText(!next->text().isEmpty() && !strong ? I18n::t("master_rule")
                       : !repeat->text().isEmpty() && !match ? I18n::t("passwords_differ")
                                                             : QString());
        ok->setEnabled(strong && match && !current->text().isEmpty());
    };
    for (PasswordField *field : {current, next, repeat})
        connect(field, &QLineEdit::textChanged, &dialog, validate);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        QApplication::setOverrideCursor(Qt::WaitCursor); // iki anahtar türetilir, birkaç saniye sürebilir
        const QString result = m_session.changePassword(current->text(), next->text());
        QApplication::restoreOverrideCursor();
        if (result.isEmpty())
            dialog.accept();
        else
            error->setText(I18n::error(result));
    });
    validate();
    if (dialog.exec() == QDialog::Accepted)
        Ui::inform(this, I18n::t("master_changed"));
}

void SettingsDialog::exportBackup()
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString name = QFileInfo(m_session.path()).completeBaseName() + "-"
                         + QDate::currentDate().toString("yyyy-MM-dd") + ".vault";
    QString path = QFileDialog::getSaveFileName(this, I18n::t("export_backup"), folder + "/" + name,
                                                I18n::t("vault_files") + " (*.vault)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".vault", Qt::CaseInsensitive))
        path += ".vault";
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(m_session.path()).canonicalFilePath()) {
        Ui::warn(this, I18n::t("backup_same_file"));
        return;
    }
    const QString error = m_session.saveCopy(path);
    if (error.isEmpty())
        Ui::inform(this, I18n::t("backup_saved"));
    else
        Ui::warn(this, I18n::error(error));
}

void SettingsDialog::importCsv()
{
    Ui::inform(this, I18n::t("import_hint"));
    const QString path = QFileDialog::getOpenFileName(
        this, I18n::t("import_csv"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), "CSV (*.csv)");
    if (path.isEmpty())
        return;
    QFile file(path);
    if (file.size() > CsvImport::MAX_BYTES) {
        Ui::warn(this, I18n::error("csv_too_large"));
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        Ui::warn(this, I18n::error("read_failed"));
        return;
    }
    const CsvImport::Result result = CsvImport::read(file.readAll());
    file.close();
    if (!result.error.isEmpty() || result.entries.isEmpty()) {
        Ui::warn(this, I18n::error(result.error.isEmpty() ? "csv_empty" : result.error));
        return;
    }
    if (!Ui::ask(this, I18n::t("import_confirm").replace("{0}", QString::number(result.entries.size())), true))
        return;
    if (!m_session.isUnlocked()) // pencere açıkken kasa kilitlendiyse
        return;

    // Hepsi tek seferde eklenir ve bir kez kaydedilir; yazılamazsa hiçbiri eklenmemiş olur
    const qsizetype before = m_session.vault().entries.size();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (Entry e : result.entries) {
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        e.created = e.modified = e.passwordChanged = now;
        m_session.vault().entries << e;
    }
    const QString error = m_session.save();
    if (!error.isEmpty()) {
        m_session.vault().entries.resize(before);
        Ui::warn(this, I18n::error(error));
        return;
    }
    m_vaultChanged = true;
    Ui::inform(this, I18n::t("import_done")
                         .replace("{0}", QString::number(result.entries.size()))
                         .replace("{1}", QString::number(result.skipped)));
    // Dışa aktarılan CSV düz metindir: kullanıcıya silmesi hatırlatılır
    if (Ui::ask(this, I18n::t("delete_csv_question").replace("{0}", QFileInfo(path).fileName()), true)
        && !QFile::remove(path))
        Ui::warn(this, I18n::error("delete_failed"));
}
