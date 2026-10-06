#pragma once

#include "core/Crypto.h"

#include <QString>

class VaultSession;

// Uygulamayı denemek için ayrı bir örnek kasa: ~30 hayali kayıt. Bazı şifreler bilerek zayıf,
// tekrarlı ya da eski; böylece sağlık raporu ve 2FA ekranı boş görünmez. Gerçek kasadan ayrı dosyadır.
namespace DemoVault {

inline const QString PASSWORD = "Demo-Kasa-2026";

QString create(VaultSession &session, const QString &path,
               const Crypto::KdfParams &params = Crypto::defaultParams());

} // namespace DemoVault
