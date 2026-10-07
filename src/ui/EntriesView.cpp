#include "ui/EntriesView.h"

#include "core/VaultSession.h"
#include "ui/EntryDialog.h"
#include "ui/EntryPanel.h"
#include "ui/UiHelpers.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QShortcut>
#include <QSplitter>
#include <QVBoxLayout>

EntriesView::EntriesView(VaultSession &session, QWidget *parent)
    : QWidget(parent), m_session(session)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    auto *toolbar = new QHBoxLayout;
    m_heading = new QLabel(this);
    m_heading->setObjectName("title");
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(I18n::t("search_entries"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(260);
    auto *add = Ui::accentButton("+ " + I18n::t("new_entry"), this);
    toolbar->addWidget(m_heading);
    toolbar->addStretch();
    toolbar->addWidget(m_search);
    toolbar->addWidget(add);
    layout->addLayout(toolbar);

    auto *splitter = new QSplitter(this);
    m_table = Ui::makeTable({I18n::t("title"), I18n::t("username"), I18n::t("category")}, splitter);
    m_table->setIconSize(QSize(26, 26));
    m_table->verticalHeader()->setDefaultSectionSize(40);
    m_panel = new EntryPanel(splitter);
    splitter->addWidget(m_table);
    splitter->addWidget(m_panel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    connect(m_search, &QLineEdit::textChanged, this, &EntriesView::refresh);
    connect(add, &QPushButton::clicked, this, &EntriesView::addEntry);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] { m_panel->showEntry(selected()); });
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &EntriesView::editEntry);
    connect(m_panel, &EntryPanel::copyRequested, this, &EntriesView::copyRequested);
    connect(m_panel, &EntryPanel::editRequested, this, &EntriesView::editEntry);
    connect(m_panel, &EntryPanel::deleteRequested, this, &EntriesView::deleteEntry);
    connect(m_panel, &EntryPanel::favoriteToggled, this, &EntriesView::toggleFavorite);
    // Kısayollar: Ctrl+F ara, Ctrl+N yeni, Ctrl+C seçili kaydın şifresini kopyala
    connect(new QShortcut(QKeySequence::Find, this), &QShortcut::activated, m_search, qOverload<>(&QWidget::setFocus));
    connect(new QShortcut(QKeySequence::New, this), &QShortcut::activated, this, &EntriesView::addEntry);
    auto *copy = new QShortcut(QKeySequence::Copy, m_table);
    copy->setContext(Qt::WidgetShortcut);
    connect(copy, &QShortcut::activated, this, [this] {
        if (const Entry *e = selected())
            emit copyRequested(e->password, "password");
    });
}

void EntriesView::setFilter(Filter filter, Category category)
{
    m_filter = filter;
    m_category = category;
    refresh();
}

void EntriesView::refresh()
{
    const QString previous = selected() ? selected()->id : QString();
    m_heading->setText(m_filter == Filter::All         ? I18n::t("all_entries")
                       : m_filter == Filter::Favorites ? I18n::t("favorites")
                                                       : I18n::category(m_category));
    // Arama başlık, kullanıcı adı, adres ve notlarda yapılır; şifrenin kendisinde aranmaz
    const QString search = m_search->text().trimmed();
    QList<const Entry *> shown;
    for (const Entry &e : m_session.vault().entries) {
        if ((m_filter == Filter::Favorites && !e.favorite) || (m_filter == Filter::ByCategory && e.category != m_category))
            continue;
        if (!search.isEmpty()
            && !(e.title + ' ' + e.username + ' ' + e.url + ' ' + e.notes).contains(search, Qt::CaseInsensitive))
            continue;
        shown << &e;
    }
    std::sort(shown.begin(), shown.end(), [](const Entry *a, const Entry *b) {
        return QString::localeAwareCompare(a->title, b->title) < 0;
    });

    m_table->setRowCount(0);
    for (const Entry *e : shown) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *title = new QTableWidgetItem(QIcon(Ui::avatar(e->title, 26)), (e->favorite ? "★ " : "") + e->title);
        title->setData(Qt::UserRole, e->id); // satırın hangi kayıt olduğu (UUID)
        m_table->setItem(row, 0, title);
        m_table->setItem(row, 1, new QTableWidgetItem(e->username));
        m_table->setItem(row, 2, new QTableWidgetItem(I18n::category(e->category)));
    }
    m_table->resizeColumnToContents(0);
    m_table->resizeColumnToContents(1);
    select(previous);
    if (!selected())
        m_panel->showEntry(nullptr);
}

void EntriesView::select(const QString &id)
{
    for (int row = 0; row < m_table->rowCount(); ++row)
        if (m_table->item(row, 0)->data(Qt::UserRole).toString() == id) {
            m_table->selectRow(row);
            return;
        }
}

const Entry *EntriesView::selected() const
{
    const QList<QTableWidgetItem *> items = m_table->selectedItems();
    if (items.isEmpty())
        return nullptr;
    const QString id = m_table->item(items.first()->row(), 0)->data(Qt::UserRole).toString();
    for (const Entry &e : m_session.vault().entries)
        if (e.id == id)
            return &e;
    return nullptr;
}

void EntriesView::addEntry()
{
    Entry entry;
    if (m_filter == Filter::ByCategory)
        entry.category = m_category; // açık kategoriye eklenir
    entry.favorite = m_filter == Filter::Favorites;
    EntryDialog dialog(entry, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString error = m_session.add(dialog.entry());
    if (!error.isEmpty()) {
        Ui::warn(this, I18n::error(error));
        return;
    }
    m_search->clear();
    refresh();
    select(m_session.vault().entries.last().id);
    emit changed();
}

void EntriesView::editEntry()
{
    const Entry *entry = selected();
    if (!entry)
        return;
    EntryDialog dialog(*entry, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString error = m_session.update(dialog.entry());
    if (!error.isEmpty())
        Ui::warn(this, I18n::error(error));
    refresh();
    emit changed();
}

void EntriesView::deleteEntry()
{
    const Entry *entry = selected();
    if (!entry || !Ui::ask(this, I18n::t("confirm_delete").replace("{0}", entry->title)))
        return;
    const QString error = m_session.remove(entry->id);
    if (!error.isEmpty())
        Ui::warn(this, I18n::error(error));
    refresh();
    emit changed();
}

void EntriesView::toggleFavorite()
{
    const Entry *entry = selected();
    if (!entry)
        return;
    Entry changedEntry = *entry;
    changedEntry.favorite = !changedEntry.favorite;
    const QString error = m_session.update(changedEntry);
    if (!error.isEmpty())
        Ui::warn(this, I18n::error(error));
    refresh();
    emit changed();
}

void EntriesView::tick()
{
    m_panel->tick();
}
