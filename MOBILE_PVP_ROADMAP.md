# 📱 Metin2 Mobile PvP - Geliştirme Roadmap'i

## 🎯 Vizyon

**"Açık kaynak Metin2 PvP sisteminin mobil platformlar için modernize ve optimize edilmiş versiyonu"**

## 📊 Proje Durumu

### Halihazırda Tamamlanmış ✅
- [x] Temel Character sistemi (C++17)
- [x] PvP saldırı mekaniği (basit)
- [x] Level ve exp sistemi
- [x] MySQL veritabanı entegrasyonu
- [x] Touch input sistemi (multi-touch)
- [x] Virtual joystick (8 yön)
- [x] UI Button sistemi
- [x] Gesture detection
- [x] Network katmanı (TCP/UDP/WebSocket)
- [x] Android/iOS hazırlığı

### Eksik Olan (Açık Kaynaklardan Alınacak) 🔴
- [ ] CPVPManager (dedicated PvP yönetimi)
- [ ] PvP request/accept sistemi
- [ ] Revenge mode
- [ ] Duel system
- [ ] Guild war
- [ ] Empire/Kingdom sistemi
- [ ] Advanced targeting

---

## 🗺️ Geliştirme Fazları

## **FAZ 1: Core PvP Manager** (1-2 hafta) 🔥 PRİORİTE

**Referans**: cCorax2/Source_code (pvp.cpp)

### 1.1 PVPManager Sınıfı

**Dosya**: `include/game/PVPManager.h`

```cpp
#ifndef __INC_GAME_PVPMANAGER_H__
#define __INC_GAME_PVPMANAGER_H__

#include "../common/types.h"
#include <map>
#include <memory>

// PvP durumları
enum EPvPState {
    PVP_STATE_NONE = 0,
    PVP_STATE_REQUEST = 1,    // Bir taraf istek gönderdi
    PVP_STATE_AGREE = 2,       // İki taraf kabul etti
    PVP_STATE_FIGHT = 3,       // Aktif savaş
    PVP_STATE_REVENGE = 4      // İntikam modu (5 dakika)
};

// PvP ilişkisi
struct TPvPInfo {
    DWORD dwPID1;              // Oyuncu 1
    DWORD dwPID2;              // Oyuncu 2
    EPvPState eState;          // Durum
    DWORD dwStartTime;         // Başlama zamanı
    bool bRevenge;             // İntikam modu aktif mi
    DWORD dwRevengeEndTime;    // İntikam bitiş zamanı

    TPvPInfo() : dwPID1(0), dwPID2(0), eState(PVP_STATE_NONE),
                 dwStartTime(0), bRevenge(false), dwRevengeEndTime(0) {}
};

class CCharacter;

class CPVPManager {
public:
    static CPVPManager& Instance();

    // PvP işlemleri
    void Insert(DWORD dwPID1, DWORD dwPID2);      // Request gönder
    void Accept(DWORD dwPID1, DWORD dwPID2);      // Kabul et
    void Decline(DWORD dwPID1, DWORD dwPID2);     // Reddet
    void Remove(DWORD dwPID1, DWORD dwPID2);      // PvP sona erdi

    // Kontroller
    bool CanAttack(CCharacter* pkAttacker, CCharacter* pkVictim);
    bool IsFighting(DWORD dwPID1, DWORD dwPID2);
    EPvPState GetPvPState(DWORD dwPID1, DWORD dwPID2);

    // Events
    void OnDeath(CCharacter* pkDead, CCharacter* pkKiller);

    // Update
    void Process();  // Her 1 saniyede cleanup

private:
    CPVPManager() = default;
    ~CPVPManager() = default;
    CPVPManager(const CPVPManager&) = delete;
    CPVPManager& operator=(const CPVPManager&) = delete;

    DWORD GetCRC(DWORD dwPID1, DWORD dwPID2);
    TPvPInfo* Find(DWORD dwPID1, DWORD dwPID2);

    std::map<DWORD, TPvPInfo> m_map_pvp;  // CRC -> PvPInfo
    std::mutex m_mutex;
};

#endif
```

**Implementasyon**: `src/game/PVPManager.cpp`

### 1.2 PK Mode Genişletmesi

**Dosya**: `include/common/types.h` (güncelle)

```cpp
// Detaylı PK modları
enum EPKMode {
    PK_MODE_PEACE = 0,      // Barış - hiç saldırı yok
    PK_MODE_NORMAL = 1,     // Normal - PvP request ile
    PK_MODE_FREE = 2,       // Serbest - herkese saldırı
    PK_MODE_GUILD = 3,      // Guild - guild dışına saldırı
    PK_MODE_PARTY = 4       // Party - parti dışına saldırı
};
```

### 1.3 Character Güncellemesi

**Dosya**: `include/game/Character.h` (eklemeler)

```cpp
// Eklenecek metodlar
EPKMode GetPKMode() const { return m_ePKMode; }
void SetPKMode(EPKMode mode);

// PvP request
void SendPvPRequest(CCharacter* target);
void AcceptPvPRequest(CCharacter* requester);
void DeclinePvPRequest(CCharacter* requester);

private:
    EPKMode m_ePKMode;  // Yeni field
```

**Implementasyon**: Güncelle `CanAttack()` metodunu PVPManager kullanarak

### 📝 Görevler (Faz 1)

- [ ] `PVPManager.h` oluştur
- [ ] `PVPManager.cpp` implement et
- [ ] `types.h` EPKMode ekle
- [ ] `Character.h` güncelle (PK mode ekle)
- [ ] `Character.cpp` güncelle (PVPManager entegrasyonu)
- [ ] Unit testler yaz
- [ ] Dokümante et

**Tahmini Süre**: 5-7 gün

---

## **FAZ 2: Mobile PvP UI** (1 hafta)

### 2.1 PvP Request Dialog

**Dosya**: `include/mobile/PvPDialog.h`

```cpp
class CPvPDialog : public CUIWindow {
public:
    void Show(const std::string& requesterName);
    void SetOnAccept(std::function<void()> callback);
    void SetOnDecline(std::function<void()> callback);

private:
    CUIButton m_btnAccept;
    CUIButton m_btnDecline;
    std::string m_strRequester;
};
```

**UI Layout**:
```
┌─────────────────────────┐
│  PvP Challenge          │
│                         │
│  EnemyWarrior wants     │
│  to duel with you!      │
│                         │
│  [ Accept ] [ Decline ] │
└─────────────────────────┘
```

### 2.2 PK Mode Selector

**Dosya**: `include/mobile/PKModeUI.h`

**UI Layout**:
```
┌─────────────────────────┐
│  PK Mode                │
│                         │
│  ⚪ Peace (안전)         │
│  🔴 Normal (일반)        │
│  ⚡ Free (자유)          │
│  ⚔️  Guild (길드전)      │
│  👥 Party (파티)         │
└─────────────────────────┘
```

### 2.3 Target Info Panel

**Dosya**: `include/mobile/TargetPanel.h`

```cpp
class CTargetPanel : public CUIWindow {
public:
    void SetTarget(CCharacter* target);
    void Update();
    void ShowPvPButton(bool show);

private:
    CCharacter* m_pkTarget;
    CUIButton m_btnPvPRequest;
    CUIProgressBar m_hpBar;
};
```

**UI Layout**:
```
┌────────────────────────┐
│ ⚔️  EnemyWarrior   Lv45 │
│ HP: ████████░░ 80%     │
│ [ Challenge to PvP ]   │
└────────────────────────┘
```

### 📝 Görevler (Faz 2)

- [ ] `PvPDialog` UI bileşeni
- [ ] `PKModeUI` selector
- [ ] `TargetPanel` genişlet
- [ ] Touch gesture: Long-press → PvP request
- [ ] Mobile notifications (toast)
- [ ] Sound effects
- [ ] Test (Android/iOS)

**Tahmini Süre**: 4-5 gün

---

## **FAZ 3: Auto-Targeting** (1 hafta) 🎯 MOBİL KRİTİK

### 3.1 Smart Targeting Algorithm

**Dosya**: `src/game/TargetManager.cpp`

```cpp
class CTargetManager {
public:
    // En yakın düşmanı bul
    CCharacter* FindNearestEnemy(
        CCharacter* me,
        float maxRange = 500.0f,
        bool onlyPvP = true
    );

    // Auto-lock
    void SetAutoTarget(CCharacter* me, CCharacter* target);
    CCharacter* GetAutoTarget(CCharacter* me);

    // Target filtering
    bool IsValidTarget(CCharacter* me, CCharacter* target);

private:
    std::map<DWORD, DWORD> m_mapAutoTarget;  // PID → Target PID
};
```

**Targeting Kriterleri**:
1. En yakın mesafe
2. PvP mode uyumlu
3. Görüş açısı içinde (180°)
4. Engel olmayan (line of sight)
5. Level farkı makul (<10 level)

### 3.2 Mobile Controls Entegrasyonu

**Dosya**: `src/mobile/GameControls.cpp` (güncelle)

```cpp
// Attack button → Auto-target + attack
SetOnAttackCallback([]() {
    auto* me = GetPlayer();
    auto* target = CTargetManager::Instance().GetAutoTarget(me);

    if (!target) {
        // Yeni hedef bul
        target = CTargetManager::Instance().FindNearestEnemy(me);
        if (target) {
            CTargetManager::Instance().SetAutoTarget(me, target);
        }
    }

    if (target && me->CanAttack(target)) {
        me->Attack(target);
    }
});
```

### 3.3 Touch-to-Target

```cpp
// Touch enemy → Select as target
CTouchInput::RegisterTouchCallback([](const TouchPoint& touch) {
    auto* character = FindCharacterAtPosition(touch.x, touch.y);
    if (character && character->IsEnemy()) {
        CTargetManager::Instance().SetAutoTarget(GetPlayer(), character);
        ShowTargetPanel(character);
    }
});
```

### 📝 Görevler (Faz 3)

- [ ] `TargetManager` sınıfı
- [ ] `FindNearestEnemy()` algoritması
- [ ] Auto-lock sistemi
- [ ] Touch-to-target
- [ ] Target highlight (3D/2D)
- [ ] Target switch UI (swipe)
- [ ] Performance optimize (100+ NPC ortamda)

**Tahmini Süre**: 5-7 gün

---

## **FAZ 4: Revenge System** (3-4 gün)

**Referans**: cCorax2 pvp.cpp `Dead()` fonksiyonu

### 4.1 Revenge Mode

**Logic**:
1. Oyuncu A, Oyuncu B'yi öldürür
2. B için "Revenge Mode" aktif olur (5 dakika)
3. B, A'ya ücretsiz saldırabilir (PvP request yok)
4. 5 dakika sonra mod kapanır

**Implementasyon**: `PVPManager.cpp`

```cpp
void CPVPManager::OnDeath(CCharacter* pkDead, CCharacter* pkKiller) {
    // Revenge mode aktif et
    auto* pvpInfo = Find(pkDead->GetPlayerID(), pkKiller->GetPlayerID());
    if (pvpInfo) {
        pvpInfo->bRevenge = true;
        pvpInfo->dwRevengeEndTime = time(nullptr) + 300; // 5 dakika

        // Notify mobile UI
        SendRevengeNotification(pkDead, pkKiller);
    }
}

bool CPVPManager::CanAttack(CCharacter* attacker, CCharacter* victim) {
    auto* pvpInfo = Find(attacker->GetPlayerID(), victim->GetPlayerID());

    // Revenge mode aktifse direkt true
    if (pvpInfo && pvpInfo->bRevenge) {
        DWORD now = time(nullptr);
        if (now < pvpInfo->dwRevengeEndTime) {
            return true;  // İntikam hakkı var!
        }
    }

    // Normal PvP kontrolü...
}
```

### 4.2 Mobile UI

```
┌──────────────────────────┐
│  💀 REVENGE MODE         │
│                          │
│  EnemyWarrior killed you!│
│  Revenge available: 4:32 │
│                          │
│  [ Take Revenge ]        │
└──────────────────────────┘
```

### 📝 Görevler (Faz 4)

- [ ] Revenge logic ekle
- [ ] Timer sistemi
- [ ] Revenge UI notification
- [ ] "Take Revenge" button
- [ ] Revenge statistics (DB)

**Tahmini Süre**: 3-4 gün

---

## **FAZ 5: Duel System** (1-2 hafta)

**Referans**: ZeNu-Elijah "Advanced Duel Options"

### 5.1 Arena System

**Özellikler**:
- Özel duel arenası (teleport)
- 1v1 savaş
- Spectator mode (izleyiciler)
- Bahis sistemi (gold)

**Dosya**: `include/game/DuelManager.h`

```cpp
class CDuelManager {
public:
    // Duel başlat
    bool StartDuel(CCharacter* p1, CCharacter* p2, DWORD betGold);

    // Arena'ya ışınla
    void TeleportToArena(CCharacter* p1, CCharacter* p2);

    // Duel bitir
    void EndDuel(CCharacter* winner, CCharacter* loser);

    // Spectators
    void AddSpectator(CCharacter* spectator, DWORD duelID);

private:
    struct TDuel {
        DWORD dwDuelID;
        DWORD dwPlayer1;
        DWORD dwPlayer2;
        DWORD dwBetGold;
        TPosition arenaPos;
        std::set<DWORD> setSpectators;
    };

    std::map<DWORD, TDuel> m_mapDuels;
};
```

### 5.2 Mobile Duel UI

```
┌────────────────────────┐
│  ⚔️  DUEL INVITATION   │
│                        │
│  EnemyWarrior          │
│  Level: 45             │
│  Bet: 10,000 Yang      │
│                        │
│  [ Accept ] [ Decline ]│
└────────────────────────┘
```

### 📝 Görevler (Faz 5)

- [ ] `DuelManager` sınıfı
- [ ] Arena teleportation
- [ ] Bet/reward system
- [ ] Spectator mode (basic)
- [ ] Duel statistics
- [ ] Leaderboard (duel wins)

**Tahmini Süre**: 7-10 gün

---

## **FAZ 6: Guild War** (2-3 hafta)

### 6.1 Guild Sistemi

**Dosya**: `include/game/Guild.h`

```cpp
class CGuild {
public:
    DWORD GetID() const;
    const std::string& GetName() const;

    // Members
    void AddMember(DWORD playerID, BYTE grade);
    void RemoveMember(DWORD playerID);
    bool IsMember(DWORD playerID);

    // War
    void DeclareWar(CGuild* enemyGuild);
    bool IsAtWar(CGuild* guild);

private:
    DWORD m_dwGuildID;
    std::string m_strName;
    std::set<DWORD> m_setMembers;
    std::set<DWORD> m_setEnemyGuilds;  // War'da olduğu guilds
};
```

### 6.2 Guild War Manager

```cpp
class CGuildWarManager {
public:
    void DeclareWar(DWORD guild1, DWORD guild2);
    void EndWar(DWORD guild1, DWORD guild2);

    bool CanAttack(CCharacter* attacker, CCharacter* victim);
    void OnKill(CCharacter* killer, CCharacter* victim);  // War score

private:
    struct TWarInfo {
        DWORD dwGuild1;
        DWORD dwGuild2;
        DWORD dwScore1;
        DWORD dwScore2;
        DWORD dwStartTime;
    };

    std::map<DWORD, TWarInfo> m_mapWars;
};
```

### 📝 Görevler (Faz 6)

- [ ] `Guild` temel sistemi
- [ ] `GuildWarManager`
- [ ] War declaration UI
- [ ] War score system
- [ ] Guild chat
- [ ] Guild storage (basic)
- [ ] Mobile guild UI

**Tahmini Süre**: 14-21 gün

---

## **FAZ 7: Empire System** (2-3 hafta)

### 7.1 Empire/Kingdom

**3 Krallık**:
1. **Shinsoo** (Kırmızı)
2. **Chunjo** (Sarı)
3. **Jinno** (Mavi)

**Özellikler**:
- Her oyuncu bir krallığa bağlı
- Krallıklar arası otomatik PvP
- Territory control
- Empire buffs

### 📝 Görevler (Faz 7)

- [ ] Empire selection (character creation)
- [ ] Inter-empire PvP (otomatik)
- [ ] Territory/village system
- [ ] Empire ranking
- [ ] Empire quests

**Tahmini Süre**: 14-21 gün

---

## 📊 Genel Zaman Çizelgesi

| Faz | Özellik | Süre | Kümülatif |
|-----|---------|------|-----------|
| 1 | Core PvP Manager | 1-2 hafta | 2 hafta |
| 2 | Mobile PvP UI | 1 hafta | 3 hafta |
| 3 | Auto-Targeting | 1 hafta | 4 hafta |
| 4 | Revenge System | 3-4 gün | ~5 hafta |
| 5 | Duel System | 1-2 hafta | 6-7 hafta |
| 6 | Guild War | 2-3 hafta | 9-10 hafta |
| 7 | Empire System | 2-3 hafta | 11-13 hafta |

**Toplam Tahmini Süre**: **~3 ay** (tam zamanlı geliştirme)

---

## 🎯 Milestone'lar

### Milestone 1: MVP (4 hafta)
- ✅ Core PvP Manager
- ✅ Mobile PvP UI
- ✅ Auto-targeting
- ✅ Revenge mode

**Hedef**: Beta testi için hazır!

### Milestone 2: Enhanced (7 hafta)
- ✅ Milestone 1 özellikleri
- ✅ Duel system
- ✅ Basic statistics

**Hedef**: Soft launch ready

### Milestone 3: Complete (13 hafta)
- ✅ Tüm özellikler
- ✅ Guild war
- ✅ Empire system
- ✅ Advanced features

**Hedef**: Full release!

---

## 🔧 Teknik Gereksinimler

### Geliştirme Ortamı
- **C++ Compiler**: GCC 7+ veya Clang 5+ (C++17 support)
- **Build System**: CMake 3.10+
- **Database**: MySQL 5.7+ / MariaDB 10.2+
- **Lua**: 5.3+
- **Mobile SDK**:
  - Android: NDK r21+, API 21+ (Lollipop)
  - iOS: Xcode 11+, iOS 11.0+

### Performance Hedefleri (Mobile)
- **FPS**: 60 FPS (smooth gameplay)
- **Network latency**: <100ms (UDP preferred)
- **Memory**: <500MB RAM
- **Battery**: <15% per hour (normal gameplay)

### Test Senaryoları
1. **Unit Tests**: Her manager sınıfı için
2. **Integration Tests**: PvP flow end-to-end
3. **Load Tests**: 100+ concurrent PvP
4. **Mobile Tests**: Android (5 devices) + iOS (3 devices)
5. **Network Tests**: 3G/4G/WiFi conditions

---

## 📚 Dokümantasyon

### Yazılacak Dokümanlar
- [ ] PVPManager API Reference
- [ ] Mobile PvP UX Guidelines
- [ ] Server Setup Guide
- [ ] Mobile Integration Guide (Android/iOS)
- [ ] Testing Handbook

---

## 🚀 İlk Adım: Faz 1 Başlat!

### Hemen Yapılacaklar

1. **PVPManager.h oluştur**
   ```bash
   touch include/game/PVPManager.h
   touch src/game/PVPManager.cpp
   ```

2. **CMakeLists.txt güncelle**
   ```cmake
   set(GAME_SOURCES
       ...
       src/game/PVPManager.cpp
   )
   ```

3. **Skeleton kod yaz**
4. **İlk test: PvP request/accept flow**

---

**Proje Başlangıç Tarihi**: 2025-11-04
**Hedef Beta Tarihi**: 2025-12-02 (4 hafta sonra)
**Hedef Release Tarihi**: 2026-02-04 (~3 ay sonra)

**Durum**: 📋 Plan hazır - Geliştirme başlayabilir! 🚀
