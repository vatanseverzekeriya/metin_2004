# Metin2 PvP Server

Modern C++ ve Lua ile geliştirilmiş, açık kaynak Metin2 PvP sunucusu.

## Özellikler

### Temel Özellikler
- ✅ MySQL/MariaDB veritabanı entegrasyonu
- ✅ Oyuncu yönetim sistemi
- ✅ PvP savaş mekaniği
- ✅ Seviye sistemi ve deneyim kazanımı
- ✅ Lua quest sistemi
- ✅ Karakterler arası savaş
- ✅ PvP istatistik takibi

### Teknik Özellikler
- Modern C++17
- Thread-safe tasarım
- MySQL bağlantı havuzu
- Lua 5.3 scripting
- Modüler mimari

## Proje Yapısı

```
metin_2004/
├── src/                    # Kaynak kodlar
│   ├── game/              # Oyun sunucusu
│   │   ├── main.cpp       # Ana program
│   │   ├── Character.cpp  # Karakter yönetimi
│   │   ├── GameServer.cpp # Sunucu yönetimi
│   │   └── QuestManager.cpp # Quest sistemi
│   ├── db/                # Veritabanı
│   │   └── DBManager.cpp  # DB bağlantı yöneticisi
│   └── common/            # Ortak kod
├── include/               # Header dosyaları
│   ├── game/
│   ├── db/
│   └── common/
│       └── types.h        # Temel tipler
├── quest/                 # Lua quest scriptleri
│   ├── welcome_quest.lua
│   ├── pvp_tutorial.lua
│   └── daily_rewards.lua
├── sql/                   # Veritabanı şemaları
│   └── schema.sql
├── config/                # Konfigürasyon
│   └── server_config.conf
├── CMakeLists.txt         # CMake build dosyası
└── Makefile              # Make build dosyası
```

## Kurulum

### Gereksinimler

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libmysqlclient-dev \
    liblua5.3-dev \
    pkg-config \
    mysql-server
```

#### CentOS/RHEL:
```bash
sudo yum install -y \
    gcc-c++ \
    cmake \
    mysql-devel \
    lua-devel
```

### Otomatik Kurulum
```bash
make install
```

## Derleme

### Make ile:
```bash
make
```

### CMake ile:
```bash
mkdir build && cd build
cmake ..
make
```

## Veritabanı Kurulumu

### 1. MySQL Kullanıcısı Oluştur
```bash
mysql -u root -p
```

```sql
CREATE USER 'metin2'@'localhost' IDENTIFIED BY 'metin2pass';
GRANT ALL PRIVILEGES ON metin2.* TO 'metin2'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

### 2. Veritabanı Şemasını Yükle
```bash
mysql -u metin2 -p < sql/schema.sql
```

Veya Make kullanarak:
```bash
make db-setup
```

## Kullanım

### Sunucuyu Başlatma
```bash
./bin/game_server
```

Veya Make ile:
```bash
make run
```

### Test Karakterleri
Sunucu başlatıldığında otomatik olarak iki test karakteri oluşturulur:
- **TestWarrior** (Savaşçı)
- **TestAssassin** (Suikastçı)

### PvP Test Senaryosu
Sunucu başlatıldığında otomatik olarak bir PvP test senaryosu çalıştırılır ve sonuçlar konsola yazdırılır.

## Kod Örnekleri

### 1. Veritabanı Bağlantısı

```cpp
#include "include/db/DBManager.h"

// Veritabanına bağlan
CDBManager::Instance().Initialize(
    "localhost",    // host
    "metin2",       // user
    "metin2pass",   // password
    "metin2",       // database
    3306            // port
);

// Oyuncu kaydet
TPlayerStats stats;
TPosition pos(957200, 244900, 0);
CDBManager::Instance().SavePlayer(player_id, stats, pos);
```

### 2. Karakter Oluşturma ve Yönetimi

```cpp
#include "include/game/Character.h"

// Yeni karakter oluştur
CCharacter* warrior = new CCharacter();
DWORD player_id;

if (CDBManager::Instance().CreatePlayer("MyWarrior", CLASS_WARRIOR, player_id))
{
    warrior->Initialize(player_id);
    warrior->SetPosition(957200, 244900, 0);
    warrior->SetPvPMode(PVP_MODE_NORMAL);

    // Sunucuya ekle
    CGameServer::Instance().AddCharacter(warrior);
    CGameServer::Instance().SpawnCharacter(warrior);
}
```

### 3. PvP Savaş

```cpp
// İki karakter arası savaş
CCharacter* attacker = CGameServer::Instance().FindCharacterByName("Warrior1");
CCharacter* victim = CGameServer::Instance().FindCharacterByName("Warrior2");

if (attacker->CanAttack(victim))
{
    attacker->Attack(victim);
}
```

### 4. Lua Quest Örneği

```lua
quest my_first_quest begin
    state start begin
        when login begin
            pc.notice("Hoş geldin " .. pc.get_name() .. "!")

            -- Oyuncuya ödül ver
            pc.change_gold(10000)
            pc.give_exp(500)

            pc.notice("10,000 Yang ve 500 EXP kazandın!")
            set_state(completed)
        end
    end

    state completed begin
        when login begin
            pc.notice("Quest tamamlandı!")
        end
    end
end
```

## API Dokümantasyonu

### Veritabanı İşlemleri (DBManager)

#### Oyuncu İşlemleri
- `bool SavePlayer(DWORD player_id, const TPlayerStats& stats, const TPosition& pos)`
- `bool LoadPlayer(DWORD player_id, TPlayerStats& stats, TPosition& pos)`
- `bool CreatePlayer(const std::string& name, BYTE job, DWORD& out_player_id)`

#### Hesap İşlemleri
- `bool CheckAccount(const std::string& login, const std::string& password)`
- `DWORD GetAccountID(const std::string& login)`

#### PvP İstatistikleri
- `bool UpdatePvPStats(DWORD player_id, DWORD kills, DWORD deaths)`
- `bool GetPvPStats(DWORD player_id, DWORD& kills, DWORD& deaths)`

### Karakter İşlemleri (Character)

#### Temel Bilgiler
- `DWORD GetPlayerID() const`
- `const std::string& GetName() const`
- `BYTE GetJob() const`
- `DWORD GetLevel() const`

#### Pozisyon
- `const TPosition& GetPosition() const`
- `void SetPosition(LONG x, LONG y, LONG z = 0)`
- `bool MoveTo(LONG x, LONG y)`

#### İstatistikler
- `DWORD GetHP() const`
- `void SetHP(DWORD hp)`
- `DWORD GetExp() const`
- `void GiveExp(DWORD exp)`
- `DWORD GetGold() const`
- `bool ChangeGold(LONG amount)`

#### Savaş
- `bool Attack(CCharacter* victim)`
- `bool CanAttack(CCharacter* victim)`
- `DWORD CalculateDamage(CCharacter* victim)`
- `void OnDamage(CCharacter* attacker, DWORD damage)`

### Lua Quest API

#### Oyuncu Bilgileri
- `pc.get_name()` - Oyuncu adını döndürür
- `pc.get_level()` - Seviyeyi döndürür
- `pc.get_hp()` - Can miktarını döndürür
- `pc.get_gold()` - Yang miktarını döndürür

#### Oyuncu İşlemleri
- `pc.change_gold(amount)` - Yang ekler/çıkarır
- `pc.give_exp(amount)` - Deneyim verir
- `pc.set_hp(amount)` - Can ayarlar
- `pc.teleport(x, y)` - Işınlama yapar

#### Mesajlar
- `pc.chat(message)` - Chat mesajı gönderir
- `pc.notice(message)` - Bildirim gönderir

## Konfigürasyon

`config/server_config.conf` dosyasını düzenleyerek sunucu ayarlarını yapabilirsiniz:

```ini
[SERVER]
PORT = 13000
MAX_PLAYERS = 1000

[DATABASE]
DB_HOST = localhost
DB_PORT = 3306
DB_USER = metin2
DB_PASSWORD = metin2pass
DB_NAME = metin2

[GAME]
EXP_RATE = 10
GOLD_RATE = 10
PVP_ENABLED = true
MAX_LEVEL = 99
```

## Karakter Sınıfları

| Sınıf | ID | Açıklama |
|-------|----|-----------
| Warrior | 0 | Yakın dövüş uzmanı |
| Assassin | 1 | Hızlı ve çevik |
| Sura | 2 | Büyü ve kılıç dengesi |
| Shaman | 3 | Büyücü ve şifacı |

## PvP Modları

| Mod | Açıklama |
|-----|----------|
| PVP_MODE_NONE | PvP kapalı |
| PVP_MODE_NORMAL | Herkes ile savaş |
| PVP_MODE_GUILD | Lonca savaşları |
| PVP_MODE_PARTY | Grup dışı savaş |

## Veritabanı Şeması

### account Tablosu
- `id` - Hesap ID
- `login` - Kullanıcı adı
- `password` - Şifreli parola
- `email` - E-posta
- `status` - Hesap durumu

### player Tablosu
- `id` - Oyuncu ID
- `account_id` - Bağlı hesap
- `name` - Karakter adı
- `job` - Meslek
- `level` - Seviye
- `exp` - Deneyim
- `gold` - Yang
- `hp/max_hp` - Can
- `pvp_kills/pvp_deaths` - PvP istatistikleri

### item Tablosu
- `id` - Item ID
- `owner_id` - Sahibi
- `vnum` - Item numarası
- `count` - Adet
- Soketler ve bonuslar

## Geliştirme

### Yeni Quest Ekleme

1. `quest/` dizininde yeni `.lua` dosyası oluştur
2. Quest yapısını tanımla
3. `QuestManager::LoadAllQuests()` içine ekle

### Yeni Özellik Ekleme

1. Header dosyasını `include/` altına ekle
2. Implementasyonu `src/` altına ekle
3. `CMakeLists.txt` veya `Makefile` güncelle

## Sorun Giderme

### MySQL Bağlantı Hatası
```
Hata: Failed to connect to database
Çözüm: MySQL servisinin çalıştığından ve kullanıcı bilgilerinin doğru olduğundan emin olun
```

### Derleme Hatası
```
Hata: mysql.h: No such file or directory
Çözüm: sudo apt-get install libmysqlclient-dev
```

### Lua Hatası
```
Hata: lua.h: No such file or directory
Çözüm: sudo apt-get install liblua5.3-dev
```

## Performans

- 25 FPS (40ms per frame)
- 1000+ eşzamanlı oyuncu desteği
- Thread-safe operasyonlar
- Verimli veritabanı sorguları

## Güvenlik

- SQL Injection koruması
- Şifreli parolalar
- Hız hilesi koruması
- Pozisyon doğrulama

## Lisans

Bu proje eğitim amaçlıdır. Ticari kullanım için uygun değildir.

## Katkıda Bulunma

1. Fork yapın
2. Feature branch oluşturun (`git checkout -b feature/amazing-feature`)
3. Commit yapın (`git commit -m 'Add amazing feature'`)
4. Push edin (`git push origin feature/amazing-feature`)
5. Pull Request açın

## İletişim

Sorularınız için GitHub Issues kullanabilirsiniz.

## Teşekkürler

Bu proje Metin2 topluluğu için geliştirilmiştir.
