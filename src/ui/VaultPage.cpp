#include "ui/VaultPage.h"

#include "core/Totp.h"
#include "core/VaultSession.h"
#include "ui/EntriesView.h"
#include "ui/GeneratorView.h"
#include "ui/HealthView.h"
#include "ui/TotpView.h"
#include "ui/UiHelpers.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

// Menü satırlarının rolü: kayıt listesi süzgeci ya da ayrı bir sayfa
enum Kind { AllEntries, Favorites, CategoryRow, Codes, Health, Generator };

} // namespace

VaultPage::VaultPage(VaultSession &session, QWidget *parent)
    : QWidget(parent), m_session(session)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QWidget(this);
    sidebar->setFixedWidth(220);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(10, 16, 10, 12);
    m_navigation = new QListWidget(sidebar);
    m_navigation->setObjectName("navigation");
    m_navigation->setFocusPolicy(Qt::NoFocus);
    side->addWidget(m_navigation, 1);
    auto *lock = Ui::accentButton("🔒  " + I18n::t("lock"), sidebar);
    auto *settings = new QPushButton("⚙  " + I18n::t("settings"), sidebar);
    auto *theme = new QPushButton((Theme::isDark() ? "☀  " : "☾  ") + I18n::t("theme"), sidebar);
    auto *language = new QPushButton("🌐  " + I18n::t("language"), sidebar);
    auto *about = new QPushButton("ⓘ  " + I18n::t("about"), sidebar);
    for (QPushButton *button : {settings, theme, language, about}) {
        button->setFlat(true);
        side->addWidget(button);
    }
    side->addWidget(lock);

    auto *right = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    m_pages = new QStackedWidget(right);
    m_entries = new EntriesView(session, m_pages);
    m_totp = new TotpView(session, m_pages);
    m_health = new HealthView(session, m_pages);
    m_generator = new GeneratorView(m_pages);
    for (QWidget *page : std::initializer_list<QWidget *>{m_entries, m_totp, m_health, m_generator})
        m_pages->addWidget(page);
    m_status = new QLabel(right);
    m_status->setObjectName("muted");
    m_status->setContentsMargins(20, 0, 20, 8);
    rightLayout->addWidget(m_pages, 1);
    rightLayout->addWidget(m_status);

    layout->addWidget(sidebar);
    layout->addWidget(right, 1);

    connect(m_navigation, &QListWidget::currentRowChanged, this, &VaultPage::showPage);
    connect(lock, &QPushButton::clicked, this, &VaultPage::lockRequested);
    connect(settings, &QPushButton::clicked, this, &VaultPage::settingsRequested);
    connect(theme, &QPushButton::clicked, this, &VaultPage::themeRequested);
    connect(language, &QPushButton::clicked, this, &VaultPage::languageRequested);
    connect(about, &QPushButton::clicked, this, &VaultPage::aboutRequested);
    connect(m_entries, &EntriesView::copyRequested, this, &VaultPage::copyRequested);
    connect(m_totp, &TotpView::copyRequested, this, &VaultPage::copyRequested);
    connect(m_generator, &GeneratorView::copyRequested, this, &VaultPage::copyRequested);
    connect(m_entries, &EntriesView::changed, this, &VaultPage::buildNavigation);
    connect(m_health, &HealthView::openEntry, this, [this](const QString &id) {
        m_navigation->setCurrentRow(0); // tüm kayıtlar
        m_entries->select(id);
    });
}

void VaultPage::buildNavigation()
{
    // Sayılar her değişiklikte yeniden hesaplanır; seçili satır korunur
    const int current = qMax(0, m_navigation->currentRow());
    const QList<Entry> &entries = m_session.vault().entries;
    int favorites = 0, withCodes = 0;
    QList<int> perCategory(static_cast<int>(Category::Other) + 1, 0);
    for (const Entry &e : entries) {
        favorites += e.favorite;
        withCodes += Totp::isValidSecret(e.totp);
        ++perCategory[static_cast<int>(e.category)];
    }
    auto add = [this](const QString &text, int count, int kind, int category = 0) {
        auto *item = new QListWidgetItem(count >= 0 ? QString("%1  (%2)").arg(text).arg(count) : text, m_navigation);
        item->setData(Qt::UserRole, kind);
        item->setData(Qt::UserRole + 1, category);
    };
    m_navigation->blockSignals(true);
    m_navigation->clear();
    add("🗂  " + I18n::t("all_entries"), static_cast<int>(entries.size()), AllEntries);
    add("★  " + I18n::t("favorites"), favorites, Favorites);
    for (int c = 0; c <= static_cast<int>(Category::Other); ++c)
        if (perCategory[c] > 0) // boş kategoriler menüyü kalabalıklaştırmasın
            add("    " + I18n::category(static_cast<Category>(c)), perCategory[c], CategoryRow, c);
    add("🔢  " + I18n::t("totp_codes"), withCodes, Codes);
    add("🩺  " + I18n::t("password_health"), -1, Health);
    add("🎲  " + I18n::t("generator"), -1, Generator);
    m_navigation->blockSignals(false);
    m_navigation->setCurrentRow(std::min(current, m_navigation->count() - 1));
}

void VaultPage::showPage(int row)
{
    QListWidgetItem *item = m_navigation->item(row);
    if (!item)
        return;
    const int kind = item->data(Qt::UserRole).toInt();
    switch (kind) {
    case AllEntries:
    case Favorites:
    case CategoryRow:
        m_entries->setFilter(kind == AllEntries  ? EntriesView::Filter::All
                             : kind == Favorites ? EntriesView::Filter::Favorites
                                                 : EntriesView::Filter::ByCategory,
                             static_cast<Category>(item->data(Qt::UserRole + 1).toInt()));
        m_pages->setCurrentWidget(m_entries);
        break;
    case Codes:
        m_totp->refresh();
        m_pages->setCurrentWidget(m_totp);
        break;
    case Health:
        m_health->refresh();
        m_pages->setCurrentWidget(m_health);
        break;
    default:
        m_pages->setCurrentWidget(m_generator);
        break;
    }
}

void VaultPage::reload()
{
    m_health->clearBreaches();
    m_navigation->setCurrentRow(-1);
    buildNavigation();
    m_navigation->setCurrentRow(0);
    showPage(0);
}

void VaultPage::clear()
{
    m_health->clearBreaches();
    m_navigation->clear();
    // Tablolardaki kayıt kopyaları da boşaltılır (kasa kilitliyken ekranda ya da bellekte kalmasın)
    m_entries->refresh();
    m_totp->refresh();
    m_health->refresh();
}

void VaultPage::tick()
{
    if (m_pages->currentWidget() == m_totp)
        m_totp->tick();
    else if (m_pages->currentWidget() == m_entries)
        m_entries->tick();
}

void VaultPage::showStatus(const QString &text)
{
    m_status->setText(text);
}
