#include "ui/GeneratorView.h"

#include "core/Generator.h"
#include "ui/UiHelpers.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

GeneratorView::GeneratorView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    auto *title = new QLabel(I18n::t("generator"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    // Sonuç kutusu
    auto *result = new QFrame(this);
    result->setObjectName("panel");
    auto *resultLayout = new QVBoxLayout(result);
    resultLayout->setContentsMargins(18, 16, 18, 16);
    m_output = new QLineEdit(result);
    m_output->setReadOnly(true);
    m_output->setStyleSheet("font-family: Consolas; font-size: 16pt; padding: 8px;");
    auto *again = new QPushButton("↻ " + I18n::t("regenerate"), result);
    auto *copy = Ui::accentButton(I18n::t("copy"), result);
    auto *outputRow = new QHBoxLayout;
    outputRow->addWidget(m_output, 1);
    outputRow->addWidget(again);
    outputRow->addWidget(copy);
    m_strength = new QProgressBar(result);
    m_strengthText = new QLabel(result);
    m_strengthText->setObjectName("muted");
    resultLayout->addLayout(outputRow);
    resultLayout->addWidget(m_strength);
    resultLayout->addWidget(m_strengthText);
    layout->addWidget(result);

    // Tür seçimi
    m_characters = new QRadioButton(I18n::t("random_characters"), this);
    m_words = new QRadioButton(I18n::t("random_words"), this);
    m_characters->setChecked(true);
    auto *kinds = new QHBoxLayout;
    kinds->addWidget(m_characters);
    kinds->addWidget(m_words);
    kinds->addStretch();
    layout->addSpacing(8);
    layout->addLayout(kinds);

    // Karakter ayarları
    m_characterOptions = new QWidget(this);
    auto *characterForm = new QFormLayout(m_characterOptions);
    m_length = new QSlider(Qt::Horizontal, m_characterOptions);
    m_length->setRange(Generator::MIN_LENGTH, 64);
    m_length->setValue(20);
    m_lengthBox = new QSpinBox(m_characterOptions);
    m_lengthBox->setRange(Generator::MIN_LENGTH, Generator::MAX_LENGTH);
    m_lengthBox->setValue(20);
    auto *lengthRow = new QHBoxLayout;
    lengthRow->addWidget(m_length, 1);
    lengthRow->addWidget(m_lengthBox);
    m_lower = new QCheckBox("a–z", m_characterOptions);
    m_upper = new QCheckBox("A–Z", m_characterOptions);
    m_digits = new QCheckBox("0–9", m_characterOptions);
    m_symbols = new QCheckBox("!@#$%", m_characterOptions);
    m_ambiguous = new QCheckBox(I18n::t("avoid_ambiguous"), m_characterOptions);
    for (QCheckBox *box : {m_lower, m_upper, m_digits, m_symbols, m_ambiguous})
        box->setChecked(true);
    auto *sets = new QHBoxLayout;
    for (QCheckBox *box : {m_lower, m_upper, m_digits, m_symbols})
        sets->addWidget(box);
    sets->addStretch();
    characterForm->addRow(I18n::t("length"), lengthRow);
    characterForm->addRow(I18n::t("characters"), sets);
    characterForm->addRow(QString(), m_ambiguous);
    layout->addWidget(m_characterOptions);

    // Kelime ayarları
    m_wordOptions = new QWidget(this);
    auto *wordForm = new QFormLayout(m_wordOptions);
    m_wordCount = new QSpinBox(m_wordOptions);
    m_wordCount->setRange(Generator::MIN_WORDS, Generator::MAX_WORDS);
    m_wordCount->setValue(5);
    m_separator = new QComboBox(m_wordOptions);
    m_separator->addItem("-  (kedi-pencere)", "-");
    m_separator->addItem(".  (kedi.pencere)", ".");
    m_separator->addItem("_  (kedi_pencere)", "_");
    m_separator->addItem(I18n::t("space"), " ");
    m_capitalize = new QCheckBox(I18n::t("capitalize"), m_wordOptions);
    m_capitalize->setChecked(true);
    m_number = new QCheckBox(I18n::t("add_number"), m_wordOptions);
    m_number->setChecked(true);
    wordForm->addRow(I18n::t("word_count"), m_wordCount);
    wordForm->addRow(I18n::t("separator"), m_separator);
    wordForm->addRow(QString(), m_capitalize);
    wordForm->addRow(QString(), m_number);
    auto *credit = new QLabel(I18n::t("wordlist_credit"), m_wordOptions);
    credit->setObjectName("muted");
    wordForm->addRow(QString(), credit);
    layout->addWidget(m_wordOptions);
    layout->addStretch();

    // Kaydırıcı ve sayı kutusu birbirini izler
    connect(m_length, &QSlider::valueChanged, m_lengthBox, &QSpinBox::setValue);
    connect(m_lengthBox, &QSpinBox::valueChanged, this, [this](int value) {
        m_length->blockSignals(true);
        m_length->setValue(value);
        m_length->blockSignals(false);
        generate();
    });
    for (QCheckBox *box : {m_lower, m_upper, m_digits, m_symbols, m_ambiguous, m_capitalize, m_number})
        connect(box, &QCheckBox::toggled, this, &GeneratorView::generate);
    connect(m_wordCount, &QSpinBox::valueChanged, this, &GeneratorView::generate);
    connect(m_separator, &QComboBox::currentIndexChanged, this, &GeneratorView::generate);
    connect(m_characters, &QRadioButton::toggled, this, &GeneratorView::generate);
    connect(again, &QPushButton::clicked, this, &GeneratorView::generate);
    connect(copy, &QPushButton::clicked, this, [this] { emit copyRequested(m_output->text(), "password"); });
    generate();
}

void GeneratorView::generate()
{
    const bool characters = m_characters->isChecked();
    m_characterOptions->setVisible(characters);
    m_wordOptions->setVisible(!characters);
    if (characters) {
        Generator::Options options;
        options.length = m_lengthBox->value();
        options.lower = m_lower->isChecked();
        options.upper = m_upper->isChecked();
        options.digits = m_digits->isChecked();
        options.symbols = m_symbols->isChecked();
        options.avoidAmbiguous = m_ambiguous->isChecked();
        m_output->setText(Generator::password(options));
    } else {
        m_output->setText(Generator::passphrase(m_wordCount->value(), m_separator->currentData().toString(),
                                                m_capitalize->isChecked(), m_number->isChecked()));
    }
    Ui::showStrength(m_strength, m_strengthText, m_output->text());
}
