# Metin2 PvP Server - Teknik Mimari Dokümantasyonu

## İçindekiler
1. [Genel Bakış](#genel-bakış)
2. [Sistem Mimarisi](#sistem-mimarisi)
3. [Veritabanı Katmanı](#veritabanı-katmanı)
4. [Oyun Sunucusu Katmanı](#oyun-sunucusu-katmanı)
5. [Quest Sistemi](#quest-sistemi)
6. [Veri Akışı](#veri-akışı)
7. [Thread Güvenliği](#thread-güvenliği)

## Genel Bakış

Metin2 PvP Server, modern C++17 standartları ve Lua scripting kullanarak geliştirilmiş, modüler bir MMORPG sunucu mimarisidir.

### Temel Prensipler
- **Modülerlik**: Her bileşen bağımsız çalışabilir
- **Ölçeklenebilirlik**: Yatay ölçekleme desteği
- **Güvenlik**: SQL Injection ve diğer saldırılara karşı korumalı
- **Performans**: Thread-safe tasarım ve optimize edilmiş algoritmalar

## Sistem Mimarisi

```
┌─────────────────────────────────────────────────┐
│                   Game Client                    │
└─────────────────────┬───────────────────────────┘
                      │
                      │ Network Protocol
                      │
┌─────────────────────▼───────────────────────────┐
│              Game Server (main.cpp)              │
│  ┌──────────────────────────────────────────┐   │
│  │         CGameServer (Singleton)          │   │
│  │  - Character Management                  │   │
│  │  - World Update Loop                     │   │
│  │  - Event Processing                      │   │
│  └────┬─────────────────────────────────┬───┘   │
│       │                                 │       │
│  ┌────▼──────────┐            ┌────────▼────┐   │
│  │  CCharacter   │            │ CQuestMgr   │   │
│  │  - Stats      │            │ - Lua VM    │   │
│  │  - Combat     │            │ - Scripts   │   │
│  │  - Position   │            │             │   │
│  └────┬──────────┘            └─────────────┘   │
│       │                                         │
└───────┼─────────────────────────────────────────┘
        │
        │ Database Operations
        │
┌───────▼─────────────────────────────────────────┐
│           CDBManager (Singleton)                 │
│  - Connection Pool                               │
│  - Query Execution                               │
│  - Transaction Management                        │
└────────────────┬────────────────────────────────┘
                 │
                 │ MySQL Protocol
                 │
┌────────────────▼────────────────────────────────┐
│              MySQL/MariaDB                       │
│  - account, player, item tables                 │
│  - quest, guild, pvp_log tables                 │
└─────────────────────────────────────────────────┘
```

## Veritabanı Katmanı

### CDBManager (include/db/DBManager.h)

Singleton pattern kullanılarak implement edilmiş veritabanı yöneticisi.

#### Özellikler:
- **Bağlantı Yönetimi**: Otomatik yeniden bağlanma
- **Query Güvenliği**: Prepared statements ve escape fonksiyonları
- **Hata Yönetimi**: Detaylı hata loglaması

#### Kullanım Örneği:
```cpp
// Başlatma
CDBManager::Instance().Initialize("localhost", "user", "pass", "db", 3306);

// Sorgu çalıştırma
MYSQL_RES* res = CDBManager::Instance().Query(
    "SELECT * FROM player WHERE id=%u", player_id
);

// Güvenli string escape
std::string safe = CDBManager::Instance().EscapeString(user_input);
```

#### Temel Metodlar:
1. `Initialize()` - Veritabanı bağlantısını kurar
2. `Query()` - SELECT sorguları için, sonuç döndürür
3. `Execute()` - INSERT/UPDATE/DELETE için, başarı durumu döndürür
4. `EscapeString()` - SQL injection koruması

### Veritabanı Şeması

#### account Tablosu
```sql
- id: Benzersiz hesap kimliği
- login: Kullanıcı adı (UNIQUE)
- password: SHA256 hash'lenmiş şifre
- status: Hesap durumu (OK/BLOCK)
```

#### player Tablosu
```sql
- id: Benzersiz oyuncu kimliği
- account_id: Bağlı hesap
- name: Karakter adı (UNIQUE)
- job: Karakter sınıfı (0-3)
- level: Seviye (1-99)
- exp, gold: Oyun içi kaynaklar
- hp/max_hp, sp/max_sp: Can ve mana
- attack, defense, magic_*: Savaş istatistikleri
- pvp_kills, pvp_deaths: PvP performansı
```

## Oyun Sunucusu Katmanı

### CGameServer (include/game/GameServer.h)

Ana oyun döngüsünü ve karakter yönetimini sağlar.

#### Sorumluluklari:
1. **Karakter Yönetimi**: Spawn/Despawn, tracking
2. **Oyun Döngüsü**: 25 FPS update cycle
3. **Broadcast**: Oyunculara mesaj gönderme
4. **Alan Yönetimi**: Yakındaki oyuncuları bulma

#### Ana Döngü (MainLoop):
```cpp
void CGameServer::MainLoop()
{
    const int FPS = 25;
    const int FRAME_TIME = 1000 / FPS; // 40ms

    while (m_bRunning)
    {
        auto start = now();

        Update();  // Tüm karakterleri güncelle

        auto elapsed = now() - start;
        if (elapsed < FRAME_TIME)
            sleep(FRAME_TIME - elapsed);
    }
}
```

#### Thread Güvenliği:
```cpp
std::mutex m_mutexCharacters;  // Karakter haritası koruması

CCharacter* FindCharacter(DWORD id)
{
    std::lock_guard<std::mutex> lock(m_mutexCharacters);
    return m_mapCharacters[id];
}
```

### CCharacter (include/game/Character.h)

Her oyuncuyu temsil eden sınıf.

#### Bileşenler:
1. **Temel Bilgiler**: ID, isim, sınıf
2. **Pozisyon**: X, Y, Z koordinatları
3. **İstatistikler**: HP, SP, Attack, Defense, vb.
4. **Durum**: IDLE, MOVING, ATTACKING, DEAD
5. **PvP**: Mod, kills, deaths

#### Savaş Sistemi:

```cpp
bool CCharacter::Attack(CCharacter* victim)
{
    // 1. Saldırı kontrolü
    if (!CanAttack(victim))
        return false;

    // 2. Saldırı hızı kontrolü
    if (elapsed < m_dwAttackSpeed)
        return false;

    // 3. Hasar hesaplama
    DWORD damage = CalculateDamage(victim);

    // 4. Hasarı uygula
    victim->OnDamage(this, damage);

    return true;
}

DWORD CCharacter::CalculateDamage(CCharacter* victim)
{
    // Basit hasar formülü:
    // Damage = (Attack - Defense) * Variance
    LONG base_damage = m_stats.attack - victim->GetDefense();

    // Minimum hasar
    if (base_damage < 10)
        base_damage = 10;

    // %80-120 varyans
    int variance = (rand() % 41) - 20;
    return base_damage * (100 + variance) / 100;
}
```

#### Seviye Sistemi:

```cpp
void CCharacter::CheckLevelUp()
{
    DWORD needed_exp = m_stats.level * 1000;

    while (m_stats.exp >= needed_exp && m_stats.level < 99)
    {
        m_stats.level++;
        m_stats.exp -= needed_exp;

        // İstatistik bonusları
        m_stats.attack += 5;
        m_stats.defense += 3;
        // ...

        // HP/SP yenile
        CalculateMaxHP();
        m_stats.hp = m_stats.max_hp;
    }
}
```

## Quest Sistemi

### CQuestManager (include/game/QuestManager.h)

Lua 5.3 kullanarak quest scriptlerini yönetir.

#### Mimari:
```
┌──────────────────┐
│  C++ Game Code   │
│                  │
│  QuestManager    │
└────────┬─────────┘
         │
         │ Lua C API
         │
┌────────▼─────────┐
│   Lua VM         │
│   (lua_State)    │
└────────┬─────────┘
         │
         │ Load Scripts
         │
┌────────▼─────────┐
│  Quest Scripts   │
│  (.lua files)    │
└──────────────────┘
```

#### Kayıtlı Fonksiyonlar:

```cpp
void CQuestManager::RegisterLuaFunctions()
{
    // pc namespace oluştur
    lua_newtable(L);

    // Fonksiyonları kaydet
    lua_pushcfunction(L, lua_GetLevel);
    lua_setfield(L, -2, "get_level");

    lua_pushcfunction(L, lua_GetGold);
    lua_setfield(L, -2, "get_gold");

    // ... diğer fonksiyonlar

    lua_setglobal(L, "pc");  // Global namespace'e ekle
}
```

#### Lua-C++ Köprü:

```cpp
// C++ tarafı
static CCharacter* g_pCurrentCharacter = nullptr;

int CQuestManager::lua_GetLevel(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        lua_pushnumber(L, g_pCurrentCharacter->GetLevel());
        return 1;  // 1 değer döndür
    }
    return 0;  // Değer döndürme
}
```

```lua
-- Lua tarafı
local level = pc.get_level()  -- C++ fonksiyonunu çağırır
pc.notice("Your level is: " .. level)
```

#### Quest Yapısı:

```lua
quest example_quest begin
    state start begin
        when login begin
            -- Quest başlatma kodu
            set_state(progress)
        end
    end

    state progress begin
        when kill begin
            -- Monster öldürme kodu
            if condition then
                set_state(complete)
            end
        end
    end

    state complete begin
        when button begin
            -- Ödül verme
            pc.change_gold(1000)
            pc.give_exp(500)
        end
    end
end
```

## Veri Akışı

### Oyuncu Girişi:
```
1. Client → Login Request
2. Server → CDBManager::CheckAccount()
3. Database → Verify credentials
4. Server → CDBManager::LoadPlayer()
5. Server → new CCharacter()
6. Server → CCharacter::Initialize()
7. Server → CGameServer::AddCharacter()
8. Server → CGameServer::SpawnCharacter()
9. Server → Broadcast character info to nearby players
10. Client ← Login Success + Character Data
```

### PvP Savaş:
```
1. Client → Attack Command (target_id)
2. Server → CGameServer::ProcessAttack()
3. Server → attacker->Attack(victim)
4. Server → CanAttack() check
5. Server → CalculateDamage()
6. Server → victim->OnDamage()
7. Server → CGameServer::BroadcastAttack()
8. Server → Check if victim died
9. If dead → attacker->OnKill(), victim->OnDeath()
10. Server → CDBManager::UpdatePvPStats()
11. Database → Update pvp_kills, pvp_deaths
12. Client ← Damage info, animations
```

### Quest Çalıştırma:
```
1. Trigger (login, kill, etc.)
2. Server → CQuestManager::RunQuest()
3. Server → Set g_pCurrentCharacter
4. Lua VM → Execute quest script
5. Lua → Call pc.* functions
6. C++ → Process function calls
7. C++ → Update character state
8. Lua ← Return values
9. Lua VM → Continue execution
10. Server → Unset g_pCurrentCharacter
```

## Thread Güvenliği

### Mutex Kullanımı:

```cpp
class CGameServer
{
private:
    std::map<DWORD, CCharacter*> m_mapCharacters;
    std::mutex m_mutexCharacters;  // Koruma

public:
    void AddCharacter(CCharacter* ch)
    {
        std::lock_guard<std::mutex> lock(m_mutexCharacters);
        m_mapCharacters[ch->GetPlayerID()] = ch;
    }  // lock otomatik serbest bırakılır
};
```

### Singleton Thread Safety:

```cpp
CGameServer& CGameServer::Instance()
{
    static CGameServer instance;  // C++11'den itibaren thread-safe
    return instance;
}
```

## Performans Optimizasyonları

### 1. Mesafe Hesaplama:
```cpp
DWORD GetDistance(const CCharacter* ch) const
{
    LONG dx = m_position.x - ch->m_position.x;
    LONG dy = m_position.y - ch->m_position.y;
    return (DWORD)sqrt(dx * dx + dy * dy);
}
```

### 2. Alan Sorguları:
```cpp
std::vector<CCharacter*> GetNearbyCharacters(const TPosition& pos, DWORD range)
{
    // Sadece menzil içindeki karakterleri döndür
    // TODO: Quadtree veya spatial hashing ile optimize et
}
```

### 3. Veritabanı Batch Operations:
```cpp
// Tek tek kaydetme yerine batch kaydetme
for (auto& ch : characters)
    batch.Add(ch);
batch.Execute();
```

## Güvenlik Önlemleri

### 1. SQL Injection Koruması:
```cpp
std::string safe = CDBManager::Instance().EscapeString(user_input);
Query("SELECT * FROM player WHERE name='%s'", safe.c_str());
```

### 2. Pozisyon Doğrulama:
```cpp
bool ValidatePosition(const TPosition& pos)
{
    // Map sınırları içinde mi?
    if (pos.x < MIN_X || pos.x > MAX_X) return false;
    if (pos.y < MIN_Y || pos.y > MAX_Y) return false;
    return true;
}
```

### 3. Hız Kontrolü:
```cpp
bool CheckSpeed(CCharacter* ch, const TPosition& newPos)
{
    DWORD distance = ch->GetDistance(newPos);
    DWORD elapsed_time = GetElapsedTime();
    DWORD max_distance = MAX_SPEED * elapsed_time;

    return distance <= max_distance;
}
```

## Gelecek Geliştirmeler

1. **Network Layer**: Asenkron I/O (Boost.Asio)
2. **Spatial Indexing**: Quadtree/Octree
3. **Load Balancing**: Multi-channel support
4. **Caching**: Redis entegrasyonu
5. **Monitoring**: Prometheus metrics
6. **Testing**: Unit ve integration testler

## Kaynaklar

- C++17 Reference: https://en.cppreference.com/
- Lua 5.3 Manual: https://www.lua.org/manual/5.3/
- MySQL C API: https://dev.mysql.com/doc/c-api/8.0/en/
