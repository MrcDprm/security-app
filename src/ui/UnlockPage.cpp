#include "ui/UnlockPage.h"

#include "ui/PasswordField.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {

constexpr int FREE_ATTEMPTS = 3;
constexpr int MAX_WAIT = 60;

} // namespace

UnlockPage::UnlockPage(QWidget *parent)
    : QWidget(parent)
{
    auto *icon = new QLabel(this);
    icon->setPixmap(QPixmap(":/icon.png").scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setAlignment(Qt::AlignCenter);
    auto *title = new QLabel(I18n::t("vault_locked"), this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);

    auto *card = new QFrame(this);
    card->setObjectName("panel");
    card->setFixedWidth(440);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 18, 22, 18);
    m_file = new QLabel(card);
    m_file->setObjectName("muted");
    m_file->setTextFormat(Qt::PlainText);
    m_password = new PasswordField(card);
    m_password->setPlaceholderText(I18n::t("master_password"));
    m_error = new QLabel(card);
    m_error->setWordWrap(true);
    m_error->setStyleSheet("color: " + Theme::danger().name());
    m_unlock = Ui::accentButton(I18n::t("unlock"), card);
    cardLayout->addWidget(m_file);
    cardLayout->addWidget(m_password);
    cardLayout->addWidget(m_error);
    cardLayout->addWidget(m_unlock);

    auto *other = new QPushButton(I18n::t("open_other"), this);
    auto *create = new QPushButton(I18n::t("new_vault"), this);
    auto *others = new QHBoxLayout;
    others->addWidget(other);
    others->addWidget(create);

    auto *column = new QVBoxLayout;
    column->addStretch();
    column->addWidget(icon);
    column->addWidget(title);
    column->addSpacing(14);
    column->addWidget(card);
    column->addLayout(others);
    column->addStretch();
    auto *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->addLayout(column);
    layout->addStretch();

    m_wait.setInterval(1000);
    connect(&m_wait, &QTimer::timeout, this, &UnlockPage::tick);
    connect(m_unlock, &QPushButton::clicked, this, &UnlockPage::submit);
    connect(m_password, &QLineEdit::returnPressed, this, &UnlockPage::submit);
    connect(other, &QPushButton::clicked, this, &UnlockPage::otherVaultRequested);
    connect(create, &QPushButton::clicked, this, &UnlockPage::newVaultRequested);
}

void UnlockPage::setVault(const QString &path)
{
    Ui::setPath(m_file, QString(), path, 390);
}

void UnlockPage::reset()
{
    m_password->clear();
    m_password->setRevealed(false);
    m_error->clear();
    m_unlock->setText(I18n::t("unlock"));
    m_unlock->setEnabled(m_waitSeconds == 0);
    m_password->setFocus();
}

void UnlockPage::submit()
{
    if (m_waitSeconds > 0 || m_password->text().isEmpty())
        return;
    // Anahtar türetme bilerek yavaştır (~1 sn): beklendiği belli olsun
    m_unlock->setEnabled(false);
    m_unlock->setText(I18n::t("unlocking"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    emit unlockRequested(m_password->text());
    QApplication::restoreOverrideCursor();
}

void UnlockPage::unlockFailed(const QString &message)
{
    ++m_failures;
    m_password->selectAll();
    m_error->setText(message);
    m_unlock->setText(I18n::t("unlock"));
    if (m_failures >= FREE_ATTEMPTS) {
        // Tahmin denemelerini yavaşlatır: 5, 10, 20, 40, 60, 60… saniye
        m_waitSeconds = std::min(MAX_WAIT, 5 << std::min(m_failures - FREE_ATTEMPTS, 4));
        m_wait.start();
        tick();
    } else {
        m_unlock->setEnabled(true);
    }
}

void UnlockPage::tick()
{
    if (m_waitSeconds <= 0) {
        m_wait.stop();
        m_unlock->setEnabled(true);
        m_unlock->setText(I18n::t("unlock"));
        return;
    }
    m_unlock->setEnabled(false);
    m_unlock->setText(I18n::t("wait_seconds").replace("{0}", QString::number(m_waitSeconds)));
    --m_waitSeconds;
}
