#pragma once

#include "core/Entry.h"

#include <QDialog>

class PasswordField;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;

// Kayıt ekleme ve düzenleme formu. "Üret" düğmesi güçlü bir şifre koyar; güç çubuğu yazarken güncellenir.
class EntryDialog : public QDialog
{
    Q_OBJECT

public:
    EntryDialog(const Entry &entry, QWidget *parent);
    Entry entry() const { return m_entry; }

private:
    void save();

    Entry m_entry;
    QLineEdit *m_title, *m_username, *m_url, *m_totp;
    PasswordField *m_password;
    QProgressBar *m_strength;
    QLabel *m_strengthText, *m_error;
    QComboBox *m_category;
    QPlainTextEdit *m_notes;
    QCheckBox *m_favorite;
};
