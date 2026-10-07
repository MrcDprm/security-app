#pragma once

#include "core/Entry.h"

#include <QWidget>

class EntryPanel;
class QLabel;
class QLineEdit;
class QTableWidget;
class VaultSession;

// Kayıt listesi (arama + tablo) ve sağda seçili kaydın ayrıntıları.
// Liste "tümü", "favoriler" ya da bir kategoriye göre süzülür.
class EntriesView : public QWidget
{
    Q_OBJECT

public:
    enum class Filter { All, Favorites, ByCategory };

    EntriesView(VaultSession &session, QWidget *parent);
    void setFilter(Filter filter, Category category = Category::General);
    void refresh();
    void select(const QString &id);
    void addEntry();
    void tick();

signals:
    void copyRequested(const QString &text, const QString &what);
    void changed(); // kayıt eklendi, değişti ya da silindi (kenar çubuğu sayıları güncellensin)

private:
    const Entry *selected() const;
    void editEntry();
    void deleteEntry();
    void toggleFavorite();

    VaultSession &m_session;
    Filter m_filter = Filter::All;
    Category m_category = Category::General;
    QLabel *m_heading;
    QLineEdit *m_search;
    QTableWidget *m_table;
    EntryPanel *m_panel;
};
