#include "ui/EntryPanel.h"

#include "core/Totp.h"
#include "ui/UiHelpers.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QGridLayout>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QToolButton *iconButton(const QString &text, const QString &tip, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setText(text);
    button->setToolTip(tip);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QLabel *caption(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName("muted");
    return label;
}

} // namespace

EntryPanel::EntryPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *frame = new QFrame(this);
    frame->setObjectName("panel");
    outer->addWidget(frame);
    auto *frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(20, 18, 20, 18);

    m_empty = new QLabel(I18n::t("select_entry"), frame);
    m_empty->setObjectName("muted");
    m_empty->setAlignment(Qt::AlignCenter);
    frameLayout->addWidget(m_empty);

    m_content = new QWidget(frame);
    frameLayout->addWidget(m_content, 1);
    auto *layout = new QVBoxLayout(m_content);
    layout->setContentsMargins(0, 0, 0, 0);

    // Başlık satırı: avatar, ad, kategori
    auto *header = new QHBoxLayout;
    m_avatar = new QLabel(m_content);
    m_title = new QLabel(m_content);
    m_title->setObjectName("heading");
    m_title->setTextFormat(Qt::PlainText); // kayıt adı kullanıcı verisi: HTML olarak yorumlanmaz
    m_title->setWordWrap(true);
    m_category = caption(QString(), m_content);
    auto *titles = new QVBoxLayout;
    titles->addWidget(m_title);
    titles->addWidget(m_category);
    header->addWidget(m_avatar);
    header->addLayout(titles, 1);
    layout->addLayout(header);
    layout->addSpacing(10);

    auto *grid = new QGridLayout;
    grid->setVerticalSpacing(6);
    int row = 0;
    auto addRow = [&](const QString &labelKey, QLabel *&value, QList<QToolButton *> buttons, QWidget **rowWidget) {
        auto *label = caption(I18n::t(labelKey.toLatin1().constData()), m_content);
        value = new QLabel(m_content);
        value->setTextFormat(Qt::PlainText);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setWordWrap(true);
        auto *holder = new QWidget(m_content);
        auto *h = new QHBoxLayout(holder);
        h->setContentsMargins(0, 0, 0, 0);
        h->addWidget(value, 1);
        for (QToolButton *b : buttons)
            h->addWidget(b);
        grid->addWidget(label, row, 0, Qt::AlignTop);
        grid->addWidget(holder, row, 1);
        if (rowWidget)
            *rowWidget = holder;
        ++row;
        return label;
    };

    auto *copyUser = iconButton("⧉", I18n::t("copy_username"), m_content);
    addRow("username", m_username, {copyUser}, nullptr);
    m_reveal = iconButton("👁", I18n::t("show_password"), m_content);
    auto *copyPassword = iconButton("⧉", I18n::t("copy_password"), m_content);
    addRow("password", m_password, {m_reveal, copyPassword}, nullptr);
    m_password->setObjectName("mono");
    m_strength = caption(QString(), m_content);
    grid->addWidget(m_strength, row++, 1);
    auto *open = iconButton("↗", I18n::t("open_url"), m_content);
    auto *copyUrl = iconButton("⧉", I18n::t("copy_url"), m_content);
    QLabel *urlLabel = addRow("url", m_url, {open, copyUrl}, &m_urlRow);
    auto *copyCode = iconButton("⧉", I18n::t("copy_code"), m_content);
    QLabel *codeLabel = addRow("totp_code", m_code, {copyCode}, &m_codeRow);
    m_code->setObjectName("code");
    m_codeTime = new QProgressBar(m_content);
    m_codeTime->setRange(0, Totp::PERIOD);
    m_codeTime->setTextVisible(false);
    grid->addWidget(m_codeTime, row++, 1);
    QLabel *notesLabel = addRow("notes", m_notes, {}, &m_notesRow);
    grid->setColumnStretch(1, 1);
    layout->addLayout(grid);
    // Boş alanların satırı (etiketiyle birlikte) gizlenir
    m_rowLabels = {{m_urlRow, urlLabel}, {m_codeRow, codeLabel}, {m_notesRow, notesLabel}};
    layout->addStretch();

    m_dates = caption(QString(), m_content);
    m_dates->setWordWrap(true);
    layout->addWidget(m_dates);
    auto *actions = new QHBoxLayout;
    m_favorite = new QPushButton(m_content);
    auto *edit = Ui::accentButton(I18n::t("edit"), m_content);
    auto *remove = new QPushButton(I18n::t("delete"), m_content);
    actions->addWidget(m_favorite);
    actions->addStretch();
    actions->addWidget(remove);
    actions->addWidget(edit);
    layout->addLayout(actions);

    connect(copyUser, &QToolButton::clicked, this, [this] { emit copyRequested(m_entry.username, "username"); });
    connect(copyPassword, &QToolButton::clicked, this, [this] { emit copyRequested(m_entry.password, "password"); });
    connect(copyUrl, &QToolButton::clicked, this, [this] { emit copyRequested(m_entry.url, "url"); });
    connect(copyCode, &QToolButton::clicked, this, [this] { emit copyRequested(Totp::codeNow(m_entry.totp), "code"); });
    connect(open, &QToolButton::clicked, this, [this] {
        // Sadece http/https: kayda yazılmış "file:" ya da "javascript:" adresi çalıştırılmaz
        const QUrl url(m_entry.url);
        if (url.scheme() == "http" || url.scheme() == "https")
            QDesktopServices::openUrl(url);
    });
    connect(m_reveal, &QToolButton::clicked, this, [this] {
        m_revealed = !m_revealed;
        showEntry(m_hasEntry ? &m_entry : nullptr);
    });
    connect(edit, &QPushButton::clicked, this, &EntryPanel::editRequested);
    connect(remove, &QPushButton::clicked, this, &EntryPanel::deleteRequested);
    connect(m_favorite, &QPushButton::clicked, this, &EntryPanel::favoriteToggled);
    showEntry(nullptr);
}

void EntryPanel::showEntry(const Entry *entry)
{
    if (!entry || !m_hasEntry || entry->id != m_entry.id)
        m_revealed = false; // başka kayda geçince şifre yeniden gizlenir
    m_hasEntry = entry != nullptr;
    m_empty->setVisible(!entry);
    m_content->setVisible(entry);
    if (!entry) {
        m_entry = Entry();
        return;
    }
    m_entry = *entry;
    m_avatar->setPixmap(Ui::avatar(entry->title, 48));
    m_title->setText(entry->title);
    m_category->setText(I18n::category(entry->category));
    m_username->setText(entry->username.isEmpty() ? "—" : entry->username);
    m_password->setText(m_revealed ? entry->password : QString(std::min<qsizetype>(entry->password.size(), 16), QChar(0x2022)));
    m_reveal->setToolTip(I18n::t(m_revealed ? "hide_password" : "show_password"));
    const Strength::Result strength = Strength::evaluate(entry->password);
    m_strength->setText(entry->password.isEmpty() ? QString() : I18n::strength(strength.level));
    m_strength->setStyleSheet("color: " + Ui::strengthColor(strength.level).name());
    m_url->setText(entry->url);
    m_notes->setText(entry->notes);
    for (QWidget *row : {m_urlRow, m_codeRow, m_notesRow}) {
        const bool visible = row == m_urlRow ? !entry->url.isEmpty()
                             : row == m_codeRow ? Totp::isValidSecret(entry->totp)
                                                : !entry->notes.isEmpty();
        row->setVisible(visible);
        m_rowLabels.value(row)->setVisible(visible);
    }
    m_codeTime->setVisible(Totp::isValidSecret(entry->totp));
    m_dates->setText(I18n::t("dates_line")
                         .replace("{0}", I18n::dateTime(entry->created))
                         .replace("{1}", I18n::dateTime(entry->passwordChanged)));
    m_favorite->setText(entry->favorite ? "★ " + I18n::t("unfavorite") : "☆ " + I18n::t("favorite"));
    tick();
}

void EntryPanel::tick()
{
    if (!m_hasEntry || !Totp::isValidSecret(m_entry.totp))
        return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QString code = Totp::codeNow(m_entry.totp);
    m_code->setText(code.left(3) + " " + code.mid(3)); // 123 456: okunması kolay
    m_codeTime->setValue(Totp::secondsLeft(now));
}
