#include "ui/TotpView.h"

#include "core/Totp.h"
#include "core/VaultSession.h"
#include "ui/UiHelpers.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {

constexpr int CODE_COLUMN = 2;

} // namespace

TotpView::TotpView(VaultSession &session, QWidget *parent)
    : QWidget(parent), m_session(session)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    auto *title = new QLabel(I18n::t("totp_codes"), this);
    title->setObjectName("title");
    auto *hint = new QLabel(I18n::t("totp_hint"), this);
    hint->setObjectName("muted");
    m_time = new QProgressBar(this);
    m_time->setRange(0, Totp::PERIOD);
    m_time->setTextVisible(false);
    m_time->setFixedWidth(160);
    m_seconds = new QLabel(this);
    m_seconds->setObjectName("muted");
    auto *header = new QHBoxLayout;
    header->addWidget(title);
    header->addStretch();
    header->addWidget(m_seconds);
    header->addWidget(m_time);
    layout->addLayout(header);
    layout->addWidget(hint);

    m_table = Ui::makeTable({I18n::t("title"), I18n::t("username"), I18n::t("totp_code")}, this);
    m_table->setIconSize(QSize(26, 26));
    m_table->verticalHeader()->setDefaultSectionSize(44);
    layout->addWidget(m_table, 1);
    m_empty = new QLabel(I18n::t("no_totp"), this);
    m_empty->setObjectName("muted");
    m_empty->setWordWrap(true);
    layout->addWidget(m_empty);

    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        const QString secret = m_table->item(item->row(), 0)->data(Qt::UserRole).toString();
        emit copyRequested(Totp::codeNow(secret), "code");
    });
}

void TotpView::refresh()
{
    m_table->setRowCount(0);
    for (const Entry &e : m_session.vault().entries) {
        if (!Totp::isValidSecret(e.totp))
            continue;
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *title = new QTableWidgetItem(QIcon(Ui::avatar(e.title, 26)), e.title);
        title->setData(Qt::UserRole, e.totp); // tablo kapanınca bu kopya da gider; kasa kilitlenince tablo boşaltılır
        m_table->setItem(row, 0, title);
        m_table->setItem(row, 1, new QTableWidgetItem(e.username));
        auto *code = new QTableWidgetItem;
        QFont font("Consolas");
        font.setPointSize(15);
        font.setBold(true);
        code->setFont(font);
        code->setForeground(Theme::accent());
        m_table->setItem(row, CODE_COLUMN, code);
    }
    m_table->sortItems(0);
    m_table->resizeColumnToContents(0);
    m_table->resizeColumnToContents(1);
    m_empty->setVisible(m_table->rowCount() == 0);
    tick();
}

void TotpView::tick()
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const int left = Totp::secondsLeft(now);
    m_time->setValue(left);
    m_seconds->setText(I18n::t("seconds_left").replace("{0}", QString::number(left)));
    // Son 5 saniyede kod kırmızı: kopyalayıp yapıştırmaya yetmeyebilir
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QString code = Totp::codeNow(m_table->item(row, 0)->data(Qt::UserRole).toString());
        QTableWidgetItem *item = m_table->item(row, CODE_COLUMN);
        item->setText(code.left(3) + " " + code.mid(3));
        item->setForeground(left <= 5 ? Theme::danger() : Theme::accent());
    }
}
