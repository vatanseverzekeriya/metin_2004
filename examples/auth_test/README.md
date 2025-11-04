# Kimlik Doğrulama Sistemi Test Programı

Bu test programı Metin2 PvP Server'ın veritabanı, hesap kaydı, giriş ve oturum yönetimi sistemlerini test eder.

## Derleme

### Manuel Derleme (g++)

```bash
g++ -o test_auth test_auth_system.cpp \
    ../../src/db/DBManager.cpp \
    ../../src/auth/SessionManager.cpp \
    ../../src/auth/AuthManager.cpp \
    -I../../include \
    -lmysqlclient -lssl -lcrypto -lpthread \
    -std=c++11
```

### CMake ile Derleme

```bash
mkdir build
cd build
cmake ..
make
```

## Kurulum

1. MySQL veritabanını kurun:
```bash
mysql -u root -p < ../../database/schema.sql
```

2. Test programında veritabanı bilgilerini güncelleyin:
```cpp
// test_auth_system.cpp içinde
std::string host = "localhost";
std::string user = "root";
std::string password = ""; // Kendi şifrenizi girin
std::string database = "metin2";
```

## Çalıştırma

```bash
./test_auth
```

## Test Edilen Özellikler

1. ✅ Veritabanı bağlantısı
2. ✅ Hesap kaydı (register)
3. ✅ Kullanıcı girişi (login)
4. ✅ Session doğrulama
5. ✅ Karakter listesi
6. ✅ Karakter oluşturma
7. ✅ Karakter seçme
8. ✅ Oyuncu verilerini yükleme ve kaydetme
9. ✅ PvP istatistikleri
10. ✅ Çıkış yapma (logout)

## Beklenen Çıktı

```
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║            Metin2 PvP Server - Auth System Test               ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝

========================================
[INFO] Test 1: Veritabanı Bağlantısı
========================================
[SUCCESS] Veritabanı bağlantısı başarılı!

...

╔════════════════════════════════════════╗
║          TEST SONUÇLARI                ║
╚════════════════════════════════════════╝
Geçen Testler: 10 / 10
[SUCCESS] Tüm testler başarıyla geçti!
```

## Sorun Giderme

### MySQL bağlantı hatası
- MySQL'in çalıştığından emin olun: `sudo systemctl status mysql`
- Kullanıcı adı ve şifrenin doğru olduğunu kontrol edin

### Derleme hatası
```bash
# Gerekli kütüphaneleri yükleyin
sudo apt-get install libmysqlclient-dev libssl-dev
```
