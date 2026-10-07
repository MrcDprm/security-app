#pragma once

#include "core/Entry.h"

#include <QHash>
#include <QWidget>

class QLabel;
class QPushButton;
class QToolButton;
class QProgressBar;

// Seçili kaydın ayrıntıları: alanlar, kopyala düğmeleri, göster/gizle, adresi aç, canlı 2FA kodu.
class EntryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit EntryPanel(QWidget *parent);
    void showEntry(const Entry *entry); // nullptr: boş durum
    void tick();                        // her saniye: 2FA kodu ve kalan süre

signals:
    void copyRequested(const QString &text, const QString &what);
    void editRequested();
    void deleteRequested();
    void favoriteToggled();

private:

    Entry m_entry;
    bool m_hasEntry = false;
    bool m_revealed = false;
    QWidget *m_content;
    QLabel *m_empty, *m_avatar, *m_title, *m_category;
    QLabel *m_username, *m_password, *m_url, *m_code, *m_notes, *m_dates, *m_strength;
    QWidget *m_urlRow, *m_codeRow, *m_notesRow;
    QHash<QWidget *, QLabel *> m_rowLabels;
    QProgressBar *m_codeTime;
    QToolButton *m_reveal;
    QPushButton *m_favorite;
};
