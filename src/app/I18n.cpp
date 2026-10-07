#include "app/I18n.h"

#include <QHash>
#include <QLocale>

namespace {

QString g_language = "tr";

struct Text {
    const char *tr;
    const char *en;
};

const QHash<QString, Text> &texts()
{
    static const QHash<QString, Text> table = {
        // Genel
        {"app_name", {"Şifre Kasası", "Password Vault"}},
        {"yes", {"Evet", "Yes"}},
        {"no", {"Hayır", "No"}},
        {"ok", {"Tamam", "OK"}},
        {"save", {"Kaydet", "Save"}},
        {"cancel", {"Vazgeç", "Cancel"}},
        {"close", {"Kapat", "Close"}},
        {"edit", {"Düzenle", "Edit"}},
        {"delete", {"Sil", "Delete"}},
        {"copy", {"Kopyala", "Copy"}},
        {"theme", {"Tema", "Theme"}},
        {"language", {"English", "Türkçe"}},
        {"about", {"Hakkında", "About"}},
        {"settings", {"Ayarlar", "Settings"}},
        {"lock", {"Kilitle", "Lock"}},
        {"minutes", {"dakika", "minutes"}},
        {"seconds", {"saniye", "seconds"}},
        {"version", {"Sürüm {0}", "Version {0}"}},
        {"data_folder", {"Veri klasörü", "Data folder"}},
        {"view_on_github", {"GitHub'da görüntüle", "View on GitHub"}},
        {"about_text",
         {"C++ ve Qt ile yazılmış şifre yöneticisi. Kasa, Argon2id ile ana şifreden türetilen anahtarla "
          "XChaCha20-Poly1305 kullanılarak şifrelenir ve sadece bu bilgisayarda durur.",
          "A password manager written in C++ and Qt. The vault is encrypted with XChaCha20-Poly1305 using a key "
          "derived from the master password with Argon2id, and it stays on this computer."}},
        {"credits",
         {"Kelime listesi: EFF Short Wordlist (CC BY 3.0). Sızıntı verisi: Have I Been Pwned.",
          "Word list: EFF Short Wordlist (CC BY 3.0). Breach data: Have I Been Pwned."}},

        // Karşılama ve kilit
        {"welcome_text", {"Şifrelerin tek bir ana şifreyle açılan, şifreli bir dosyada.",
                          "Your passwords in one encrypted file, unlocked with a single master password."}},
        {"create_vault", {"Yeni kasa oluştur", "Create a new vault"}},
        {"master_explain",
         {"Ana şifre hiçbir yerde saklanmaz ve unutulursa kurtarılamaz. Uzun ve sadece senin bildiğin bir şey seç; "
          "birkaç kelimelik bir cümle iyi bir seçimdir.",
          "The master password is never stored and cannot be recovered if forgotten. Choose something long that only "
          "you know; a sentence of a few words works well."}},
        {"master_password", {"Ana şifre", "Master password"}},
        {"repeat_password", {"Şifre tekrar", "Repeat password"}},
        {"master_rule", {"Ana şifre en az 12 karakter ve \"güçlü\" olmalı.",
                         "The master password must be at least 12 characters and \"strong\"."}},
        {"passwords_differ", {"Şifreler aynı değil.", "The passwords do not match."}},
        {"vault_location", {"Kasa dosyası", "Vault file"}},
        {"change_location", {"Konumu değiştir…", "Change location…"}},
        {"open_existing", {"Var olan kasayı aç…", "Open an existing vault…"}},
        {"try_demo", {"Demo kasayı dene", "Try the demo vault"}},
        {"demo_note", {"Demo kasanın şifresi: {0}", "Demo vault password: {0}"}},
        {"vault_files", {"Kasa dosyaları", "Vault files"}},
        {"default_vault_name", {"Kasam", "MyVault"}},
        {"overwrite_vault", {"\"{0}\" zaten var. Üzerine yeni boş kasa yazılsın mı? İçindeki kayıtlar silinir.",
                             "\"{0}\" already exists. Overwrite it with a new empty vault? Its entries will be lost."}},
        {"vault_locked", {"Kasa kilitli", "Vault locked"}},
        {"unlock", {"Kilidi aç", "Unlock"}},
        {"unlocking", {"Açılıyor…", "Unlocking…"}},
        {"open_other", {"Başka kasa aç…", "Open another vault…"}},
        {"new_vault", {"Yeni kasa", "New vault"}},
        {"wait_seconds", {"{0} sn bekle", "Wait {0} s"}},
        {"show_password", {"Şifreyi göster", "Show password"}},
        {"hide_password", {"Şifreyi gizle", "Hide password"}},

        // Kayıtlar
        {"all_entries", {"Tüm kayıtlar", "All entries"}},
        {"favorites", {"Favoriler", "Favourites"}},
        {"search_entries", {"Ara (başlık, kullanıcı adı, adres)…", "Search (title, user name, URL)…"}},
        {"new_entry", {"Yeni kayıt", "New entry"}},
        {"edit_entry", {"Kaydı düzenle", "Edit entry"}},
        {"select_entry", {"Ayrıntıları görmek için soldan bir kayıt seç.", "Select an entry on the left to see it."}},
        {"title", {"Başlık", "Title"}},
        {"username", {"Kullanıcı adı", "User name"}},
        {"password", {"Şifre", "Password"}},
        {"url", {"Adres", "URL"}},
        {"category", {"Kategori", "Category"}},
        {"notes", {"Notlar", "Notes"}},
        {"favorite", {"Favori", "Favourite"}},
        {"unfavorite", {"Favorilerden çıkar", "Remove favourite"}},
        {"generate", {"Üret", "Generate"}},
        {"totp_secret", {"2FA anahtarı", "2FA secret"}},
        {"totp_placeholder", {"Base32 anahtar ya da otpauth:// adresi (isteğe bağlı)",
                              "Base32 secret or otpauth:// URL (optional)"}},
        {"totp_code", {"2FA kodu", "2FA code"}},
        {"title_required", {"Başlık gerekli.", "A title is required."}},
        {"notes_too_long", {"Not çok uzun (en fazla 20.000 karakter).", "The note is too long (20,000 characters max)."}},
        {"invalid_totp", {"2FA anahtarı geçersiz. Sitenin verdiği Base32 anahtarı ya da otpauth:// adresini yapıştır.",
                          "Invalid 2FA secret. Paste the Base32 secret or otpauth:// URL the site gave you."}},
        {"invalid_url", {"Adres geçersiz. Sadece http ve https adresleri kabul edilir.",
                         "Invalid URL. Only http and https addresses are accepted."}},
        {"confirm_delete", {"\"{0}\" kaydı silinsin mi? Bu geri alınamaz.", "Delete \"{0}\"? This cannot be undone."}},
        {"copy_username", {"Kullanıcı adını kopyala", "Copy user name"}},
        {"copy_password", {"Şifreyi kopyala", "Copy password"}},
        {"copy_url", {"Adresi kopyala", "Copy URL"}},
        {"copy_code", {"Kodu kopyala", "Copy code"}},
        {"open_url", {"Tarayıcıda aç", "Open in browser"}},
        {"dates_line", {"Oluşturuldu: {0} · Şifre değişti: {1}", "Created: {0} · Password changed: {1}"}},
        {"copied_username", {"Kullanıcı adı kopyalandı.", "User name copied."}},
        {"copied_password", {"Şifre kopyalandı.", "Password copied."}},
        {"copied_url", {"Adres kopyalandı.", "URL copied."}},
        {"copied_code", {"2FA kodu kopyalandı.", "2FA code copied."}},
        {"clipboard_countdown", {"Pano {0} saniye sonra temizlenecek.", "Clipboard clears in {0} seconds."}},
        {"clipboard_cleared", {"Pano temizlendi.", "Clipboard cleared."}},

        // 2FA
        {"totp_codes", {"2FA kodları", "2FA codes"}},
        {"totp_hint", {"Kopyalamak için koda çift tıkla.", "Double-click a code to copy it."}},
        {"seconds_left", {"{0} sn", "{0} s"}},
        {"no_totp", {"2FA anahtarı olan kayıt yok. Bir kaydı düzenleyip sitenin verdiği anahtarı ekleyebilirsin.",
                     "No entries with a 2FA secret. Edit an entry and add the secret the site gave you."}},

        // Sağlık
        {"password_health", {"Şifre sağlığı", "Password health"}},
        {"health_score", {"Güvenlik puanı (100 üzerinden)", "Security score (out of 100)"}},
        {"weak_passwords", {"Zayıf", "Weak"}},
        {"reused_passwords", {"Tekrar kullanılan", "Reused"}},
        {"old_passwords", {"1 yıldan eski", "Older than a year"}},
        {"breached_passwords", {"Sızıntıda görülen", "Seen in breaches"}},
        {"problems", {"Sorunlar", "Problems"}},
        {"problem_weak", {"zayıf", "weak"}},
        {"problem_reused", {"başka kayıtta da kullanılıyor", "used in another entry"}},
        {"problem_old", {"1 yıldır değişmedi", "unchanged for a year"}},
        {"problem_breached", {"{0} sızıntıda görüldü", "seen in {0} breaches"}},
        {"check_breaches", {"Sızıntı kontrolü yap", "Check for breaches"}},
        {"breach_explain",
         {"Sızıntı kontrolü isteğe bağlıdır: şifrelerin Have I Been Pwned veritabanındaki sızıntılarda görülüp "
          "görülmediğine bakar.",
          "Breach check is optional: it looks up whether your passwords appear in breaches known to Have I Been Pwned."}},
        {"breach_confirm",
         {"Sızıntı kontrolü için internete çıkılacak. Şifrelerin kendisi gönderilmez: her şifrenin SHA-1 özetinin "
          "sadece ilk 5 karakteri gönderilir, eşleşme bu bilgisayarda aranır (k-anonimlik). Devam edilsin mi?",
          "The breach check goes online. Your passwords are not sent: only the first 5 characters of each password's "
          "SHA-1 hash are sent, and matching happens on this computer (k-anonymity). Continue?"}},
        {"breach_progress", {"Kontrol ediliyor: {0}/{1}", "Checking: {0}/{1}"}},
        {"breach_done", {"Sızıntı kontrolü tamamlandı.", "Breach check finished."}},
        {"breach_failed", {"Sızıntı kontrolü yapılamadı. İnternet bağlantını kontrol et.",
                           "The breach check failed. Check your internet connection."}},

        // Üretici
        {"generator", {"Şifre üretici", "Password generator"}},
        {"regenerate", {"Yenile", "Regenerate"}},
        {"random_characters", {"Rastgele karakterler", "Random characters"}},
        {"random_words", {"Rastgele kelimeler (parola)", "Random words (passphrase)"}},
        {"length", {"Uzunluk", "Length"}},
        {"characters", {"Karakterler", "Characters"}},
        {"avoid_ambiguous", {"Karışan karakterleri kullanma (0 O 1 l I)", "Avoid look-alike characters (0 O 1 l I)"}},
        {"word_count", {"Kelime sayısı", "Number of words"}},
        {"separator", {"Ayırıcı", "Separator"}},
        {"space", {"Boşluk", "Space"}},
        {"capitalize", {"Kelimelerin ilk harfi büyük", "Capitalise each word"}},
        {"add_number", {"Bir rakam ekle", "Add a number"}},
        {"wordlist_credit", {"Kelimeler: EFF Short Wordlist (1.296 kelime, CC BY 3.0)",
                             "Words: EFF Short Wordlist (1,296 words, CC BY 3.0)"}},

        // Güç
        {"hint_empty", {"", ""}},
        {"hint_common", {"çok kullanılan bir şifre", "a very common password"}},
        {"hint_too_short", {"daha uzun olmalı", "make it longer"}},
        {"hint_sequence", {"abc, 123, qwerty gibi diziler var", "contains sequences like abc, 123, qwerty"}},
        {"hint_repeat", {"tekrar eden karakterler var", "contains repeated characters"}},
        {"hint_variety", {"büyük harf, rakam ve işaret ekle", "add capitals, digits and symbols"}},
        {"hint_longer", {"birkaç karakter daha ekle", "add a few more characters"}},

        // Ayarlar
        {"security", {"Güvenlik", "Security"}},
        {"auto_lock_after", {"Hareketsizlikte kilitle", "Lock when idle for"}},
        {"clear_clipboard_after", {"Panoyu temizle", "Clear clipboard after"}},
        {"lock_on_minimize", {"Simge durumuna küçültünce kilitle", "Lock when minimised"}},
        {"vault", {"Kasa", "Vault"}},
        {"change_master", {"Ana şifreyi değiştir", "Change master password"}},
        {"current_password", {"Mevcut şifre", "Current password"}},
        {"new_password", {"Yeni şifre", "New password"}},
        {"master_changed", {"Ana şifre değiştirildi. Eski yedekler eski şifreyle açılmaya devam eder.",
                            "The master password was changed. Older backups still open with the old password."}},
        {"export_backup", {"Şifreli yedek al…", "Export encrypted backup…"}},
        {"backup_saved", {"Yedek kaydedildi. Aynı ana şifreyle açılır.", "Backup saved. It opens with the same master password."}},
        {"backup_same_file", {"Yedek, açık kasanın üzerine yazılamaz. Başka bir ad seç.",
                              "The backup cannot overwrite the open vault. Choose another name."}},
        {"import_csv", {"CSV'den içe aktar…", "Import from CSV…"}},
        {"import_hint",
         {"Chrome, Edge, Firefox ya da Bitwarden'dan dışa aktardığın CSV dosyasını seç.",
          "Choose the CSV file you exported from Chrome, Edge, Firefox or Bitwarden."}},
        {"import_confirm", {"{0} kayıt bulundu. Kasaya eklensin mi?", "{0} entries found. Add them to the vault?"}},
        {"import_done", {"{0} kayıt eklendi, {1} satır atlandı.", "{0} entries added, {1} rows skipped."}},
        {"delete_csv_question",
         {"\"{0}\" dosyası şifreleri düz metin olarak içeriyor. Şimdi silinsin mi?",
          "\"{0}\" contains your passwords as plain text. Delete it now?"}},

        // Hatalar
        {"unexpected_error", {"Beklenmeyen bir hata oluştu.", "An unexpected error occurred."}},
        {"err_wrong_password", {"Ana şifre yanlış ya da kasa dosyası değiştirilmiş.",
                                "Wrong master password, or the vault file has been modified."}},
        {"err_not_a_vault", {"Bu bir kasa dosyası değil ya da bozuk.", "This is not a vault file, or it is damaged."}},
        {"err_unsupported_version", {"Bu kasa uygulamanın daha yeni bir sürümüyle oluşturulmuş.",
                                     "This vault was created by a newer version of the app."}},
        {"err_file_missing", {"Kasa dosyası bulunamadı.", "The vault file was not found."}},
        {"err_corrupted", {"Kasanın içeriği okunamadı.", "The vault contents could not be read."}},
        {"err_crypto_failed", {"Şifreleme kitaplığı başlatılamadı.", "The encryption library could not start."}},
        {"err_save_failed", {"Kasa kaydedilemedi. Klasöre yazma izni olduğundan emin ol.",
                             "The vault could not be saved. Make sure the folder is writable."}},
        {"err_locked", {"Kasa kilitli.", "The vault is locked."}},
        {"err_not_found", {"Kayıt bulunamadı.", "Entry not found."}},
        {"err_csv_too_large", {"CSV dosyası çok büyük (en fazla 20 MB).", "The CSV file is too large (20 MB max)."}},
        {"err_csv_unknown_format", {"CSV biçimi tanınmadı. Başlık satırında name/url, username ve password sütunları olmalı.",
                                    "Unknown CSV format. The header must have name/url, username and password columns."}},
        {"err_csv_empty", {"CSV dosyasında içe aktarılacak kayıt yok.", "The CSV file has no entries to import."}},
        {"err_read_failed", {"Dosya okunamadı.", "The file could not be read."}},
        {"err_delete_failed", {"Dosya silinemedi; elle silmeyi unutma.", "The file could not be deleted; remember to delete it."}},
    };
    return table;
}

QString lookup(const QString &key)
{
    const auto it = texts().constFind(key);
    if (it == texts().constEnd())
        return key;
    return QString::fromUtf8(g_language == "en" ? it->en : it->tr);
}

QString pick(const Text &text)
{
    return QString::fromUtf8(g_language == "en" ? text.en : text.tr);
}

} // namespace

namespace I18n {

void setLanguage(const QString &language)
{
    g_language = language == "en" ? "en" : "tr";
    QLocale::setDefault(QLocale(g_language == "en" ? QLocale::English : QLocale::Turkish,
                                g_language == "en" ? QLocale::UnitedStates : QLocale::Turkey));
}

QString language()
{
    return g_language;
}

QString t(const char *key)
{
    return lookup(QString::fromLatin1(key));
}

QString error(const QString &key)
{
    const QString text = lookup("err_" + key);
    return text.startsWith("err_") ? lookup("unexpected_error") : text;
}

QString category(Category value)
{
    static const Text names[] = {{"Genel", "General"},        {"E-posta", "Email"},     {"Sosyal medya", "Social"},
                                 {"Banka ve ödeme", "Finance"}, {"Alışveriş", "Shopping"}, {"İş", "Work"},
                                 {"Eğlence", "Entertainment"}, {"Diğer", "Other"}};
    return pick(names[static_cast<int>(value)]);
}

QString strength(Strength::Level value)
{
    static const Text names[] = {{"Çok zayıf", "Very weak"}, {"Zayıf", "Weak"}, {"Orta", "Fair"},
                                 {"Güçlü", "Strong"},        {"Çok güçlü", "Very strong"}};
    return pick(names[static_cast<int>(value)]);
}

QString dateTime(const QDateTime &value)
{
    if (!value.isValid())
        return "—";
    return value.toLocalTime().toString(g_language == "en" ? "MMM d, yyyy" : "dd.MM.yyyy");
}

} // namespace I18n
