#pragma once

#include <QString>
#include <QStringList>

// Şifre ve parola üretici. Rastgelelik libsodium'un kriptografik üretecinden gelir
// (rand() ya da QRandomGenerator'ın varsayılan hâli tahmin edilebilir olabilir).
namespace Generator {

struct Options {
    int length = 20;
    bool lower = true;
    bool upper = true;
    bool digits = true;
    bool symbols = true;
    bool avoidAmbiguous = true; // 0/O, 1/l/I gibi karışan karakterler çıkarılır
};

constexpr int MIN_LENGTH = 8;
constexpr int MAX_LENGTH = 128;
constexpr int MIN_WORDS = 3;
constexpr int MAX_WORDS = 12;

QString password(const Options &options);
QString passphrase(int words, const QString &separator, bool capitalize, bool addNumber,
                   const QStringList &wordList = QStringList()); // boşsa gömülü EFF listesi
const QStringList &wordList();

} // namespace Generator
