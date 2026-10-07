#pragma once

#include "app/I18n.h"
#include "app/Theme.h"
#include "core/Strength.h"

#include <QDir>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>

// Sayfaların ortak küçük yardımcıları: tablo kurulumu, düğmeler, soru kutuları, güç çubuğu, harf avatarı.
namespace Ui {

inline QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(0, headers.size(), parent);
    table->setHorizontalHeaderLabels(headers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->hide();
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->setWordWrap(false);
    return table;
}

inline QPushButton *accentButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setProperty("accent", true);
    return button;
}

// Qt'nin hazır düğmeleri çeviri dosyası olmadan İngilizce görünür; düğmeler kendi metinlerimizle kurulur
inline bool ask(QWidget *parent, const QString &text, bool defaultYes = false)
{
    QMessageBox box(QMessageBox::Question, I18n::t("app_name"), text, QMessageBox::NoButton, parent);
    box.setTextFormat(Qt::PlainText); // kayıt adı gibi kullanıcı verisi HTML olarak yorumlanmasın
    QPushButton *yes = box.addButton(I18n::t("yes"), QMessageBox::YesRole);
    QPushButton *no = box.addButton(I18n::t("no"), QMessageBox::NoRole);
    box.setDefaultButton(defaultYes ? yes : no);
    box.exec();
    return box.clickedButton() == yes;
}

inline void message(QWidget *parent, QMessageBox::Icon icon, const QString &text)
{
    QMessageBox box(icon, I18n::t("app_name"), text, QMessageBox::NoButton, parent);
    box.setTextFormat(Qt::PlainText);
    box.addButton(I18n::t("ok"), QMessageBox::AcceptRole);
    box.exec();
}

inline void inform(QWidget *parent, const QString &text)
{
    message(parent, QMessageBox::Information, text);
}

inline void warn(QWidget *parent, const QString &text)
{
    message(parent, QMessageBox::Warning, text);
}

inline QColor strengthColor(Strength::Level level)
{
    switch (level) {
    case Strength::Level::VeryWeak:
    case Strength::Level::Weak: return Theme::danger();
    case Strength::Level::Fair: return Theme::warning();
    default: return Theme::success();
    }
}

// Güç çubuğu ve altındaki açıklama birlikte güncellenir
inline void showStrength(QProgressBar *bar, QLabel *label, const QString &password)
{
    const Strength::Result result = Strength::evaluate(password);
    const int level = static_cast<int>(result.level);
    bar->setRange(0, 4);
    bar->setValue(password.isEmpty() ? 0 : level + (level < 4 ? 1 : 0));
    bar->setTextVisible(false);
    bar->setStyleSheet(QString("QProgressBar::chunk { background: %1; border-radius: 3px; }")
                           .arg(strengthColor(result.level).name()));
    QString text = password.isEmpty() ? QString() : I18n::strength(result.level);
    if (!password.isEmpty() && !result.hint.isEmpty())
        text += " · " + I18n::t(result.hint.toLatin1().constData());
    label->setText(text);
}

// Uzun dosya yolu tek satıra sığacak şekilde ortadan kısaltılır; tam yol ipucunda görünür
inline void setPath(QLabel *label, const QString &prefix, const QString &path, int width)
{
    const QString full = QDir::toNativeSeparators(path);
    label->setText(label->fontMetrics().elidedText(prefix + full, Qt::ElideMiddle, width));
    label->setToolTip(full.toHtmlEscaped());
}

// Kayıt başlığının ilk harfiyle renkli yuvarlak (site ikonu yerine; ağdan ikon indirilmez)
inline QPixmap avatar(const QString &title, int size)
{
    static const char *const colors[] = {"#4c6ef5", "#15aabf", "#12b886", "#82c91e", "#fab005",
                                         "#fd7e14", "#fa5252", "#e64980", "#be4bdb", "#7950f2"};
    const QString letter = title.trimmed().isEmpty() ? "?" : title.trimmed().left(1).toUpper();
    const qreal ratio = 2;
    QPixmap pixmap(QSize(size, size) * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(colors[qHash(title.trimmed().toLower()) % 10]));
    p.drawEllipse(QRectF(0, 0, size, size));
    QFont font("Segoe UI");
    font.setPixelSize(size / 2);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, letter);
    return pixmap;
}

} // namespace Ui
