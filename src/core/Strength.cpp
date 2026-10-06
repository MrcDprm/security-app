#include "core/Strength.h"

#include <QSet>
#include <QStringList>
#include <cmath>

namespace {

// Sızıntı listelerinde en sık görülen şifrelerden bir seçki (küçük harfe çevrilmiş hâlleriyle karşılaştırılır)
const QSet<QString> &commonPasswords()
{
    static const QSet<QString> list = {
        "123456",    "password",  "12345678",  "qwerty",    "123456789", "12345",     "1234",      "111111",
        "1234567",   "dragon",    "123123",    "baseball",  "abc123",    "football",  "monkey",    "letmein",
        "696969",    "shadow",    "master",    "666666",    "qwertyuiop", "123321",   "mustang",   "1234567890",
        "michael",   "654321",    "superman",  "1qaz2wsx",  "7777777",   "121212",    "000000",    "qazwsx",
        "123qwe",    "killer",    "trustno1",  "jordan",    "jennifer",  "zxcvbnm",   "asdfgh",    "hunter",
        "buster",    "soccer",    "harley",    "batman",    "andrew",    "tigger",    "sunshine",  "iloveyou",
        "2000",      "charlie",   "robert",    "thomas",    "hockey",    "ranger",    "daniel",    "starwars",
        "klaster",   "112233",    "george",    "computer",  "michelle",  "jessica",   "pepper",    "1111",
        "zxcvbn",    "555555",    "11111111",  "131313",    "freedom",   "777777",    "pass",      "maggie",
        "159753",    "aaaaaa",    "ginger",    "princess",  "joshua",    "cheese",    "amanda",    "summer",
        "love",      "ashley",    "nicole",    "chelsea",   "biteme",    "matthew",   "access",    "yankees",
        "987654321", "dallas",    "austin",    "thunder",   "taylor",    "matrix",    "admin",     "welcome",
        "passw0rd",  "p@ssw0rd",  "password1", "qwerty123", "login",     "secret",    "galatasaray", "fenerbahce",
        "besiktas",  "trabzonspor", "sifre",   "sifre123",  "parola",    "turkiye",   "istanbul",  "ankara",
    };
    return list;
}

const QStringList KEYBOARD_ROWS = {"qwertyuiop", "asdfghjkl", "zxcvbnm", "1234567890", "qazwsx"};

int poolSize(const QString &password, int &classes)
{
    bool lower = false, upper = false, digit = false, symbol = false, other = false;
    for (const QChar c : password) {
        if (c.unicode() < 128) {
            if (c.isLower()) lower = true;
            else if (c.isUpper()) upper = true;
            else if (c.isDigit()) digit = true;
            else symbol = true;
        } else {
            other = true; // Türkçe harfler ve diğer Unicode karakterler
        }
    }
    classes = lower + upper + digit + symbol + other;
    return lower * 26 + upper * 26 + digit * 10 + symbol * 33 + other * 100;
}

} // namespace

namespace Strength {

bool isCommon(const QString &password)
{
    QString lowered = password.toLower();
    if (commonPasswords().contains(lowered))
        return true;
    // "Galatasaray1905!" gibi: sondaki rakam ve işaretler atılınca listede mi?
    while (!lowered.isEmpty() && !lowered.back().isLetter())
        lowered.chop(1);
    return lowered.size() >= 4 && commonPasswords().contains(lowered);
}

Result evaluate(const QString &password)
{
    Result result;
    if (password.isEmpty()) {
        result.hint = "hint_empty";
        return result;
    }

    int classes = 0;
    const int pool = poolSize(password, classes);

    // Tekrar eden ("aaa") ve ardışık ("abc", "321") karakterler ile klavye dizileri ("qwer") az sayılır
    const QString lowered = password.toLower();
    QList<double> weight(password.size(), 1.0);
    bool sequence = false, repeat = false;
    for (int i = 2; i < password.size(); ++i) {
        const int a = password[i - 2].unicode(), b = password[i - 1].unicode(), c = password[i].unicode();
        if (a == b && b == c) {
            weight[i] = 0.2;
            repeat = true;
        } else if (b - a == c - b && std::abs(c - b) == 1) {
            weight[i] = 0.2;
            sequence = true;
        }
    }
    for (const QString &row : KEYBOARD_ROWS)
        for (int i = 0; i + 4 <= row.size(); ++i) {
            const int at = lowered.indexOf(row.mid(i, 4));
            if (at >= 0) {
                sequence = true;
                for (int k = at + 1; k < at + 4; ++k)
                    weight[k] = std::min(weight[k], 0.2);
            }
        }
    double effective = 0;
    for (double w : weight)
        effective += w;
    double bits = effective * std::log2(std::max(pool, 2));

    const bool common = isCommon(password);
    if (common)
        bits = std::min(bits, 10.0);
    result.bits = static_cast<int>(bits);

    if (bits < 28) result.level = Level::VeryWeak;
    else if (bits < 36) result.level = Level::Weak;
    else if (bits < 60) result.level = Level::Fair;
    else if (bits < 80) result.level = Level::Strong;
    else result.level = Level::VeryStrong;
    if (password.size() < 8 && result.level > Level::Weak)
        result.level = Level::Weak; // kısa şifre ne kadar çeşitli olursa olsun zayıftır

    if (common) result.hint = "hint_common";
    else if (password.size() < MASTER_MIN_LENGTH && result.level < Level::Strong) result.hint = "hint_too_short";
    else if (sequence) result.hint = "hint_sequence";
    else if (repeat) result.hint = "hint_repeat";
    else if (classes < 3 && result.level < Level::Strong) result.hint = "hint_variety";
    else if (result.level < Level::Strong) result.hint = "hint_longer";
    return result;
}

bool acceptableMaster(const QString &password)
{
    return password.size() >= MASTER_MIN_LENGTH && evaluate(password).level >= Level::Strong;
}

} // namespace Strength
