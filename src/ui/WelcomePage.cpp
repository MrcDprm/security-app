#include "ui/WelcomePage.h"

#include "core/DemoVault.h"
#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

WelcomePage::WelcomePage(const QString &defaultPath, QWidget *parent)
    : QWidget(parent), m_path(defaultPath)
{
    auto *icon = new QLabel(this);
    icon->setPixmap(QPixmap(":/icon.png").scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setAlignment(Qt::AlignCenter);
    auto *title = new QLabel(I18n::t("app_name"), this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(I18n::t("welcome_text"), this);
    subtitle->setObjectName("muted");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);

    // Yeni kasa kartı
    auto *card = new QFrame(this);
    card->setObjectName("panel");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 18, 22, 18);
    auto *heading = new QLabel(I18n::t("create_vault"), card);
    heading->setObjectName("heading");
    auto *explain = new QLabel(I18n::t("master_explain"), card);
    explain->setObjectName("muted");
    explain->setWordWrap(true);
    m_password = new PasswordField(card);
    m_confirm = new PasswordField(card);
    m_strength = new QProgressBar(card);
    m_strengthText = new QLabel(card);
    m_strengthText->setObjectName("muted");
    auto *form = new QFormLayout;
    form->addRow(I18n::t("master_password"), m_password);
    form->addRow(QString(), m_strength);
    form->addRow(QString(), m_strengthText);
    form->addRow(I18n::t("repeat_password"), m_confirm);
    m_location = new QLabel(card);
    m_location->setObjectName("muted");
    m_location->setTextFormat(Qt::PlainText);
    auto *change = new QPushButton(I18n::t("change_location"), card);
    auto *locationRow = new QHBoxLayout;
    locationRow->addWidget(m_location, 1);
    locationRow->addWidget(change);
    m_error = new QLabel(card);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    m_create = Ui::accentButton(I18n::t("create_vault"), card);
    cardLayout->addWidget(heading);
    cardLayout->addWidget(explain);
    cardLayout->addLayout(form);
    cardLayout->addLayout(locationRow);
    cardLayout->addWidget(m_error);
    cardLayout->addWidget(m_create);

    // Diğer seçenekler
    auto *open = new QPushButton(I18n::t("open_existing"), this);
    auto *demo = new QPushButton(I18n::t("try_demo"), this);
    auto *demoNote = new QLabel(I18n::t("demo_note").replace("{0}", DemoVault::PASSWORD), this);
    demoNote->setObjectName("muted");
    demoNote->setAlignment(Qt::AlignCenter);
    auto *others = new QHBoxLayout;
    others->addWidget(open);
    others->addWidget(demo);

    auto *column = new QVBoxLayout;
    column->addStretch();
    column->addWidget(icon);
    column->addWidget(title);
    column->addWidget(subtitle);
    column->addSpacing(14);
    column->addWidget(card);
    column->addSpacing(8);
    column->addLayout(others);
    column->addWidget(demoNote);
    column->addStretch();
    auto *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->addLayout(column);
    layout->addStretch();
    card->setFixedWidth(500);

    connect(m_password, &QLineEdit::textChanged, this, &WelcomePage::validate);
    connect(m_confirm, &QLineEdit::textChanged, this, &WelcomePage::validate);
    connect(m_confirm, &QLineEdit::returnPressed, m_create, &QPushButton::click);
    connect(change, &QPushButton::clicked, this, &WelcomePage::chooseLocation);
    connect(m_create, &QPushButton::clicked, this, [this] { emit createRequested(m_path, m_password->text()); });
    connect(open, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, I18n::t("open_existing"), QDir::homePath(),
                                                          I18n::t("vault_files") + " (*.vault)");
        if (!path.isEmpty())
            emit openRequested(path);
    });
    connect(demo, &QPushButton::clicked, this, &WelcomePage::demoRequested);
    validate();
}

void WelcomePage::validate()
{
    Ui::setPath(m_location, I18n::t("vault_location") + ": ", m_path, 270);
    Ui::showStrength(m_strength, m_strengthText, m_password->text());
    // Zayıf ana şifreye izin verilmez: kasanın tüm güvenliği bu şifreye bağlı
    const bool strong = Strength::acceptableMaster(m_password->text());
    const bool match = m_password->text() == m_confirm->text();
    m_error->setText(!m_password->text().isEmpty() && !strong ? I18n::t("master_rule")
                     : !m_confirm->text().isEmpty() && !match ? I18n::t("passwords_differ")
                                                              : QString());
    m_create->setEnabled(strong && match);
}

void WelcomePage::chooseLocation()
{
    const QString path = QFileDialog::getSaveFileName(this, I18n::t("change_location"), m_path,
                                                      I18n::t("vault_files") + " (*.vault)");
    if (path.isEmpty())
        return;
    m_path = path.endsWith(".vault", Qt::CaseInsensitive) ? path : path + ".vault";
    validate();
}

void WelcomePage::clearFields()
{
    m_password->clear();
    m_confirm->clear();
    m_password->setRevealed(false);
    m_confirm->setRevealed(false);
}

void WelcomePage::showError(const QString &text)
{
    m_error->setText(text);
}
