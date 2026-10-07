#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class QTableWidget;
class VaultSession;

// 2FA anahtarı olan bütün kayıtların o anki kodları; her saniye güncellenir. Çift tıklayınca kod kopyalanır.
class TotpView : public QWidget
{
    Q_OBJECT

public:
    TotpView(VaultSession &session, QWidget *parent);
    void refresh();
    void tick();

signals:
    void copyRequested(const QString &text, const QString &what);

private:
    VaultSession &m_session;
    QTableWidget *m_table;
    QProgressBar *m_time;
    QLabel *m_seconds, *m_empty;
};
