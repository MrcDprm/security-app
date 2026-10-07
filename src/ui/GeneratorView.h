#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QRadioButton;
class QSlider;
class QSpinBox;

// Bağımsız şifre üretici: rastgele karakterler ya da rastgele kelimelerden parola.
// Ayarlar değişince yeni şifre hemen üretilir.
class GeneratorView : public QWidget
{
    Q_OBJECT

public:
    explicit GeneratorView(QWidget *parent);

signals:
    void copyRequested(const QString &text, const QString &what);

private:
    void generate();

    QRadioButton *m_characters, *m_words;
    QSlider *m_length;
    QSpinBox *m_lengthBox, *m_wordCount;
    QCheckBox *m_lower, *m_upper, *m_digits, *m_symbols, *m_ambiguous, *m_capitalize, *m_number;
    QComboBox *m_separator;
    QWidget *m_characterOptions, *m_wordOptions;
    QLineEdit *m_output;
    QProgressBar *m_strength;
    QLabel *m_strengthText;
};
