#include "core/DemoVault.h"

#include "core/Generator.h"
#include "core/VaultSession.h"

#include <QUuid>

namespace {

// Rastgele 2FA anahtarı (Base32, 160 bit)
QString randomSecret()
{
    static const QString alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    const QByteArray bytes = Crypto::randomBytes(32);
    QString secret;
    for (const char b : bytes)
        secret += alphabet[static_cast<unsigned char>(b) % 32];
    return secret;
}

struct Sample {
    const char *title, *username, *url;
    Category category;
    int kind;     // 0 güçlü üretilmiş, 1 parola, 2 zayıf, 3 tekrarlı, 4 yaygın
    int ageDays;  // şifre kaç gün önce değişti
    bool totp, favorite;
    const char *notes;
};

const Sample SAMPLES[] = {
    {"GitHub", "demo-dev", "https://github.com", Category::Work, 0, 40, true, true, ""},
    {"Gmail", "demo.kullanici@example.com", "https://mail.google.com", Category::Email, 1, 120, true, true,
     "Kurtarma e-postası: yedek@example.com"},
    {"Outlook", "demo.kullanici@example.org", "https://outlook.live.com", Category::Email, 3, 500, false, false, ""},
    {"Proton Mail", "demo.gizli@example.net", "https://mail.proton.me", Category::Email, 0, 30, true, false, ""},
    {"LinkedIn", "demo.kullanici@example.com", "https://www.linkedin.com", Category::Social, 3, 420, false, false, ""},
    {"Instagram", "demo.fotograf", "https://www.instagram.com", Category::Social, 2, 800, false, false, ""},
    {"X", "demo_kullanici", "https://x.com", Category::Social, 0, 60, true, false, ""},
    {"Discord", "demo#4821", "https://discord.com", Category::Social, 1, 200, true, false, ""},
    {"Reddit", "demo_okur", "https://www.reddit.com", Category::Social, 3, 650, false, false, ""},
    {"Banka hesabı (örnek)", "12345678901", "", Category::Finance, 0, 90, false, true,
     "Müşteri numarası ile giriş. Kart şifresi burada tutulmaz."},
    {"PayPal", "demo.kullanici@example.com", "https://www.paypal.com", Category::Finance, 0, 150, true, false, ""},
    {"Binance", "demo.kripto@example.com", "https://www.binance.com", Category::Finance, 1, 20, true, false,
     "Çekim adresi beyaz listesi açık."},
    {"Papara", "05320000000", "https://www.papara.com", Category::Finance, 2, 900, false, false, ""},
    {"Trendyol", "demo.kullanici@example.com", "https://www.trendyol.com", Category::Shopping, 4, 1000, false, false, ""},
    {"Hepsiburada", "demo.kullanici@example.com", "https://www.hepsiburada.com", Category::Shopping, 3, 700, false, false, ""},
    {"Amazon", "demo.kullanici@example.com", "https://www.amazon.com.tr", Category::Shopping, 0, 100, true, false, ""},
    {"Yemeksepeti", "demo.kullanici@example.com", "https://www.yemeksepeti.com", Category::Shopping, 2, 1200, false, false, ""},
    {"Netflix", "demo.aile@example.com", "https://www.netflix.com", Category::Entertainment, 1, 300, false, true,
     "Aile planı, 4 ekran."},
    {"Spotify", "demo.muzik", "https://open.spotify.com", Category::Entertainment, 4, 1500, false, false, ""},
    {"Steam", "demo_oyuncu", "https://store.steampowered.com", Category::Entertainment, 0, 45, true, false,
     "Steam Guard mobil uygulamada."},
    {"Epic Games", "demo_oyuncu", "https://store.epicgames.com", Category::Entertainment, 1, 400, true, false, ""},
    {"YouTube Premium", "demo.kullanici@example.com", "https://www.youtube.com", Category::Entertainment, 0, 70, false, false, ""},
    {"Slack", "demo@sirket.example", "https://app.slack.com", Category::Work, 0, 15, false, false, ""},
    {"Jira", "demo@sirket.example", "https://sirket.atlassian.net", Category::Work, 1, 380, false, false, ""},
    {"AWS Konsol", "demo-admin", "https://console.aws.amazon.com", Category::Work, 0, 10, true, true,
     "Kök hesap değil; IAM kullanıcısı."},
    {"Ev Wi-Fi", "AgAdi-5G", "", Category::General, 1, 600, false, false, "Modem arayüzü: 192.168.1.1"},
    {"e-Devlet (örnek)", "12345678901", "https://www.turkiye.gov.tr", Category::General, 0, 200, false, false, ""},
    {"Mobil operatör (örnek)", "05320000000", "", Category::General, 2, 750, false, false, ""},
    {"Dropbox", "demo.kullanici@example.com", "https://www.dropbox.com", Category::Other, 3, 550, false, false, ""},
    {"Zoom", "demo.kullanici@example.com", "https://zoom.us", Category::Other, 0, 35, false, false, ""},
};

} // namespace

namespace DemoVault {

QString create(VaultSession &session, const QString &path, const Crypto::KdfParams &params)
{
    const QString error = session.create(path, PASSWORD, params);
    if (!error.isEmpty())
        return error;

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString reused = "Yaz2023!kahve"; // birkaç kayıtta ortak: "tekrar kullanılan" uyarısı için
    const char *weak[] = {"istanbul34", "demo1234", "kedi2020", "ankara06"};
    const char *common[] = {"123456", "galatasaray1905", "qwerty123"};
    int weakIndex = 0, commonIndex = 0;
    for (const Sample &s : SAMPLES) {
        Entry e;
        e.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        e.title = QString::fromUtf8(s.title);
        e.username = QString::fromUtf8(s.username);
        e.url = QString::fromUtf8(s.url);
        e.notes = QString::fromUtf8(s.notes);
        e.category = s.category;
        e.favorite = s.favorite;
        switch (s.kind) {
        case 0: e.password = Generator::password({}); break;
        case 1: e.password = Generator::passphrase(5, "-", true, true); break;
        case 2: e.password = weak[weakIndex++ % 4]; break;
        case 3: e.password = reused; break;
        default: e.password = common[commonIndex++ % 3]; break;
        }
        if (s.totp)
            e.totp = randomSecret();
        e.passwordChanged = now.addDays(-s.ageDays);
        e.created = e.passwordChanged.addDays(-30);
        e.modified = e.passwordChanged;
        session.vault().entries << e;
    }
    return session.save();
}

} // namespace DemoVault
