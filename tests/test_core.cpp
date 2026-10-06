#include "core/CsvImport.h"
#include "core/Crypto.h"
#include "core/DemoVault.h"
#include "core/Generator.h"
#include "core/Health.h"
#include "core/Strength.h"
#include "core/Totp.h"
#include "core/VaultSession.h"

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

// Testlerde Argon2 hızlı ayarla çalışır; gerçek kasada ~256 MB ve yaklaşık bir saniye sürer.
class TestCore : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    const QString m_password = "dogru-at-pil-zimba";

    QString path(const QString &name) const { return m_dir.filePath(name); }

    Entry sample(const QString &title = "GitHub")
    {
        Entry e;
        e.title = title;
        e.username = "ayse";
        e.password = "S3cret!pass";
        e.url = "https://github.com";
        e.notes = "çok satırlı\nnot";
        e.category = Category::Work;
        return e;
    }

    QByteArray readFile(const QString &file)
    {
        QFile f(file);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }

    void writeFile(const QString &file, const QByteArray &data)
    {
        QFile f(file);
        if (f.open(QIODevice::WriteOnly))
            f.write(data);
    }

private slots:
    void initTestCase() { QVERIFY(Crypto::init()); }

    void encryptionRoundTrip()
    {
        Crypto::SecretKey key, other;
        const QByteArray salt = Crypto::randomBytes(Crypto::SALT_BYTES);
        QVERIFY(Crypto::deriveKey("parola", salt, Crypto::fastParams(), key));
        QVERIFY(Crypto::deriveKey("parolA", salt, Crypto::fastParams(), other));
        QVERIFY(!Crypto::sameKey(key, other)); // tek harf farkı tamamen farklı anahtar

        const QByteArray nonce = Crypto::randomBytes(Crypto::NONCE_BYTES);
        const QByteArray cipher = Crypto::encrypt(key, "gizli veri", "baslik", nonce);
        QVERIFY(!cipher.contains("gizli"));
        QCOMPARE(*Crypto::decrypt(key, cipher, "baslik", nonce), QByteArray("gizli veri"));

        QByteArray flipped = cipher;
        flipped[0] = static_cast<char>(flipped[0] ^ 1);
        QVERIFY(!Crypto::decrypt(key, flipped, "baslik", nonce));   // içerik değişti
        QVERIFY(!Crypto::decrypt(key, cipher, "baslik!", nonce));   // başlık değişti
        QVERIFY(!Crypto::decrypt(other, cipher, "baslik", nonce));  // yanlış anahtar
    }

    void vaultCreateAndUnlock()
    {
        VaultSession session;
        QCOMPARE(session.create(path("a.vault"), m_password, Crypto::fastParams()), QString());
        QCOMPARE(session.add(sample()), QString());
        const QString id = session.vault().entries.first().id;
        QVERIFY(!QUuid::fromString(id).isNull());
        session.lock();
        QVERIFY(!session.isUnlocked());
        QVERIFY(session.vault().entries.isEmpty());

        const QByteArray file = readFile(path("a.vault"));
        QVERIFY(file.startsWith("PVLT"));
        QVERIFY(!file.contains("GitHub") && !file.contains("S3cret")); // diskte hiçbir şey açıkta değil

        QCOMPARE(session.unlock(path("a.vault"), "yanlis"), QString("wrong_password"));
        QCOMPARE(session.unlock(path("a.vault"), m_password), QString());
        const Entry e = session.vault().entries.first();
        QCOMPARE(e.id, id);
        QCOMPARE(e.notes, QString("çok satırlı\nnot"));
        QCOMPARE(e.category, Category::Work);
        QCOMPARE(session.unlock(path("yok.vault"), m_password), QString("file_missing"));
    }

    void tamperedFilesAreRejected()
    {
        VaultSession session;
        QCOMPARE(session.create(path("b.vault"), m_password, Crypto::fastParams()), QString());
        QCOMPARE(session.add(sample()), QString());
        const QByteArray good = readFile(path("b.vault"));

        auto tryWith = [&](QByteArray data) {
            writeFile(path("t.vault"), data);
            return session.unlock(path("t.vault"), m_password);
        };
        QByteArray changed = good;
        changed[20] = static_cast<char>(changed[20] ^ 1); // tuz
        QCOMPARE(tryWith(changed), QString("wrong_password"));
        changed = good;
        changed[changed.size() - 1] = static_cast<char>(changed[changed.size() - 1] ^ 1); // şifreli içerik
        QCOMPARE(tryWith(changed), QString("wrong_password"));
        QCOMPARE(tryWith(good.left(30)), QString("not_a_vault"));
        QCOMPARE(tryWith("rastgele bir metin dosyası"), QString("not_a_vault"));
        changed = good;
        changed[4] = 2; // bilinmeyen sürüm
        QCOMPARE(tryWith(changed), QString("unsupported_version"));
        changed = good;
        changed[9] = static_cast<char>(0x7F); // Argon2 için ~2 GB bellek isteyen sahte başlık
        QCOMPARE(tryWith(changed), QString("not_a_vault"));
        QCOMPARE(tryWith(good), QString());
    }

    void entriesAreUpdatedAndRemoved()
    {
        VaultSession session;
        QCOMPARE(session.create(path("c.vault"), m_password, Crypto::fastParams()), QString());
        QCOMPARE(session.add(sample()), QString());
        Entry e = session.vault().entries.first();
        const QDateTime changed = e.passwordChanged;
        QTest::qSleep(1100);

        e.title = "GitHub İş";
        QCOMPARE(session.update(e), QString());
        QCOMPARE(session.vault().entries.first().passwordChanged, changed); // şifre aynı: tarih değişmez
        e.password = "yeni-sifre";
        QCOMPARE(session.update(e), QString());
        QVERIFY(session.vault().entries.first().passwordChanged > changed);
        QCOMPARE(session.vault().entries.first().created, e.created);

        Entry ghost = sample();
        ghost.id = "baska-bir-kimlik";
        QCOMPARE(session.update(ghost), QString("not_found"));
        QCOMPARE(session.remove(e.id), QString());
        QVERIFY(session.vault().entries.isEmpty());
    }

    void masterPasswordChanges()
    {
        VaultSession session;
        QCOMPARE(session.create(path("d.vault"), m_password, Crypto::fastParams()), QString());
        QCOMPARE(session.add(sample()), QString());
        QCOMPARE(session.changePassword("yanlis", "yeni-guclu-parola-42", Crypto::fastParams()),
                 QString("wrong_password"));
        QCOMPARE(session.changePassword(m_password, "yeni-guclu-parola-42", Crypto::fastParams()), QString());
        session.lock();
        QCOMPARE(session.unlock(path("d.vault"), m_password), QString("wrong_password"));
        QCOMPARE(session.unlock(path("d.vault"), "yeni-guclu-parola-42"), QString());
        QCOMPARE(session.vault().entries.size(), 1);

        // Şifreli yedek de aynı şifreyle açılır
        QCOMPARE(session.saveCopy(path("yedek.vault")), QString());
        VaultSession backup;
        QCOMPARE(backup.unlock(path("yedek.vault"), "yeni-guclu-parola-42"), QString());
    }

    void jsonIsValidated()
    {
        QVERIFY(!VaultFile::fromJson("bozuk"));
        QVERIFY(!VaultFile::fromJson(R"({"entries": 5})"));
        QVERIFY(!VaultFile::fromJson(R"({"entries": [{"title": 42}]})"));
        QVERIFY(!VaultFile::fromJson(QByteArray(R"({"entries": [{"password": ")") + QByteArray(1001, 'a') + "\"}]}"));
        const auto vault = VaultFile::fromJson(R"({"entries": [{"id": "../../x", "title": "A", "category": 99}]})");
        QVERIFY(vault);
        QVERIFY(!QUuid::fromString(vault->entries.first().id).isNull()); // geçersiz kimlik yenilendi
        QCOMPARE(vault->entries.first().category, Category::General);
    }

    void generatorFollowsOptions()
    {
        Generator::Options options;
        options.length = 32;
        QSet<QString> seen;
        for (int i = 0; i < 200; ++i) {
            const QString p = Generator::password(options);
            QCOMPARE(p.size(), 32);
            QVERIFY(p.contains(QRegularExpression("[a-z]")) && p.contains(QRegularExpression("[A-Z]"))
                    && p.contains(QRegularExpression("[0-9]")) && p.contains(QRegularExpression("[^a-zA-Z0-9]")));
            QVERIFY(!p.contains(QRegularExpression("[0O1lI|]"))); // karışan karakterler yok
            seen.insert(p);
        }
        QCOMPARE(seen.size(), 200);

        options = {};
        options.upper = options.digits = options.symbols = false;
        options.length = 3; // en az 8'e yükseltilir
        QVERIFY(Generator::password(options).contains(QRegularExpression("^[a-z]{8}$")));
        options.lower = false; // hiçbir grup seçilmezse küçük harf
        QVERIFY(!Generator::password(options).isEmpty());

        const QStringList words = {"elma", "armut", "kiraz", "erik"};
        const QString phrase = Generator::passphrase(5, "-", true, false, words);
        QCOMPARE(phrase.split('-').size(), 5);
        for (const QString &w : phrase.split('-'))
            QVERIFY(words.contains(w.toLower()) && w[0].isUpper());
        QVERIFY(Generator::passphrase(4, " ", false, true, words).contains(QRegularExpression("[0-9]")));
        QVERIFY(Generator::wordList().size() == 1296); // gömülü EFF listesi
    }

    void strengthIsEstimated()
    {
        using Strength::Level;
        QCOMPARE(Strength::evaluate("").level, Level::VeryWeak);
        QCOMPARE(Strength::evaluate("123456").hint, QString("hint_common"));
        QCOMPARE(Strength::evaluate("Galatasaray1905!").hint, QString("hint_common"));
        QVERIFY(Strength::evaluate("abcdefgh").level <= Level::Weak);
        QVERIFY(Strength::evaluate("Aa1!").level <= Level::Weak); // kısa
        QVERIFY(Strength::evaluate("qwerty12345").level <= Level::Fair);
        QVERIFY(Strength::evaluate(Generator::password({})).level >= Level::Strong);
        QVERIFY(Strength::evaluate("Kedi-Pencere-Mavi-Kalem7").level >= Level::Strong);
        QVERIFY(!Strength::acceptableMaster("Kisa1!"));
        QVERIFY(!Strength::acceptableMaster("password123456"));
        QVERIFY(Strength::acceptableMaster("dogru-at-pil-zimba-42"));
    }

    void totpMatchesRfc6238()
    {
        // RFC 6238 Ek B test değerleri (SHA-1, 8 hane, anahtar "12345678901234567890")
        const QByteArray key = "12345678901234567890";
        QCOMPARE(Totp::code(key, 59, 8), QString("94287082"));
        QCOMPARE(Totp::code(key, 1111111109, 8), QString("07081804"));
        QCOMPARE(Totp::code(key, 1111111111, 8), QString("14050471"));
        QCOMPARE(Totp::code(key, 1234567890, 8), QString("89005924"));
        QCOMPARE(Totp::code(key, 2000000000, 8), QString("69279037"));
        QCOMPARE(Totp::code(key, 20000000000, 8), QString("65353130"));
        QCOMPARE(Totp::secondsLeft(59), 1);

        QCOMPARE(*Totp::decodeBase32("JBSWY3DPEHPK3PXP"), QByteArray("Hello!\xDE\xAD\xBE\xEF"));
        QVERIFY(!Totp::decodeBase32("JBSW1!"));
        QCOMPARE(Totp::normalizeSecret("otpauth://totp/Site:ayse?secret=jbswy3dpehpk3pxp&issuer=Site"),
                 QString("JBSWY3DPEHPK3PXP"));
        QCOMPARE(Totp::normalizeSecret(" jbsw y3dp ehpk 3pxp "), QString("JBSWY3DPEHPK3PXP"));
        QVERIFY(Totp::isValidSecret("JBSWY3DPEHPK3PXP"));
        QVERIFY(!Totp::isValidSecret("JBSW")); // çok kısa
        QCOMPARE(Totp::codeNow("geçersiz"), QString());
        QCOMPARE(Totp::codeNow("JBSWY3DPEHPK3PXP").size(), 6);
    }

    void healthReportFindsProblems()
    {
        const QDateTime now = QDateTime::currentDateTimeUtc();
        auto entry = [&](const QString &id, const QString &password, int ageDays) {
            Entry e;
            e.id = id;
            e.password = password;
            e.passwordChanged = now.addDays(-ageDays);
            return e;
        };
        const QList<Entry> entries = {entry("a", Generator::password({}), 10), entry("b", "123456", 10),
                                      entry("c", "Ortak-Sifre-2024!", 10),   entry("d", "Ortak-Sifre-2024!", 10),
                                      entry("e", Generator::password({}), 500), entry("f", Generator::password({}), 10),
                                      entry("g", QString(), 10)};
        const Health::Report report = Health::analyze(entries, {{"f", 3}}, now);
        QCOMPARE(report.checked, 6); // şifresiz kayıt sayılmaz
        QCOMPARE(report.weak, QSet<QString>({"b"}));
        QCOMPARE(report.reused, QSet<QString>({"c", "d"}));
        QCOMPARE(report.old, QSet<QString>({"e"}));
        QCOMPARE(report.breached, QSet<QString>({"f"}));
        QCOMPARE(report.score, 17); // 6 kayıttan sadece "a" sorunsuz
        QCOMPARE(Health::analyze({}, {}, now).score, 100);
    }

    void csvImportReadsCommonFormats()
    {
        const QByteArray chrome = "\xEF\xBB\xBFname,url,username,password,note\n"
                                  "GitHub,https://github.com,ayse,\"p,a\"\"ss\",\"iki\nsatır\"\n"
                                  ",https://example.com/login,veli,1234,\n"
                                  "Boş,https://x.com,,,\n";
        const CsvImport::Result c = CsvImport::read(chrome);
        QCOMPARE(c.entries.size(), 2);
        QCOMPARE(c.skipped, 1);
        QCOMPARE(c.entries[0].password, QString("p,a\"ss"));
        QCOMPARE(c.entries[0].notes, QString("iki\nsatır"));
        QCOMPARE(c.entries[1].title, QString("example.com")); // ad yoksa alan adı

        const QByteArray bitwarden =
            "folder,favorite,type,name,notes,fields,reprompt,login_uri,login_username,login_password,login_totp\r\n"
            ",1,login,Steam,,,0,https://store.steampowered.com,oyuncu,Gizli-123,"
            "otpauth://totp/Steam?secret=JBSWY3DPEHPK3PXP\r\n"
            ",,note,Güvenli not,metin,,0,,,,\r\n";
        const CsvImport::Result b = CsvImport::read(bitwarden);
        QCOMPARE(b.entries.size(), 1);
        QCOMPARE(b.skipped, 1);
        QVERIFY(b.entries[0].favorite);
        QCOMPARE(b.entries[0].totp, QString("JBSWY3DPEHPK3PXP"));

        QCOMPARE(CsvImport::read("a,b,c\n1,2,3\n").error, QString("csv_unknown_format"));
        QCOMPARE(CsvImport::read("").error, QString("csv_unknown_format"));
    }

    void demoVaultIsReady()
    {
        VaultSession session;
        QCOMPARE(DemoVault::create(session, path("demo.vault"), Crypto::fastParams()), QString());
        session.lock();
        QCOMPARE(session.unlock(path("demo.vault"), DemoVault::PASSWORD), QString());
        const QList<Entry> &entries = session.vault().entries;
        QCOMPARE(entries.size(), 30);
        int withTotp = 0;
        for (const Entry &e : entries) {
            QVERIFY(!e.password.isEmpty());
            withTotp += Totp::isValidSecret(e.totp);
        }
        QVERIFY(withTotp >= 8);
        const Health::Report report = Health::analyze(entries, {}, QDateTime::currentDateTimeUtc());
        QVERIFY(!report.weak.isEmpty() && !report.reused.isEmpty() && !report.old.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "test_core.moc"
