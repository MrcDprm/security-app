#include "services/SecureClipboard.h"

#include <QClipboard>
#include <QCryptographicHash>
#include <QGuiApplication>
#include <QMimeData>

namespace {

QString hashOf(const QString &text)
{
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// Qt, bu biçimdeki MIME türlerini Windows'ta aynı adlı pano biçimi olarak kaydeder
QString windowsFormat(const char *name)
{
    return QString("application/x-qt-windows-mime;value=\"%1\"").arg(name);
}

} // namespace

SecureClipboard::SecureClipboard(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SecureClipboard::clearIfOurs);
}

void SecureClipboard::copy(const QString &text, int clearAfterSeconds)
{
    auto *mime = new QMimeData;
    mime->setText(text);
    // Pano geçmişi ve bulut eşitlemesi bu veriyi almasın (Windows 10 1809+)
    mime->setData(windowsFormat("ExcludeClipboardContentFromMonitorProcessing"), QByteArray(4, 0));
    mime->setData(windowsFormat("CanIncludeInClipboardHistory"), QByteArray(4, 0));
    mime->setData(windowsFormat("CanUploadToCloudClipboard"), QByteArray(4, 0));
    QGuiApplication::clipboard()->setMimeData(mime); // pano sahipliği alır, silmeyi o yapar
    m_copiedHash = hashOf(text);
    m_timer.start(clearAfterSeconds * 1000);
}

void SecureClipboard::clearNow()
{
    m_timer.stop();
    clearIfOurs();
}

int SecureClipboard::secondsLeft() const
{
    return m_timer.isActive() ? (m_timer.remainingTime() + 999) / 1000 : 0;
}

void SecureClipboard::clearIfOurs()
{
    // Kullanıcı bu arada başka bir şey kopyaladıysa ona dokunulmaz
    if (!m_copiedHash.isEmpty() && hashOf(QGuiApplication::clipboard()->text()) == m_copiedHash) {
        QGuiApplication::clipboard()->clear();
        emit cleared();
    }
    m_copiedHash.clear();
}
