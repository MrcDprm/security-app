#include "ui/EntryDialog.h"

#include "core/Generator.h"
#include "core/Totp.h"
#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QUrl>

namespace {

// Kasa dosyasındaki sınırların aynısı (VaultFile)
constexpr int MAX_TITLE = 200, MAX_USER = 300, MAX_PASSWORD = 1000, MAX_URL = 2000, MAX_NOTES = 20000;

} // namespace

EntryDialog::EntryDialog(const Entry &entry, QWidget *parent)
    : QDialog(parent), m_entry(entry)
{
    setWindowTitle(I18n::t(entry.id.isEmpty() ? "new_entry" : "edit_entry"));
    m_title = new QLineEdit(entry.title, this);
    m_title->setMaxLength(MAX_TITLE);
    m_username = new QLineEdit(entry.username, this);
    m_username->setMaxLength(MAX_USER);
    m_password = new PasswordField(this);
    m_password->setMaxLength(MAX_PASSWORD);
    m_password->setText(entry.password);
    auto *generate = new QPushButton(I18n::t("generate"), this);
    auto *passwordRow = new QHBoxLayout;
    passwordRow->addWidget(m_password, 1);
    passwordRow->addWidget(generate);
    m_strength = new QProgressBar(this);
    m_strengthText = new QLabel(this);
    m_strengthText->setObjectName("muted");
    m_url = new QLineEdit(entry.url, this);
    m_url->setMaxLength(MAX_URL);
    m_url->setPlaceholderText("https://");
    m_category = new QComboBox(this);
    for (int i = 0; i <= static_cast<int>(Category::Other); ++i)
        m_category->addItem(I18n::category(static_cast<Category>(i)));
    m_category->setCurrentIndex(static_cast<int>(entry.category));
    m_totp = new QLineEdit(entry.totp, this);
    m_totp->setPlaceholderText(I18n::t("totp_placeholder"));
    m_notes = new QPlainTextEdit(entry.notes, this);
    m_notes->setFixedHeight(90);
    m_favorite = new QCheckBox(I18n::t("favorite"), this);
    m_favorite->setChecked(entry.favorite);

    auto *form = new QFormLayout;
    form->addRow(I18n::t("title"), m_title);
    form->addRow(I18n::t("username"), m_username);
    form->addRow(I18n::t("password"), passwordRow);
    form->addRow(QString(), m_strength);
    form->addRow(QString(), m_strengthText);
    form->addRow(I18n::t("url"), m_url);
    form->addRow(I18n::t("category"), m_category);
    form->addRow(I18n::t("totp_secret"), m_totp);
    form->addRow(I18n::t("notes"), m_notes);
    form->addRow(QString(), m_favorite);

    m_error = new QLabel(this);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(Ui::accentButton(I18n::t("save"), this), QDialogButtonBox::AcceptRole);
    buttons->addButton(I18n::t("cancel"), QDialogButtonBox::RejectRole);
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_error);
    layout->addWidget(buttons);
    setMinimumWidth(520);

    connect(buttons, &QDialogButtonBox::accepted, this, &EntryDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_password, &QLineEdit::textChanged, this, [this] { Ui::showStrength(m_strength, m_strengthText, m_password->text()); });
    connect(generate, &QPushButton::clicked, this, [this] {
        m_password->setText(Generator::password({}));
        m_password->setRevealed(true); // üretilen şifre görülsün
    });
    Ui::showStrength(m_strength, m_strengthText, m_password->text());
}

void EntryDialog::save()
{
    Entry e = m_entry;
    e.title = m_title->text().trimmed();
    e.username = m_username->text().trimmed();
    e.password = m_password->text();
    e.url = m_url->text().trimmed();
    e.category = static_cast<Category>(m_category->currentIndex());
    e.totp = Totp::normalizeSecret(m_totp->text());
    e.notes = m_notes->toPlainText();
    e.favorite = m_favorite->isChecked();

    if (e.title.isEmpty()) {
        m_error->setText(I18n::t("title_required"));
        return;
    }
    if (e.notes.size() > MAX_NOTES) {
        m_error->setText(I18n::t("notes_too_long"));
        return;
    }
    if (!e.totp.isEmpty() && !Totp::isValidSecret(e.totp)) {
        m_error->setText(I18n::t("invalid_totp"));
        return;
    }
    // Adres sadece http/https olabilir: "javascript:" ya da "file:" gibi adresler "Aç" ile çalıştırılmasın
    if (!e.url.isEmpty()) {
        if (!e.url.contains("://"))
            e.url.prepend("https://");
        const QUrl url(e.url);
        if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https") || url.host().isEmpty()) {
            m_error->setText(I18n::t("invalid_url"));
            return;
        }
    }
    m_entry = e;
    accept();
}
