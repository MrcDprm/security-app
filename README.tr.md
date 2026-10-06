# Şifre Kasası

[English](README.md) | **Türkçe**

C++20 ve Qt 6 ile yazılmış masaüstü şifre yöneticisi. Bütün kayıtlar tek bir ana şifreyle açılan, tek bir şifreli kasa dosyasında tutulur.

> 🚧 Geliştirme sürüyor. Bu README proje planıdır, v1.0.0'da tamamlanacak.

## Plan

### MVP
- **Ana şifre:** güçlü bir ana şifreyle kasa oluşturma (uzunluk ve çeşitlilik kontrolü). Anahtar **Argon2id** (libsodium) ile türetilir; ana şifrenin kendisi hiçbir yerde saklanmaz.
- **Şifreli kasa dosyası:** kasanın tamamı (başlıklar, kullanıcı adları, şifreler, adresler, notlar) **XChaCha20-Poly1305** ile şifrelenir. Dosyada yapılan en küçük değişiklik fark edilir ve kasa açılmaz. Kayıt atomiktir; program yarıda kapansa da kasa bozulmaz.
- **Kayıtlar:** başlık, kullanıcı adı, şifre, adres, not, kategori, favori; ekleme, düzenleme, silme; arama ve kategori filtresi.
- **Şifre üretici:** uzunluk, karakter grupları, karışan karakterleri çıkarma ya da rastgele kelimelerden parola; güç göstergesi.
- **Pano:** kullanıcı adı ya da şifre kopyalama; pano 30 saniye sonra kendiliğinden temizlenir.
- **Kilitleme:** kilit düğmesi ve hareketsizlikte otomatik kilit; kilitlenince anahtar bellekten silinir. Yanlış ana şifre denemeleri yavaşlatılır.
- **Ana şifreyi değiştirme.**
- **Masaüstü uygulaması:** koyu ve açık tema, Türkçe ve İngilizce, ikon, sürüm, Hakkında penceresi, kasa kullanıcı klasöründe, Windows kurulum dosyası.
- **Testler:** şifreleme gidiş-dönüşü, dosya kurcalama tespiti, yanlış şifre, üretici kuralları, güç puanı (Qt Test).

### Ekler
- **Şifre sağlığı raporu:** zayıf, tekrar kullanılan ve eski şifreler ve genel güvenlik puanı.
- **Sızıntı kontrolü (Have I Been Pwned):** k-anonimlik; şifrenin SHA-1 özetinin sadece ilk 5 karakteri gönderilir. İsteğe bağlı, internet gerekir.
- **2FA kodları (TOTP):** TOTP anahtarı olan kayıtlar için kalan süreyle 6 haneli kod.
- **İçe ve dışa aktarma:** Chrome, Edge ve Bitwarden CSV'sinden içe aktarma; şifreli yedek dışa aktarma.
- **Demo kasa:** bilinen bir şifreyle açılan, ~30 hayali kayıtlı ayrı bir örnek kasa.

### Gelecek Planları
- Otomatik doldurma için tarayıcı eklentisi.
- Cihazlar arası eşitleme.
- Kayıtlara dosya ekleme.
- Windows Hello ile kilit açma.

## Kullanılan Teknolojiler
- C++20, Qt 6 (Widgets, Network, Test)
- libsodium (Argon2id, XChaCha20-Poly1305)
- CMake, MinGW-w64 (MSYS2 UCRT64)
- windeployqt, Inno Setup
