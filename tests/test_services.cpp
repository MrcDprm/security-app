#include "app/Settings.h"
#include "services/BreachCheck.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

class TestServices : public QObject
{
    Q_OBJECT

private slots:
    void sha1IsUppercaseHex()
    {
        // "password" kelimesinin SHA-1 özeti (HIBP belgelerindeki örnek)
        QCOMPARE(BreachCheck::sha1Hex("password"), QByteArray("5BAA61E4C9B93F3F0682250B6CF8331B7EE68FD8"));
    }

    void responseIsParsed()
    {
        const QByteArray body = "0018A45C4D1DEF81644B54AB7F969B88D65:1\r\n"
                                "1E4C9B93F3F0682250B6CF8331B7EE68FD8:9659365\r\n"
                                "011053FD0102E94D6AE2F8B83D76FAF94F6:0\r\n";
        QCOMPARE(BreachCheck::countIn(body, "1E4C9B93F3F0682250B6CF8331B7EE68FD8"), 9659365);
        QCOMPARE(BreachCheck::countIn(body, "011053FD0102E94D6AE2F8B83D76FAF94F6"), 0); // dolgu satırı
        QCOMPARE(BreachCheck::countIn(body, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"), 0);
        QCOMPARE(BreachCheck::countIn("bozuk yanıt", "1E4C9B93F3F0682250B6CF8331B7EE68FD8"), 0);
    }

    void settingsAreValidated()
    {
        QTemporaryDir dir;
        Settings settings(dir.path());
        QCOMPARE(settings.autoLockMinutes(), 5);
        QCOMPARE(settings.clipboardSeconds(), 30);
        settings.setAutoLockMinutes(500); // sınırın dışı kırpılır
        QCOMPARE(settings.autoLockMinutes(), 60);
        // Dosya elle bozulmuşsa varsayılana dönülür
        QSettings raw(dir.filePath("settings.ini"), QSettings::IniFormat);
        raw.setValue("clipboard_seconds", "asla");
        raw.setValue("language", "de");
        raw.setValue("last_vault", dir.filePath("olmayan.vault"));
        raw.sync();
        QCOMPARE(settings.clipboardSeconds(), 30);
        QCOMPARE(settings.language(), QString("tr"));
        QCOMPARE(settings.lastVault(), QString()); // olmayan dosya hatırlanmaz
    }
};

QTEST_GUILESS_MAIN(TestServices)
#include "test_services.moc"
