#include "core/Health.h"

#include "core/Strength.h"

namespace Health {

Report analyze(const QList<Entry> &entries, const QHash<QString, int> &breachCounts, const QDateTime &now)
{
    Report report;
    // Aynı şifreyi kullanan kayıtları bulmak için şifre → kimlikler
    QHash<QString, QStringList> byPassword;
    for (const Entry &e : entries) {
        if (e.password.isEmpty())
            continue;
        ++report.checked;
        byPassword[e.password] << e.id;
        if (Strength::evaluate(e.password).level <= Strength::Level::Weak)
            report.weak.insert(e.id);
        if (e.passwordChanged.isValid() && e.passwordChanged.daysTo(now) > OLD_AFTER_DAYS)
            report.old.insert(e.id);
        if (breachCounts.value(e.id) > 0)
            report.breached.insert(e.id);
    }
    for (const QStringList &ids : std::as_const(byPassword))
        if (ids.size() > 1)
            for (const QString &id : ids)
                report.reused.insert(id);

    if (report.checked > 0) {
        const QSet<QString> problems = report.weak + report.reused + report.old + report.breached;
        report.score = qRound(100.0 * (report.checked - problems.size()) / report.checked);
    }
    return report;
}

} // namespace Health
