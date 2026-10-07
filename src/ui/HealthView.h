#pragma once

#include <QHash>
#include <QWidget>

class BreachCheck;
class QLabel;
class QPushButton;
class QTableWidget;
class VaultSession;

// Şifre sağlığı: genel puan, dört sayaç (zayıf, tekrarlı, eski, sızıntıda) ve sorunlu kayıtların listesi.
// Sızıntı kontrolü isteğe bağlıdır ve onay alınmadan internete çıkılmaz.
class HealthView : public QWidget
{
    Q_OBJECT

public:
    HealthView(VaultSession &session, QWidget *parent);
    void refresh();
    void clearBreaches(); // kilitlenince sızıntı sonuçları da unutulur

signals:
    void openEntry(const QString &id);

private:
    void checkBreaches();

    VaultSession &m_session;
    BreachCheck *m_breach;
    QHash<QString, int> m_breachCounts;
    QHash<QString, QByteArray> m_checkedHashes; // kontrol anındaki şifre özeti: şifre sonradan değişirse sonuç geçersiz
    bool m_breachChecked = false;
    QLabel *m_score, *m_scoreText, *m_status;
    QList<QLabel *> m_counts;
    QPushButton *m_check;
    QTableWidget *m_table;
};
