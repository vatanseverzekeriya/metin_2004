# 📋 Metin2 Mobile PvP - Tüm Fazlar ve Detaylar

## Genel Bakış

**Toplam Süre**: ~3 ay (13 hafta)
**Toplam Faz**: 7 faz
**Durum**: Faz 1 ✅ Tamamlandı

---

## 🔥 **FAZ 1: Core PvP Manager** ✅ TAMAMLANDI
**Süre**: 1-2 hafta
**Durum**: ✅ Tamamlandı (2025-11-04)

### Yapılanlar:
- [x] **PVPManager Sınıfı** (240+ satır)
  - CPVP: İki oyuncu arası PvP ilişkisi
  - CPVPManager: Global PvP yöneticisi (singleton)
  - CRC hash sistemi (hızlı arama)
  - Thread-safe tasarım (std::mutex)

- [x] **PvP Logic** (380+ satır)
  - Request/Accept sistemi
  - Fight durumu yönetimi
  - Revenge mode (5 dakika)
  - Auto cleanup (10 dakika timeout)
  - Mobile callbacks

- [x] **Character Entegrasyonu**
  - SendPvPRequest()
  - AcceptPvPRequest()
  - DeclinePvPRequest()
  - CanAttack() → PVPManager kontrolü
  - OnDeath() → Revenge tetikleme

- [x] **PvP Durumları**
  - PVP_STATE_NONE (PvP yok)
  - PVP_STATE_WAIT (Bir taraf kabul etti)
  - PVP_STATE_FIGHT (Aktif savaş)
  - PVP_STATE_REVENGE (İntikam modu)

- [x] **Test Suite** (6 test senaryosu)
  - Basic request/accept
  - Revenge mode
  - Timeout mechanism
  - Multiple PvPs
  - PvP removal
  - Mobile callbacks

- [x] **Dokümantasyon**
  - OPEN_SOURCE_REFERENCES.md
  - MOBILE_PVP_ROADMAP.md
  - PVP_IMPLEMENTATION_SUMMARY.md

### Çıktılar:
```
include/game/PVPManager.h        (240 satır)
src/game/PVPManager.cpp          (380 satır)
examples/pvp_test.cpp            (250 satır)
Güncellemeler: Character.h/cpp, CMakeLists.txt, Makefile
```

**Açık Kaynak Referans**: cCorax2/Source_code

---

## 📱 **FAZ 2: Mobile PvP UI**
**Süre**: 1 hafta
**Durum**: ⏳ Beklemede

### Yapılacaklar:

#### 1. PvP Request Dialog
- [ ] **CPvPDialog Sınıfı** (`mobile/PvPDialog.h`)
  ```cpp
  class CPvPDialog : public CUIWindow {
      void Show(string requesterName);
      void SetOnAccept(callback);
      void SetOnDecline(callback);
  };
  ```

- [ ] **UI Layout**:
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

#### 2. PK Mode Selector
- [ ] **EPKMode Enum Genişletmesi**
  ```cpp
  enum EPKMode {
      PK_MODE_PEACE = 0,    // ⚪ Barış (saldırı yok)
      PK_MODE_NORMAL = 1,   // 🔴 Normal (PvP request ile)
      PK_MODE_FREE = 2,     // ⚡ Serbest (herkese saldırı)
      PK_MODE_GUILD = 3,    // ⚔️  Guild (guild dışına)
      PK_MODE_PARTY = 4     // 👥 Party (parti dışına)
  };
  ```

- [ ] **CPKModeUI Sınıfı** (`mobile/PKModeUI.h`)
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

#### 3. Target Info Panel
- [ ] **CTargetPanel Sınıfı** (`mobile/TargetPanel.h`)
  - Target character bilgileri
  - HP bar (progress bar)
  - Level ve isim
  - "Challenge to PvP" button

- [ ] **UI Layout**:
  ```
  ┌────────────────────────┐
  │ ⚔️  EnemyWarrior   Lv45 │
  │ HP: ████████░░ 80%     │
  │ [ Challenge to PvP ]   │
  └────────────────────────┘
  ```

#### 4. Revenge Notification
- [ ] **Revenge UI** (`mobile/RevengeNotification.h`)
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

- [ ] **Countdown Timer**
  - 5 dakika geri sayım
  - Süre dolunca buton disable

#### 5. Touch Gestures
- [ ] **Long-press → PvP Request**
  ```cpp
  CTouchInput::RegisterGestureCallback([](GestureInfo& g) {
      if (g.type == GESTURE_LONG_PRESS) {
          auto* target = FindCharacterAtPosition(g.x, g.y);
          if (target) SendPvPRequest(target);
      }
  });
  ```

- [ ] **Swipe → Decline Request**
- [ ] **Tap → Accept Request**

#### 6. Mobile Notifications
- [ ] Toast notifications
- [ ] Sound effects
- [ ] Vibration feedback (mobile)

### Çıktılar:
```
include/mobile/PvPDialog.h
include/mobile/PKModeUI.h
include/mobile/TargetPanel.h
include/mobile/RevengeNotification.h
src/mobile/[corresponding .cpp files]
```

**Tahmini Süre**: 4-5 gün

---

## 🎯 **FAZ 3: Auto-Targeting**
**Süre**: 1 hafta
**Durum**: ⏳ Beklemede
**Öncelik**: 🔥 MOBİL İÇİN KRİTİK!

### Yapılacaklar:

#### 1. Smart Targeting Algorithm
- [ ] **CTargetManager Sınıfı** (`game/TargetManager.h`)
  ```cpp
  class CTargetManager {
      // En yakın düşmanı bul
      CCharacter* FindNearestEnemy(
          CCharacter* me,
          float maxRange = 500.0f,
          bool onlyPvP = true
      );

      // Auto-lock sistemi
      void SetAutoTarget(CCharacter* me, CCharacter* target);
      CCharacter* GetAutoTarget(CCharacter* me);

      // Target validation
      bool IsValidTarget(CCharacter* me, CCharacter* target);
  };
  ```

#### 2. Targeting Kriterleri
- [ ] **Mesafe Bazlı**
  - En yakın düşman öncelikli
  - Maksimum range kontrolü

- [ ] **PvP Mode Uyumlu**
  - PvP mode'a göre filtreleme
  - Guild/Party kontrolü

- [ ] **Görüş Açısı** (FOV)
  - 180° görüş açısı içinde
  - Arkadaki düşmanlar hedef alınmaz

- [ ] **Line of Sight**
  - Engel olmayan hedefler
  - Duvar/obje kontrolü

- [ ] **Level Farkı**
  - Makul level farkı (<10 level)
  - Çok güçlü/zayıf hedefleri göz ardı et

#### 3. Mobile Controls Entegrasyonu
- [ ] **Attack Button → Auto-target**
  ```cpp
  CGameControls::SetOnAttackCallback([]() {
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

#### 4. Touch-to-Target
- [ ] **Touch enemy → Select as target**
  ```cpp
  CTouchInput::RegisterTouchCallback([](TouchPoint& touch) {
      auto* character = FindCharacterAtPosition(touch.x, touch.y);
      if (character && character->IsEnemy()) {
          CTargetManager::Instance().SetAutoTarget(GetPlayer(), character);
          ShowTargetPanel(character);
      }
  });
  ```

#### 5. Target UI
- [ ] **Target Highlight** (3D/2D)
  - Kırmızı outline/circle
  - Target indicator arrow

- [ ] **Target Lock Icon**
  - Lock simgesi
  - Target ismi üzerinde

- [ ] **Target Switch** (Swipe)
  - Sağa swipe → Next target
  - Sola swipe → Previous target

#### 6. Performance Optimization
- [ ] **Spatial Indexing**
  - QuadTree veya Grid system
  - 100+ NPC ortamında optimize

- [ ] **Update Frequency**
  - Target arama: 100ms interval
  - Görsel güncelleme: 60 FPS

### Çıktılar:
```
include/game/TargetManager.h
src/game/TargetManager.cpp
Güncellemeler: GameControls.cpp, TouchInput.cpp
```

**Tahmini Süre**: 5-7 gün

---

## 💀 **FAZ 4: Revenge System**
**Süre**: 3-4 gün
**Durum**: ⏳ Beklemede

### Yapılacaklar:

#### 1. Revenge Mode Logic (Zaten var, genişletme)
- [ ] **Revenge Duration Control**
  - 5 dakika timer (zaten var)
  - UI countdown gösterimi
  - Süre dolunca otomatik kapat

- [ ] **Revenge Privileges**
  ```cpp
  bool CPVPManager::CanAttack(...) {
      // Revenge mode'daysa ücretsiz saldırı
      if (pvp->IsRevenge() && pvp->CanRevenge(attacker_pid)) {
          return true;  // No PvP request needed!
      }
  }
  ```

#### 2. Mobile UI Integration
- [ ] **Revenge Notification**
  - Ölüm anında notification
  - "Take Revenge" büyük buton
  - Countdown timer

- [ ] **Revenge Indicator**
  - Killer'ın üzerinde özel icon
  - Kırmızı skull icon
  - "Revenge available" text

- [ ] **Quick Revenge Button**
  - Ana UI'da sabit revenge button
  - Killer'a otomatik yönlendirme
  - Teleport option (optional)

#### 3. Revenge Statistics
- [ ] **Database Schema**
  ```sql
  ALTER TABLE player ADD COLUMN revenge_kills INT DEFAULT 0;
  ALTER TABLE player ADD COLUMN revenge_deaths INT DEFAULT 0;
  ```

- [ ] **Revenge Achievements**
  - "Avenger" (10 revenge kills)
  - "Vengeful" (25 revenge kills)
  - "Wrath" (50 revenge kills)

#### 4. Revenge Rewards
- [ ] **Bonus Rewards**
  - Revenge kill: 2x EXP
  - Revenge kill: 2x Gold
  - Special achievement points

- [ ] **Revenge Chain**
  - Consecutive revenge kills
  - Combo multiplier
  - "Revenge Streak" badge

### Çıktılar:
```
Güncellemeler: PVPManager.cpp (genişletme)
mobile/RevengeNotification.cpp
sql/revenge_schema.sql
```

**Tahmini Süre**: 3-4 gün

---

## ⚔️ **FAZ 5: Duel System**
**Süre**: 1-2 hafta
**Durum**: ⏳ Beklemede
**Referans**: ZeNu-Elijah/Metin2 "Advanced Duel Options"

### Yapılacaklar:

#### 1. Duel Manager
- [ ] **CDuelManager Sınıfı** (`game/DuelManager.h`)
  ```cpp
  class CDuelManager {
      // Duel başlat
      bool StartDuel(CCharacter* p1, CCharacter* p2, DWORD betGold);

      // Arena'ya ışınla
      void TeleportToArena(CCharacter* p1, CCharacter* p2);

      // Duel bitir
      void EndDuel(CCharacter* winner, CCharacter* loser);

      // Spectators
      void AddSpectator(CCharacter* spectator, DWORD duelID);
      void RemoveSpectator(CCharacter* spectator);
  };
  ```

#### 2. Arena System
- [ ] **Arena Map**
  - Özel duel arenası (küçük alan)
  - 1v1 savaş için optimize
  - Duvarlarla çevrili

- [ ] **Teleportation**
  - Duel başladığında arena'ya ışınla
  - Duel bitince eski pozisyona dön
  - Spectator alanı (tribün)

- [ ] **Arena Rules**
  - Sadece duel yapan oyuncular saldırabilir
  - Spectator'lar saldıramaz
  - Arena dışına çıkılamaz

#### 3. Betting System
- [ ] **Bahis Sistemi**
  ```cpp
  struct TDuel {
      DWORD dwPlayer1;
      DWORD dwPlayer2;
      DWORD dwBetGold;      // Bahis miktarı
      DWORD dwWinner;       // Kazanan
      time_t tStartTime;
  };
  ```

- [ ] **Bet Rules**
  - Her iki oyuncu aynı miktarı koyar
  - Kazanan tüm parayı alır
  - Draw durumunda para iade

- [ ] **Mobile UI**
  ```
  ┌────────────────────────┐
  │  ⚔️  DUEL INVITATION   │
  │                        │
  │  EnemyWarrior          │
  │  Level: 45             │
  │  Bet: 10,000 Yang      │
  │                        │
  │  [Bet: ________ ]      │
  │  [ Accept ] [ Decline ]│
  └────────────────────────┘
  ```

#### 4. Spectator Mode
- [ ] **İzleme Sistemi**
  - Spectator listesi
  - Arena tribününe ışınlanma
  - Sadece izleme (saldırı yok)

- [ ] **Spectator UI**
  - Player 1 vs Player 2 info
  - HP bars (her ikisi için)
  - Round timer
  - Bet info (optional)

- [ ] **Camera System**
  - Arena'yı izleyebilme
  - Free camera rotation
  - Player follow mode

#### 5. Duel Statistics
- [ ] **Database Schema**
  ```sql
  CREATE TABLE duel_history (
      id INT AUTO_INCREMENT PRIMARY KEY,
      player1_id INT,
      player2_id INT,
      winner_id INT,
      bet_amount INT,
      duration INT,
      created_at TIMESTAMP
  );
  ```

- [ ] **Duel Leaderboard**
  - Most duel wins
  - Highest win streak
  - Total gold won

- [ ] **Achievements**
  - "Duelist" (10 wins)
  - "Champion" (50 wins)
  - "Legend" (100 wins)

#### 6. Advanced Features
- [ ] **Round System** (Optional)
  - Best of 3 rounds
  - HP reset between rounds
  - Final winner determination

- [ ] **Duel Invitation Cooldown**
  - Same player: 5 min cooldown
  - Prevent spam

- [ ] **Duel Rewards**
  - Winner: Gold + EXP
  - Loser: Participation EXP
  - Spectators: Small reward

### Çıktılar:
```
include/game/DuelManager.h
src/game/DuelManager.cpp
mobile/DuelInvitationUI.h/cpp
mobile/SpectatorUI.h/cpp
sql/duel_schema.sql
```

**Tahmini Süre**: 7-10 gün

---

## 🏰 **FAZ 6: Guild War**
**Süre**: 2-3 hafta
**Durum**: ⏳ Beklemede

### Yapılacaklar:

#### 1. Guild Sistemi (Temel)
- [ ] **CGuild Sınıfı** (`game/Guild.h`)
  ```cpp
  class CGuild {
      DWORD GetID() const;
      const string& GetName() const;

      // Members
      void AddMember(DWORD playerID, BYTE grade);
      void RemoveMember(DWORD playerID);
      bool IsMember(DWORD playerID);

      // Grades
      enum EGuildGrade {
          GRADE_MASTER = 0,     // Guild Master
          GRADE_OFFICER = 1,    // Officer
          GRADE_MEMBER = 2      // Normal member
      };

      // War
      void DeclareWar(CGuild* enemyGuild);
      bool IsAtWar(CGuild* guild);
  };
  ```

#### 2. Guild War Manager
- [ ] **CGuildWarManager Sınıfı** (`game/GuildWarManager.h`)
  ```cpp
  class CGuildWarManager {
      // War declaration
      void DeclareWar(DWORD guild1, DWORD guild2);
      void AcceptWar(DWORD guild1, DWORD guild2);
      void EndWar(DWORD guild1, DWORD guild2);

      // Combat
      bool CanAttack(CCharacter* attacker, CCharacter* victim);
      void OnKill(CCharacter* killer, CCharacter* victim);

      // War status
      bool IsAtWar(DWORD guild1, DWORD guild2);
      DWORD GetWarScore(DWORD guildID);
  };
  ```

- [ ] **War Info Structure**
  ```cpp
  struct TWarInfo {
      DWORD dwGuild1;
      DWORD dwGuild2;
      DWORD dwScore1;        // Guild 1 kills
      DWORD dwScore2;        // Guild 2 kills
      DWORD dwStartTime;
      DWORD dwDuration;      // War duration (e.g., 1 hour)
      EWarState eState;      // REQUESTED, ACTIVE, ENDED
  };
  ```

#### 3. Guild Creation & Management
- [ ] **Create Guild**
  - Minimum level requirement (e.g., 40)
  - Creation cost (e.g., 1,000,000 yang)
  - Unique guild name

- [ ] **Invite Members**
  - Guild master can invite
  - Accept/Decline invitation
  - Maximum members (e.g., 50)

- [ ] **Guild Ranks**
  - Master: Full control
  - Officer: Can invite, kick members
  - Member: Normal privileges

#### 4. Guild War Mechanics
- [ ] **War Declaration**
  - Guild master declares war
  - Target guild must accept
  - Both guilds notified

- [ ] **War Rules**
  - Duration: 1-2 hours
  - Kill count scoring
  - No level restrictions
  - No death penalty (in war)

- [ ] **Auto-Attack in War**
  ```cpp
  bool CanAttack(attacker, victim) {
      auto* attackerGuild = attacker->GetGuild();
      auto* victimGuild = victim->GetGuild();

      if (attackerGuild && victimGuild) {
          if (attackerGuild->IsAtWar(victimGuild)) {
              return true;  // Free PvP during guild war!
          }
      }
  }
  ```

- [ ] **Score System**
  - 1 kill = 1 point
  - First to 50 kills wins (or)
  - Most kills after duration

#### 5. Guild Database
- [ ] **Database Schema**
  ```sql
  CREATE TABLE guild (
      id INT AUTO_INCREMENT PRIMARY KEY,
      name VARCHAR(50) UNIQUE NOT NULL,
      master_id INT,
      level INT DEFAULT 1,
      exp INT DEFAULT 0,
      gold INT DEFAULT 0,
      created_at TIMESTAMP
  );

  CREATE TABLE guild_member (
      guild_id INT,
      player_id INT,
      grade TINYINT,  -- 0=Master, 1=Officer, 2=Member
      joined_at TIMESTAMP,
      PRIMARY KEY (guild_id, player_id)
  );

  CREATE TABLE guild_war (
      id INT AUTO_INCREMENT PRIMARY KEY,
      guild1_id INT,
      guild2_id INT,
      score1 INT DEFAULT 0,
      score2 INT DEFAULT 0,
      winner_id INT,
      started_at TIMESTAMP,
      ended_at TIMESTAMP
  );
  ```

#### 6. Guild Mobile UI
- [ ] **Guild Window**
  ```
  ┌──────────────────────────┐
  │  Guild: DragonSlayers    │
  │  Level: 5   Members: 23  │
  │                          │
  │  [Members]  [War]  [Info]│
  │                          │
  │  Master: WarriorKing     │
  │  Officer: MageQueen      │
  │  Member: AssassinPro     │
  │  ...                     │
  │                          │
  │  [ Invite ] [ Leave ]    │
  └──────────────────────────┘
  ```

- [ ] **War Declaration UI**
  ```
  ┌──────────────────────────┐
  │  Declare Guild War       │
  │                          │
  │  Target: EvilEmpire      │
  │  Duration: 2 hours       │
  │  Win Condition: 50 kills │
  │                          │
  │  [ Declare War ]         │
  └──────────────────────────┘
  ```

- [ ] **War Status UI**
  ```
  ┌──────────────────────────┐
  │  ⚔️  GUILD WAR            │
  │                          │
  │  DragonSlayers  vs       │
  │      [32]                │
  │                          │
  │      [28]                │
  │  EvilEmpire              │
  │                          │
  │  Time Left: 1:23:45      │
  └──────────────────────────┘
  ```

#### 7. Guild Features
- [ ] **Guild Chat**
  - Separate chat channel
  - Guild-only messages
  - Officer announcements

- [ ] **Guild Storage** (Basic)
  - Shared item storage
  - Deposit/Withdraw items
  - Master/Officer only access

- [ ] **Guild Skills** (Optional)
  - Guild buffs (HP+, ATK+)
  - Requires guild level
  - Costs guild gold

#### 8. War Rewards
- [ ] **Winner Rewards**
  - All members get rewards
  - Gold + EXP bonus
  - Guild exp increase

- [ ] **Individual Rewards**
  - Top killer: Extra rewards
  - Participation rewards
  - War achievement points

### Çıktılar:
```
include/game/Guild.h
include/game/GuildWarManager.h
src/game/Guild.cpp
src/game/GuildWarManager.cpp
mobile/GuildUI.h/cpp
mobile/GuildWarUI.h/cpp
sql/guild_schema.sql
```

**Tahmini Süre**: 14-21 gün

---

## 🏛️ **FAZ 7: Empire System**
**Süre**: 2-3 hafta
**Durum**: ⏳ Beklemede

### Yapılacaklar:

#### 1. Empire Structure
- [ ] **3 Krallık Sistemi**
  ```cpp
  enum EEmpire {
      EMPIRE_NONE = 0,
      EMPIRE_SHINSOO = 1,    // 🔴 Kırmızı
      EMPIRE_CHUNJO = 2,     // 🟡 Sarı
      EMPIRE_JINNO = 3       // 🔵 Mavi
  };
  ```

- [ ] **CEmpire Sınıfı** (`game/Empire.h`)
  ```cpp
  class CEmpire {
      EEmpire GetID() const;
      const string& GetName() const;
      DWORD GetColor() const;

      // Members
      vector<DWORD> GetMembers();
      bool IsMember(DWORD playerID);

      // Territory
      vector<DWORD> GetTerritories();
      bool OwnsTerritory(DWORD mapID);

      // Stats
      DWORD GetTotalKills();
      DWORD GetTotalDeaths();
      float GetKDRatio();
  };
  ```

#### 2. Empire Selection
- [ ] **Character Creation**
  - Empire seçimi (zorunlu)
  - Her empire'ın açıklaması
  - Empire colors/icons

- [ ] **Mobile UI**
  ```
  ┌──────────────────────────┐
  │  Choose Your Empire      │
  │                          │
  │  🔴 Shinsoo Empire       │
  │     Warrior kingdom      │
  │     Strength & Honor     │
  │                          │
  │  🟡 Chunjo Empire        │
  │     Magic kingdom        │
  │     Wisdom & Power       │
  │                          │
  │  🔵 Jinno Empire         │
  │     Rogue kingdom        │
  │     Speed & Cunning      │
  │                          │
  │  [ SELECT ]              │
  └──────────────────────────┘
  ```

- [ ] **Empire Change** (Optional)
  - Yüksek maliyet (e.g., 10M yang)
  - Level requirement (e.g., 75+)
  - Cooldown (e.g., 30 days)

#### 3. Inter-Empire PvP
- [ ] **Auto-PvP Between Empires**
  ```cpp
  bool CanAttack(attacker, victim) {
      if (attacker->GetEmpire() != victim->GetEmpire()) {
          // Farklı empire → otomatik düşman!
          return true;
      }
  }
  ```

- [ ] **Empire War Zones**
  - Özel haritalar (savaş bölgeleri)
  - Sürekli PvP aktif
  - Bonus rewards

- [ ] **Safe Zones**
  - Kendi empire'ın şehirleri
  - PvP yasak
  - Diğer empire'lar giremez

#### 4. Territory System
- [ ] **Territory Control**
  - Haritalar/bölgeler empire'lara ait
  - Ele geçirilebilir bölgeler
  - Territory bonusları

- [ ] **Capture Mechanics**
  - Flag/Crystal sistemi
  - Takım savaşı (20v20)
  - Capture time (e.g., 10 dakika)

- [ ] **Territory Benefits**
  - Sahip empire: EXP +10%
  - Sahip empire: Gold drop +20%
  - Tax system (Optional)

#### 5. Empire Ranking
- [ ] **Global Ranking**
  ```sql
  CREATE TABLE empire_stats (
      empire_id TINYINT PRIMARY KEY,
      total_kills INT DEFAULT 0,
      total_deaths INT DEFAULT 0,
      territories_owned INT DEFAULT 0,
      total_members INT DEFAULT 0,
      ranking INT DEFAULT 0,
      updated_at TIMESTAMP
  );
  ```

- [ ] **Weekly Competition**
  - Empire kills leaderboard
  - Territory control ranking
  - Rewards for #1 empire

- [ ] **Empire Buffs**
  - Winning empire: All members +5% stats
  - Buff duration: 1 week
  - Reset every Sunday

#### 6. Empire Quests
- [ ] **Daily Empire Missions**
  - "Kill 10 enemy empire players"
  - "Capture 1 territory"
  - "Defend homeland"

- [ ] **Empire Quest Rewards**
  - Empire contribution points
  - Special items (empire-themed)
  - Gold + EXP

- [ ] **Contribution Shop**
  - Special items (empire gear)
  - Cosmetics (empire colors)
  - Mounts (empire-themed)

#### 7. Empire Mobile UI
- [ ] **Empire Info Window**
  ```
  ┌──────────────────────────┐
  │  🔴 Shinsoo Empire       │
  │                          │
  │  Rank: #1                │
  │  Members: 1,234          │
  │  Territories: 12/20      │
  │                          │
  │  K/D Ratio: 1.45         │
  │  Total Kills: 45,678     │
  │                          │
  │  Buff: +5% All Stats ⬆️  │
  │  Duration: 3d 12h        │
  │                          │
  │  [ Quests ] [ Ranking ]  │
  └──────────────────────────┘
  ```

- [ ] **Territory Map**
  - Harita üzerinde bölgeler
  - Empire renkleri ile kodlanmış
  - Contested zones (savaş bölgeleri)
  - Tap to see details

- [ ] **Empire Chat**
  - Empire-wide chat channel
  - Strategy coordination
  - War announcements

#### 8. Siege Warfare (Advanced)
- [ ] **Castle Siege**
  - Büyük kale savaşları
  - 50v50 battles
  - Siege weapons (catapults, etc.)

- [ ] **Siege Schedule**
  - Weekly siege events
  - Pre-registration
  - Time zone friendly

- [ ] **Siege Rewards**
  - Winning empire: Castle ownership
  - Castle benefits (tax, buffs)
  - Individual rewards (top players)

#### 9. Empire Achievements
- [ ] **Personal Achievements**
  - "Empire Defender" (100 kills)
  - "Territory Conqueror" (10 captures)
  - "Empire Hero" (1000 contribution)

- [ ] **Empire Titles**
  - "Shinsoo Champion"
  - "Chunjo Archmage"
  - "Jinno Shadow"

### Çıktılar:
```
include/game/Empire.h
include/game/TerritoryManager.h
include/game/SiegeManager.h
src/game/Empire.cpp
src/game/TerritoryManager.cpp
src/game/SiegeManager.cpp
mobile/EmpireUI.h/cpp
mobile/TerritoryMapUI.h/cpp
sql/empire_schema.sql
```

**Tahmini Süre**: 14-21 gün

---

## 📊 Genel Zaman Çizelgesi

| Faz | Özellik | Süre | Kümülatif | Durum |
|-----|---------|------|-----------|--------|
| 1 | Core PvP Manager | 1-2 hafta | 2 hafta | ✅ |
| 2 | Mobile PvP UI | 1 hafta | 3 hafta | ⏳ |
| 3 | Auto-Targeting | 1 hafta | 4 hafta | ⏳ |
| 4 | Revenge System | 3-4 gün | ~5 hafta | ⏳ |
| 5 | Duel System | 1-2 hafta | 6-7 hafta | ⏳ |
| 6 | Guild War | 2-3 hafta | 9-10 hafta | ⏳ |
| 7 | Empire System | 2-3 hafta | 11-13 hafta | ⏳ |

**Toplam**: ~3 ay (13 hafta)

---

## 🎯 Milestone'lar

### 🏁 Milestone 1: MVP (4 hafta)
**Hedef**: Beta test için hazır!

**İçerik**:
- ✅ Faz 1: Core PvP Manager
- ⏳ Faz 2: Mobile PvP UI
- ⏳ Faz 3: Auto-targeting
- ⏳ Faz 4: Revenge mode

**Özellikler**:
- PvP request/accept sistemi
- Mobile touch controls
- Auto-targeting düşmanlar
- Revenge mode (5 dakika)
- Basit UI (dialog, buttons)

**Test Kriterleri**:
- [ ] 10 oyuncu concurrent PvP
- [ ] Mobile UI responsive
- [ ] No crashes
- [ ] Revenge mode çalışıyor

---

### 🏁 Milestone 2: Enhanced (7 hafta)
**Hedef**: Soft launch ready!

**İçerik**:
- Milestone 1 özellikleri
- ⏳ Faz 5: Duel system

**Özellikler**:
- Duel invitations
- Arena teleportation
- Betting system
- Spectator mode
- Duel statistics

**Test Kriterleri**:
- [ ] 50 oyuncu concurrent
- [ ] Duel system stable
- [ ] Betting works correctly
- [ ] Spectator mode smooth

---

### 🏁 Milestone 3: Complete (13 hafta)
**Hedef**: Full release!

**İçerik**:
- Milestone 1 + 2 özellikleri
- ⏳ Faz 6: Guild War
- ⏳ Faz 7: Empire System

**Özellikler**:
- Guild creation
- Guild war system
- Empire selection
- Territory control
- Siege warfare
- Full ranking system

**Test Kriterleri**:
- [ ] 200+ oyuncu concurrent
- [ ] Guild war 50v50 stable
- [ ] Empire system balanced
- [ ] All features working
- [ ] Production ready

---

## 📈 İlerleme Gösterimi

```
Genel İlerleme:
════════════════════════════════════════════════════════════
Faz 1: ████████████████████████████████████████████████ 100% ✅
Faz 2: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% ⏳
Faz 3: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% ⏳
Faz 4: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% ⏳
Faz 5: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% ⏳
Faz 6: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% ⏳
Faz 7: ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% ⏳
════════════════════════════════════════════════════════════
TOPLAM:  ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  14%
════════════════════════════════════════════════════════════
```

---

## 🎯 Sonraki Adımlar

### Hemen Yapılabilir (Faz 2 başlangıç):

1. **PvPDialog UI Component**
   ```bash
   touch include/mobile/PvPDialog.h
   touch src/mobile/PvPDialog.cpp
   ```

2. **PK Mode Selector**
   ```bash
   touch include/mobile/PKModeUI.h
   touch src/mobile/PKModeUI.cpp
   ```

3. **Target Panel Genişletme**
   - Mevcut TargetPanel'e PvP butonu ekle
   - HP bar ekle
   - Level/name gösterimi

### Test Stratejisi:

Her faz için:
- [ ] Unit tests
- [ ] Integration tests
- [ ] Mobile UI tests (Android + iOS)
- [ ] Performance tests
- [ ] User acceptance tests

---

**Son Güncelleme**: 2025-11-04
**Aktif Faz**: 1/7 ✅ (Faz 2 başlayabilir)
**Toplam İlerleme**: 14%
**Hedef Beta**: 2025-12-02 (4 hafta)
**Hedef Release**: 2026-02-04 (13 hafta)

🚀 **Ready to start Phase 2!**
