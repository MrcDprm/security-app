#pragma once

#include <QString>

// Şifre gücü tahmini: karakter çeşitliliğine göre entropi, ardından bilinen zayıflıklar için ceza
// (çok kullanılan şifreler, "abc"/"123"/"qwerty" dizileri, tekrar eden karakterler, sadece rakam).
namespace Strength {

enum class Level { VeryWeak, Weak, Fair, Strong, VeryStrong };

struct Result {
    Level level = Level::VeryWeak;
    int bits = 0;  // tahmini entropi
    QString hint;  // iyileştirme önerisinin metin anahtarı ("hint_too_short"); güçlüyse boş
};

Result evaluate(const QString &password);
bool isCommon(const QString &password);

// Ana şifre kuralı: en az 12 karakter ve "güçlü" seviye
constexpr int MASTER_MIN_LENGTH = 12;
bool acceptableMaster(const QString &password);

} // namespace Strength
