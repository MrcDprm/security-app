#include "ui/HealthView.h"

#include "core/Health.h"
#include "core/VaultSession.h"
#include "services/BreachCheck.h"
#include "ui/UiHelpers.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {

const char *const COUNT_KEYS[] = {"weak_passwords", "reused_passwords", "old_passwords", "breached_passwords"};

} // namespace

HealthView::HealthView(VaultSession &session, QWidget *parent)
    : QWidget(parent), m_session(session), m_breach(new BreachCheck(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    auto *title = new QLabel(I18n::t("password_health"), this);
    title->setObjectName("title");
    layout->addWidget(title);

    // Üst sıra: puan kartı ve dört sayaç
    auto *cards = new QHBoxLayout;
    auto *scoreCard = new QFrame(this);
    scoreCard->setObjectName("panel");
    auto *scoreLayout = new QVBoxLayout(scoreCard);
    m_score = new QLabel(scoreCard);
    m_score->setStyleSheet("font-size: 30pt; font-weight: 700;");
    m_scoreText = new QLabel(I18n::t("health_score"), scoreCard);
    m_scoreText->setObjectName("muted");
    scoreLayout->addWidget(m_score);
    scoreLayout->addWidget(m_scoreText);
    cards->addWidget(scoreCard, 2);
    for (const char *key : COUNT_KEYS) {
        auto *card = new QFrame(this);
        card->setObjectName("panel");
        auto *cardLayout = new QVBoxLayout(card);
        auto *value = new QLabel("0", card);
        value->setObjectName("cardValue");
        auto *label = new QLabel(I18n::t(key), card);
        label->setObjectName("muted");
        cardLayout->addWidget(value);
        cardLayout->addWidget(label);
        cards->addWidget(card, 1);
        m_counts << value;
    }
    layout->addLayout(cards);

    // Sızıntı kontrolü
    auto *breachRow = new QHBoxLayout;
    m_status = new QLabel(I18n::t("breach_explain"), this);
    m_status->setObjectName("muted");
    m_status->setWordWrap(true);
    m_check = Ui::accentButton(I18n::t("check_breaches"), this);
    breachRow->addWidget(m_status, 1);
    breachRow->addWidget(m_check);
    layout->addLayout(breachRow);

    m_table = Ui::makeTable({I18n::t("title"), I18n::t("username"), I18n::t("problems")}, this);
    m_table->setIconSize(QSize(26, 26));
    m_table->verticalHeader()->setDefaultSectionSize(40);
    layout->addWidget(m_table, 1);

    connect(m_check, &QPushButton::clicked, this, &HealthView::checkBreaches);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        emit openEntry(m_table->item(item->row(), 0)->data(Qt::UserRole).toString());
    });
    connect(m_breach, &BreachCheck::progress, this, [this](int done, int total) {
        m_status->setText(I18n::t("breach_progress").replace("{0}", QString::number(done)).replace("{1}", QString::number(total)));
    });
    connect(m_breach, &BreachCheck::found, this, [this](const QString &id, int count) { m_breachCounts.insert(id, count); });
    connect(m_breach, &BreachCheck::finished, this, [this](bool ok) {
        m_check->setEnabled(true);
        m_breachChecked = ok;
        m_status->setText(ok ? I18n::t("breach_done") : I18n::t("breach_failed"));
        refresh();
    });
}

void HealthView::refresh()
{
    // Kontrolden sonra şifresi değişen kaydın eski sonucu sayılmaz
    QHash<QString, int> counts;
    for (const Entry &e : m_session.vault().entries)
        if (m_breachCounts.contains(e.id) && m_checkedHashes.value(e.id) == BreachCheck::sha1Hex(e.password))
            counts.insert(e.id, m_breachCounts.value(e.id));
    const Health::Report report = Health::analyze(m_session.vault().entries, counts, QDateTime::currentDateTimeUtc());
    m_score->setText(QString::number(report.score));
    m_score->setStyleSheet(QString("font-size: 30pt; font-weight: 700; color: %1;")
                               .arg((report.score >= 80 ? Theme::success()
                                     : report.score >= 50 ? Theme::warning()
                                                          : Theme::danger()).name()));
    const int totals[] = {int(report.weak.size()), int(report.reused.size()), int(report.old.size()),
                          int(report.breached.size())};
    for (int i = 0; i < 4; ++i) {
        // Sızıntı kontrolü yapılmadıysa sayı yerine "—"
        m_counts[i]->setText(i == 3 && !m_breachChecked ? "—" : QString::number(totals[i]));
        m_counts[i]->setStyleSheet(totals[i] > 0 ? "color: " + Theme::danger().name() : "");
    }

    m_table->setRowCount(0);
    for (const Entry &e : m_session.vault().entries) {
        QStringList problems;
        if (report.breached.contains(e.id))
            problems << I18n::t("problem_breached").replace("{0}", QLocale().toString(counts.value(e.id)));
        if (report.weak.contains(e.id))
            problems << I18n::t("problem_weak");
        if (report.reused.contains(e.id))
            problems << I18n::t("problem_reused");
        if (report.old.contains(e.id))
            problems << I18n::t("problem_old");
        if (problems.isEmpty())
            continue;
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *title = new QTableWidgetItem(QIcon(Ui::avatar(e.title, 26)), e.title);
        title->setData(Qt::UserRole, e.id);
        m_table->setItem(row, 0, title);
        m_table->setItem(row, 1, new QTableWidgetItem(e.username));
        auto *problemItem = new QTableWidgetItem(problems.join(" · "));
        if (report.breached.contains(e.id) || report.weak.contains(e.id))
            problemItem->setForeground(Theme::danger());
        m_table->setItem(row, 2, problemItem);
    }
    m_table->sortItems(0);
    m_table->resizeColumnToContents(0);
    m_table->resizeColumnToContents(1);
}

void HealthView::clearBreaches()
{
    m_breach->cancel();
    m_breachCounts.clear();
    m_checkedHashes.clear();
    m_breachChecked = false;
    m_check->setEnabled(true);
    m_status->setText(I18n::t("breach_explain"));
}

void HealthView::checkBreaches()
{
    // İnternete çıkmadan önce ne gönderileceği açıkça söylenir ve onay alınır
    if (!Ui::ask(this, I18n::t("breach_confirm"), true))
        return;
    QList<QPair<QString, QString>> passwords;
    m_breachCounts.clear();
    m_checkedHashes.clear();
    for (const Entry &e : m_session.vault().entries) {
        passwords << qMakePair(e.id, e.password);
        m_checkedHashes.insert(e.id, BreachCheck::sha1Hex(e.password));
    }
    m_check->setEnabled(false);
    m_breach->start(passwords);
}
