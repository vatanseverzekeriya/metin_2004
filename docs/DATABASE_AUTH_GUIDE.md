# Metin2 PvP Server - Veritabanı ve Kimlik Doğrulama Sistemi Rehberi

## İçindekiler
1. [Sistem Mimarisi](#sistem-mimarisi)
2. [Veritabanı Bağlantısı](#veritabanı-bağlantısı)
3. [Hesap Kayıt Sistemi](#hesap-kayıt-sistemi)
4. [Giriş ve Oturum Yönetimi](#giriş-ve-oturum-yönetimi)
5. [Oyuncu Verileri İşlemleri](#oyuncu-verileri-işlemleri)
6. [Kullanım Örnekleri](#kullanım-örnekleri)

---

## Sistem Mimarisi

Kimlik doğrulama ve veritabanı sistemi üç ana bileşenden oluşur:

### 1. CDBManager (Veritabanı Yöneticisi)
- **Dosyalar**: `include/db/DBManager.h`, `src/db/DBManager.cpp`
- **Görevler**:
  - MySQL veritabanı bağlantı yönetimi
  - SQL sorguları çalıştırma (Query, Execute)
  - Oyuncu ve hesap CRUD işlemleri
  - Password hashing (SHA256)

### 2. CSessionManager (Oturum Yöneticisi)
- **Dosyalar**: `include/auth/SessionManager.h`, `src/auth/SessionManager.cpp`
- **Görevler**:
  - Oturum anahtarı oluşturma ve doğrulama
  - Aktif oturumları takip etme
  - Oturum zaman aşımı yönetimi
  - Thread-safe oturum işlemleri (mutex kullanımı)

### 3. CAuthManager (Kimlik Doğrulama Yöneticisi)
- **Dosyalar**: `include/auth/AuthManager.h`, `src/auth/AuthManager.cpp`
- **Görevler**:
  - Hesap kaydı (register)
  - Kullanıcı girişi (login)
  - Karakter yönetimi (oluşturma, silme, seçme)
  - Validasyon kontrolleri

---

## Veritabanı Bağlantısı

### Veritabanı Şeması Kurulumu

```bash
# MySQL'e bağlan ve şemayı yükle
mysql -u root -p < database/schema.sql
```

Şema şunları oluşturur:
- `account` - Oyuncu hesapları
- `player` - Oyuncu karakterleri
- `inventory` - Envanter sistemi
- `pvp_log` - PvP kayıtları
- `rankings` - Sıralama tabloları
- `guild` ve `guild_member` - Lonca sistemi

### C++ ile Bağlantı Kurma

```cpp
#include "include/db/DBManager.h"

// Singleton instance al
CDBManager& db = CDBManager::Instance();

// Veritabanına bağlan
bool success = db.Initialize(
    "localhost",  // host
    "root",       // user
    "password",   // password
    "metin2",     // database
    3306          // port
);

if (success) {
    std::cout << "Veritabanı bağlantısı başarılı!" << std::endl;
}
```

### Bağlantı Özellikleri

- **Auto-reconnect**: Bağlantı koptuğunda otomatik yeniden bağlanma
- **UTF-8 Encoding**: Türkçe karakter desteği
- **Connection pooling**: Singleton pattern ile tek bağlantı yönetimi

---

## Hesap Kayıt Sistemi

### Hesap Oluşturma

```cpp
#include "include/auth/AuthManager.h"

CAuthManager& auth = CAuthManager::Instance();

// Yeni hesap kaydet
ERegisterResult result = auth.RegisterAccount(
    "kullanici_adi",           // login (4-16 karakter, alfanumerik)
    "sifre123",                // password (6-32 karakter, rakam+harf)
    "email@example.com"        // email
);

switch (result) {
    case REGISTER_SUCCESS:
        std::cout << "Hesap başarıyla oluşturuldu!" << std::endl;
        break;

    case REGISTER_FAILED_ALREADY_EXISTS:
        std::cout << "Bu kullanıcı adı zaten kullanılıyor!" << std::endl;
        break;

    case REGISTER_FAILED_INVALID_LOGIN:
        std::cout << "Geçersiz kullanıcı adı formatı!" << std::endl;
        break;

    case REGISTER_FAILED_INVALID_PASSWORD:
        std::cout << "Şifre en az 6 karakter ve bir rakam içermelidir!" << std::endl;
        break;

    case REGISTER_FAILED_INVALID_EMAIL:
        std::cout << "Geçersiz email adresi!" << std::endl;
        break;

    case REGISTER_FAILED_DB_ERROR:
        std::cout << "Veritabanı hatası!" << std::endl;
        break;
}
```

### Password Güvenliği

Sistemde **SHA256 hash** algoritması kullanılır:

```cpp
CDBManager& db = CDBManager::Instance();

// Şifre hashle
std::string hashed = db.HashPassword("kullanici_sifresi");
// Çıktı: 64 karakter hex string
// Örnek: "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"
```

**Güvenlik Özellikleri:**
- Şifreler veritabanında asla düz metin olarak saklanmaz
- SHA256 tek yönlü şifreleme (geri çevrilemez)
- SQL injection koruması (mysql_real_escape_string)

### Validasyon Kuralları

```cpp
CAuthManager& auth = CAuthManager::Instance();

// Kullanıcı adı kontrolü
bool valid_login = auth.IsValidLoginName("testuser");
// Kurallar: 4-16 karakter, sadece a-z, A-Z, 0-9, _

// Şifre kontrolü
bool valid_pass = auth.IsValidPassword("test123");
// Kurallar: 6-32 karakter, en az 1 rakam + 1 harf

// Email kontrolü
bool valid_email = auth.IsValidEmail("user@example.com");
// Kurallar: Standart email formatı (regex)

// Karakter ismi kontrolü
bool valid_char = auth.IsValidCharacterName("Warrior123");
// Kurallar: 3-16 karakter, sadece alfanumerik
```

---

## Giriş ve Oturum Yönetimi

### Kullanıcı Girişi (Login)

```cpp
#include "include/auth/AuthManager.h"

CAuthManager& auth = CAuthManager::Instance();

std::string session_key;

// Giriş yap
EAuthResult result = auth.Login(
    "kullanici_adi",     // login
    "sifre123",          // password
    "192.168.1.100",     // IP adresi
    session_key          // [OUT] session key
);

if (result == AUTH_SUCCESS) {
    std::cout << "Giriş başarılı!" << std::endl;
    std::cout << "Session Key: " << session_key << std::endl;
    // Session key: 32 karakter hex string (örnek: a3f2d9c8b1e4...)
}
```

### Session Yönetimi

```cpp
#include "include/auth/SessionManager.h"

CSessionManager& session_mgr = CSessionManager::Instance();

// Session geçerli mi kontrol et
bool is_valid = session_mgr.ValidateSession(session_key);

// Session bilgilerini al
TSessionInfo* session = session_mgr.GetSession(session_key);
if (session) {
    std::cout << "Account ID: " << session->account_id << std::endl;
    std::cout << "Login: " << session->login << std::endl;
    std::cout << "IP: " << session->ip_address << std::endl;
    std::cout << "Aktif Karakter: " << session->current_player_id << std::endl;
}

// Aktiviteyi güncelle (timeout'u sıfırla)
session_mgr.UpdateActivity(session_key);

// Session timeout ayarla (saniye cinsinden)
session_mgr.SetSessionTimeout(3600); // 1 saat

// Süresi dolmuş oturumları temizle
session_mgr.CleanupExpiredSessions();
```

### Session Özellikleri

- **Otomatik Timeout**: Varsayılan 1 saat (3600 saniye)
- **Activity Tracking**: Her işlemde son aktivite zamanı güncellenir
- **Tek Oturum**: Bir hesap aynı anda sadece bir yerden giriş yapabilir
- **Thread-Safe**: Mutex kullanarak çoklu thread desteği

### Çıkış (Logout)

```cpp
CAuthManager& auth = CAuthManager::Instance();

// Oturumu kapat
bool success = auth.Logout(session_key);

if (success) {
    std::cout << "Çıkış başarılı!" << std::endl;
    // Session key artık geçersiz
}
```

---

## Oyuncu Verileri İşlemleri

### Karakter Listesi

```cpp
CAuthManager& auth = CAuthManager::Instance();

std::vector<std::pair<DWORD, std::string>> characters;

// Hesaba ait karakterleri al
if (auth.GetCharacterList(session_key, characters)) {
    std::cout << "Karakterler:" << std::endl;
    for (const auto& ch : characters) {
        std::cout << "  ID: " << ch.first
                  << " - İsim: " << ch.second << std::endl;
    }
}
```

### Karakter Oluşturma

```cpp
DWORD player_id = 0;

// Yeni karakter oluştur
ECreateCharacterResult result = auth.CreateCharacter(
    session_key,           // session key
    "MyWarrior",          // karakter ismi
    CLASS_WARRIOR,        // sınıf (0=Warrior, 1=Assassin, 2=Sura, 3=Shaman)
    player_id             // [OUT] oluşturulan karakter ID'si
);

if (result == CREATE_CHARACTER_SUCCESS) {
    std::cout << "Karakter oluşturuldu! ID: " << player_id << std::endl;
}

// Maksimum 4 karakter sınırı vardır
```

### Karakter Seçme

```cpp
// Oyuna girerken karakter seç
bool success = auth.SelectCharacter(session_key, player_id);

if (success) {
    // Artık bu karakterle oynuyorsunuz
    // Session'da current_player_id ayarlanır
}
```

### Karakter Silme

```cpp
bool success = auth.DeleteCharacter(session_key, player_id);

if (success) {
    std::cout << "Karakter silindi!" << std::endl;
}
```

### Oyuncu Verilerini Yükleme

```cpp
#include "include/db/DBManager.h"
#include "include/common/types.h"

CDBManager& db = CDBManager::Instance();

TPlayerStats stats;
TPosition pos;

// Veritabanından yükle
if (db.LoadPlayer(player_id, stats, pos)) {
    std::cout << "Seviye: " << stats.level << std::endl;
    std::cout << "HP: " << stats.hp << "/" << stats.max_hp << std::endl;
    std::cout << "Altın: " << stats.gold << std::endl;
    std::cout << "Pozisyon: (" << pos.x << ", " << pos.y << ")" << std::endl;
}
```

### Oyuncu Verilerini Kaydetme

```cpp
// İstatistikleri değiştir
stats.level = 10;
stats.exp = 50000;
stats.gold = 100000;
stats.hp = 2000;

pos.x = 957300;
pos.y = 245000;

// Veritabanına kaydet
if (db.SavePlayer(player_id, stats, pos)) {
    std::cout << "Veriler kaydedildi!" << std::endl;
}
```

### PvP İstatistikleri

```cpp
CDBManager& db = CDBManager::Instance();

// PvP istatistiklerini al
DWORD kills = 0, deaths = 0;
if (db.GetPvPStats(player_id, kills, deaths)) {
    float kd_ratio = (deaths > 0) ? (float)kills / deaths : (float)kills;
    std::cout << "Öldürme: " << kills << std::endl;
    std::cout << "Ölüm: " << deaths << std::endl;
    std::cout << "K/D Oranı: " << kd_ratio << std::endl;
}

// PvP istatistiklerini güncelle
kills += 1; // Bir oyuncu öldürdü
db.UpdatePvPStats(player_id, kills, deaths);
```

---

## Kullanım Örnekleri

### Örnek 1: Tam Kayıt ve Giriş Akışı

```cpp
#include "include/db/DBManager.h"
#include "include/auth/AuthManager.h"

int main() {
    // 1. Veritabanı bağlantısı
    CDBManager& db = CDBManager::Instance();
    if (!db.Initialize("localhost", "root", "pass", "metin2", 3306)) {
        std::cerr << "DB bağlantı hatası!" << std::endl;
        return 1;
    }

    // 2. Hesap kaydı
    CAuthManager& auth = CAuthManager::Instance();
    ERegisterResult reg_result = auth.RegisterAccount(
        "newplayer", "secure123", "player@example.com"
    );

    if (reg_result != REGISTER_SUCCESS) {
        std::cerr << "Kayıt başarısız!" << std::endl;
        return 1;
    }

    // 3. Giriş
    std::string session_key;
    EAuthResult login_result = auth.Login(
        "newplayer", "secure123", "127.0.0.1", session_key
    );

    if (login_result != AUTH_SUCCESS) {
        std::cerr << "Giriş başarısız!" << std::endl;
        return 1;
    }

    std::cout << "Giriş başarılı! Session: " << session_key << std::endl;

    // 4. Karakter oluştur
    DWORD player_id;
    auth.CreateCharacter(session_key, "Hero", CLASS_WARRIOR, player_id);

    // 5. Karakter seç
    auth.SelectCharacter(session_key, player_id);

    // 6. Oyuna gir...
    // Oyun mantığı burada

    // 7. Çıkış
    auth.Logout(session_key);

    return 0;
}
```

### Örnek 2: Oyuncu Verilerini Periyodik Kaydetme

```cpp
#include <chrono>
#include <thread>

void AutoSaveLoop(DWORD player_id) {
    CDBManager& db = CDBManager::Instance();

    while (game_running) {
        // Her 5 dakikada bir otomatik kaydet
        std::this_thread::sleep_for(std::chrono::minutes(5));

        // Mevcut oyuncu verilerini al
        TPlayerStats current_stats = GetCurrentPlayerStats();
        TPosition current_pos = GetCurrentPlayerPosition();

        // Veritabanına kaydet
        if (db.SavePlayer(player_id, current_stats, current_pos)) {
            std::cout << "[AUTO-SAVE] Veriler kaydedildi." << std::endl;
        }
    }
}
```

### Örnek 3: PvP Kill Handling

```cpp
void OnPlayerKill(CCharacter* killer, CCharacter* victim) {
    CDBManager& db = CDBManager::Instance();

    DWORD killer_id = killer->GetPlayerID();
    DWORD victim_id = victim->GetPlayerID();

    // İstatistikleri al
    DWORD killer_kills, killer_deaths;
    DWORD victim_kills, victim_deaths;

    db.GetPvPStats(killer_id, killer_kills, killer_deaths);
    db.GetPvPStats(victim_id, victim_kills, victim_deaths);

    // Güncelle
    killer_kills++;
    victim_deaths++;

    db.UpdatePvPStats(killer_id, killer_kills, killer_deaths);
    db.UpdatePvPStats(victim_id, victim_kills, victim_deaths);

    // Log kaydet (stored procedure kullanarak)
    TPosition pos = victim->GetPosition();
    db.Execute("CALL sp_record_pvp_kill(%u, %u, %ld, %ld)",
               killer_id, victim_id, pos.x, pos.y);

    std::cout << killer->GetName() << " killed " << victim->GetName() << "!" << std::endl;
}
```

### Örnek 4: Session Cleanup (Arka Plan Görevi)

```cpp
#include <thread>

void SessionCleanupTask() {
    CAuthManager& auth = CAuthManager::Instance();

    while (server_running) {
        // Her 10 dakikada bir süresi dolmuş oturumları temizle
        std::this_thread::sleep_for(std::chrono::minutes(10));

        auth.CleanupExpiredSessions();

        CSessionManager& session_mgr = CSessionManager::Instance();
        int active = session_mgr.GetActiveSessionCount();

        std::cout << "[CLEANUP] Aktif oturum sayısı: " << active << std::endl;
    }
}

// Ana programda
int main() {
    // ...

    // Arka plan thread başlat
    std::thread cleanup_thread(SessionCleanupTask);
    cleanup_thread.detach();

    // Sunucu ana döngüsü
    ServerMainLoop();

    return 0;
}
```

---

## Test Programı

Sistemi test etmek için hazır bir test programı bulunmaktadır:

```bash
# Derleme
cd examples/auth_test
g++ -o test_auth test_auth_system.cpp \
    ../../src/db/DBManager.cpp \
    ../../src/auth/SessionManager.cpp \
    ../../src/auth/AuthManager.cpp \
    -I../../include \
    -lmysqlclient -lssl -lcrypto -lpthread \
    -std=c++11

# Çalıştırma
./test_auth
```

Test programı şunları test eder:
1. Veritabanı bağlantısı
2. Hesap kaydı
3. Giriş yapma
4. Session doğrulama
5. Karakter listesi
6. Karakter oluşturma
7. Karakter seçme
8. Oyuncu verileri yükleme/kaydetme
9. PvP istatistikleri
10. Çıkış yapma

---

## Veritabanı Sorguları

### Manuel Sorgu Çalıştırma

```cpp
CDBManager& db = CDBManager::Instance();

// SELECT sorgusu (sonuç döner)
MYSQL_RES* result = db.Query("SELECT id, name FROM player WHERE level > %d", 10);
if (result) {
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        DWORD id = atoi(row[0]);
        std::string name = row[1];
        std::cout << "ID: " << id << ", Name: " << name << std::endl;
    }
    mysql_free_result(result);
}

// INSERT/UPDATE/DELETE sorgusu (sonuç dönmez)
bool success = db.Execute(
    "UPDATE player SET gold = gold + %u WHERE id = %u",
    1000, player_id
);
```

### SQL Injection Koruması

```cpp
// Kullanıcı girdilerini her zaman escape et
std::string user_input = "Robert'; DROP TABLE player; --";
std::string safe_input = db.EscapeString(user_input);

// Güvenli sorgu
db.Execute("UPDATE player SET name = '%s' WHERE id = %u",
           safe_input.c_str(), player_id);
```

---

## Performans Optimizasyonu

### Bağlantı Havuzu (Gelecek Geliştirme)

Şu anda tek bağlantı kullanılıyor. Yüksek yük için bağlantı havuzu eklenebilir:

```cpp
// TODO: Connection pool implementasyonu
// - Birden fazla MySQL bağlantısı
// - Thread-safe queue sistemi
// - Otomatik load balancing
```

### Batch Operations

Toplu işlemler için:

```cpp
// Kötü (her işlem ayrı sorgu)
for (int i = 0; i < 1000; i++) {
    db.Execute("INSERT INTO ...");
}

// İyi (tek sorgu ile toplu ekleme)
db.Execute("INSERT INTO inventory (player_id, item_vnum, count) VALUES "
           "(1, 100, 5), (1, 101, 3), (1, 102, 10), ..."
);
```

### Prepared Statements (Gelecek Geliştirme)

```cpp
// TODO: MySQL prepared statements desteği
// - Daha iyi performans
// - Otomatik parametre escape
// - SQL injection koruması
```

---

## Hata Yönetimi

### Veritabanı Hataları

```cpp
CDBManager& db = CDBManager::Instance();

// Sorgu başarısız olursa
MYSQL_RES* result = db.Query("SELECT * FROM invalid_table");
if (!result) {
    // Hata mesajı otomatik olarak stderr'e yazılır
    // mysql_error() içeride kullanılır
}

// Execute başarısız olursa
bool success = db.Execute("UPDATE invalid_query");
if (!success) {
    // Hata mesajı otomatik olarak loglanır
}
```

### Session Hataları

```cpp
// Geçersiz session
if (!auth.ValidateSession(invalid_key)) {
    // Kullanıcıyı login sayfasına yönlendir
    return AUTH_FAILED_INVALID_SESSION;
}

// Süresi dolmuş session
TSessionInfo* session = session_mgr.GetSession(session_key);
if (!session) {
    // Session bulunamadı veya süresi doldu
    return AUTH_FAILED_INVALID_SESSION;
}
```

---

## Güvenlik Notları

1. **Password Storage**: SHA256 hash kullanılır, asla düz metin saklanmaz
2. **SQL Injection**: `EscapeString()` fonksiyonu her zaman kullanılmalı
3. **Session Keys**: Kriptografik olarak güvenli rastgele sayı üreteci
4. **Session Timeout**: Varsayılan 1 saat, ayarlanabilir
5. **IP Tracking**: Her oturum için IP adresi kaydedilir
6. **Account Locking**: (TODO) Başarısız giriş denemelerinde hesap kilitleme

---

## Bağımlılıklar

Sistemi derlemek için gereken kütüphaneler:

```bash
# Ubuntu/Debian
sudo apt-get install libmysqlclient-dev libssl-dev

# CentOS/RHEL
sudo yum install mysql-devel openssl-devel

# Derleme bayrakları
g++ -lmysqlclient -lssl -lcrypto -lpthread
```

---

## Sorun Giderme

### Veritabanı Bağlantı Hatası

```
MySQL connection failed: Access denied for user...
```

**Çözüm**: MySQL kullanıcı adı/şifre kontrolü, yetkiler

```sql
GRANT ALL PRIVILEGES ON metin2.* TO 'kullanici'@'localhost';
FLUSH PRIVILEGES;
```

### Karakter Encoding Hatası

```
Invalid character in database...
```

**Çözüm**: UTF-8 encoding kontrolü

```cpp
mysql_options(connection, MYSQL_SET_CHARSET_NAME, "utf8");
```

### Session Timeout Çok Kısa

```cpp
CSessionManager& session_mgr = CSessionManager::Instance();
session_mgr.SetSessionTimeout(7200); // 2 saat
```

---

## Gelişmiş Özellikler (Roadmap)

- [ ] Connection pooling
- [ ] Prepared statements
- [ ] Redis cache entegrasyonu
- [ ] Account locking (brute-force koruması)
- [ ] 2FA (Two-Factor Authentication)
- [ ] Email verification
- [ ] Password reset sistemi
- [ ] Audit logging (tüm işlemler loglanır)

---

## Lisans ve Destek

Bu sistem Metin2 PvP Server projesi için geliştirilmiştir.

**Geliştirici**: Claude AI
**Tarih**: 2025
**Versiyon**: 1.0
