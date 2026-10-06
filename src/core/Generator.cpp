#include "core/Generator.h"

#include <QFile>
#include <sodium.h>

namespace {

const QString LOWER = "abcdefghijklmnopqrstuvwxyz";
const QString UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const QString DIGITS = "0123456789";
const QString SYMBOLS = "!@#$%^&*()-_=+[]{};:,.?/";
const QString AMBIGUOUS = "0O1lI|";

// 0..count-1 arası eşit olasılıklı sayı (modulo yanlılığı olmadan)
int uniform(int count)
{
    return static_cast<int>(randombytes_uniform(static_cast<uint32_t>(count)));
}

QString without(QString set, bool avoidAmbiguous)
{
    if (avoidAmbiguous)
        for (const QChar c : AMBIGUOUS)
            set.remove(c);
    return set;
}

} // namespace

namespace Generator {

QString password(const Options &options)
{
    QStringList sets;
    if (options.lower)
        sets << without(LOWER, options.avoidAmbiguous);
    if (options.upper)
        sets << without(UPPER, options.avoidAmbiguous);
    if (options.digits)
        sets << without(DIGITS, options.avoidAmbiguous);
    if (options.symbols)
        sets << without(SYMBOLS, options.avoidAmbiguous);
    if (sets.isEmpty())
        sets << without(LOWER, options.avoidAmbiguous);
    const int length = std::clamp(options.length, MIN_LENGTH, MAX_LENGTH);

    // Seçilen her gruptan en az bir karakter; kalanı bütün gruplardan
    QString result;
    for (const QString &set : sets)
        result += set[uniform(set.size())];
    const QString all = sets.join(QString());
    while (result.size() < length)
        result += all[uniform(all.size())];
    // Zorunlu karakterler hep başta kalmasın diye karıştırılır (Fisher-Yates)
    for (int i = result.size() - 1; i > 0; --i)
        std::swap(result[i], result[uniform(i + 1)]);
    return result;
}

const QStringList &wordList()
{
    // EFF kısa kelime listesi (1.296 kelime, CC BY 3.0): her satır "1111<TAB>kelime"
    static const QStringList words = [] {
        QStringList list;
        QFile file(":/eff_short_wordlist_1.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            while (!file.atEnd()) {
                const QString word = QString::fromUtf8(file.readLine()).trimmed().section('\t', -1);
                if (!word.isEmpty())
                    list << word;
            }
        return list;
    }();
    return words;
}

QString passphrase(int words, const QString &separator, bool capitalize, bool addNumber, const QStringList &list)
{
    const QStringList &source = list.isEmpty() ? wordList() : list;
    if (source.isEmpty())
        return QString();
    QStringList parts;
    for (int i = 0; i < std::clamp(words, MIN_WORDS, MAX_WORDS); ++i) {
        QString word = source[uniform(source.size())];
        if (capitalize)
            word[0] = word[0].toUpper();
        parts << word;
    }
    if (addNumber) // rastgele bir kelimenin sonuna tek rakam
        parts[uniform(parts.size())] += QString::number(uniform(10));
    return parts.join(separator);
}

} // namespace Generator
