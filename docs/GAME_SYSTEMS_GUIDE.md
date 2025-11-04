# Metin2 PvP Server - Oyun Sistemleri Rehberi

## İçindekiler
1. [Hareket Sistemi](#hareket-sistemi)
2. [Combat ve Skill Sistemi](#combat-ve-skill-sistemi)
3. [Can/Mana Yönetimi](#canmana-yönetimi)
4. [Buff/Debuff Sistemi](#buffdebuff-sistemi)
5. [Lua Event Sistemi](#lua-event-sistemi)
6. [Kullanım Örnekleri](#kullanım-örnekleri)

---

## Hareket Sistemi

### Temel Hareket

```cpp
#include "include/game/Movement.h"
#include "include/game/Character.h"

CCharacter player;
TPosition target(500, 500);

// Koşarak hareket
player.StartMove(target, MOVE_TYPE_RUN);

// Game loop'ta güncelle
while (game_running) {
    float delta_time = 0.016f;  // 60 FPS
    player.Update(delta_time);
}
```

### Hareket Türleri
- `MOVE_TYPE_WALK` - Yavaş yürüme (50 birim/sn)
- `MOVE_TYPE_RUN` - Normal koşma (150 birim/sn)
- `MOVE_TYPE_DASH` - Hızlı sprint (300 birim/sn)

### Path-Finding (Çoklu Waypoint)

```cpp
std::vector<TPosition> path;
path.push_back(TPosition(100, 100));
path.push_back(TPosition(200, 150));
path.push_back(TPosition(300, 200));

CMovement* movement = player.GetMovement();
movement->StartPathMove(player.GetPosition(), path, MOVE_TYPE_RUN);
```

---

## Combat ve Skill Sistemi

### Normal Saldırı

```cpp
#include "include/game/Combat.h"

CCombatSystem& combat = CCombatSystem::Instance();

// Hasar hesapla
TDamageInfo dmg = combat.CalculateDamage(attacker, victim);

// Hasarı uygula
combat.ApplyDamage(dmg);

// Kritik mi?
if (dmg.is_critical) {
    std::cout << "KRİTİK VURUŞ! x" << combat.GetCriticalMultiplier() << std::endl;
}
```

### Skill Tanımlama

```cpp
TSkillProto fire_ball;
fire_ball.vnum = 100;
fire_ball.name = "Ateş Topu";
fire_ball.type = SKILL_TYPE_ATTACK;
fire_ball.target_type = SKILL_TARGET_ENEMY;
fire_ball.damage_base = 300;
fire_ball.damage_multiplier = 1.5f;
fire_ball.sp_cost = 50;
fire_ball.cooldown_ms = 3000;  // 3 saniye
fire_ball.range = 500;

combat.RegisterSkill(fire_ball);
```

### Skill Kullanımı

```cpp
// Skill kullan
if (combat.CanUseSkill(player, 100, target)) {
    combat.UseSkill(player, 100, target);
}

// Cooldown kontrolü
DWORD remaining = combat.GetRemainingCooldown(player, 100);
if (remaining > 0) {
    std::cout << "Cooldown: " << remaining / 1000.0f << "s" << std::endl;
}
```

---

## Can/Mana Yönetimi

### HP/SP İşlemleri

```cpp
// HP azalt/artır
player.DecreaseHP(500);
player.IncreaseHP(200);
player.SetHP(3000);

// SP azalt/artır
player.DecreaseSP(100);
player.IncreaseSP(50);
player.SetSP(250);

// Mevcut değerler
DWORD hp = player.GetHP();
DWORD max_hp = player.GetMaxHP();
DWORD sp = player.GetSP();
DWORD max_sp = player.GetMaxSP();
```

### Otomatik Rejenerasyon

```cpp
CAffectManager* affect_mgr = player.GetAffectManager();

// Rejenerasyon hızını ayarla
affect_mgr->SetRegenRate(20, 10);  // 20 HP/sn, 10 SP/sn

// Her frame güncelle
affect_mgr->UpdateRegeneration(delta_time);
```

---

## Buff/Debuff Sistemi

### Affect Türleri

| Affect | ID | Açıklama |
|--------|-----|----------|
| HP_REGEN | 1 | HP rejenerasyonu bonusu |
| SP_REGEN | 2 | SP rejenerasyonu bonusu |
| ATTACK_BOOST | 3 | Saldırı artışı |
| DEFENSE_BOOST | 4 | Savunma artışı |
| SPEED_BOOST | 5 | Hız artışı |
| POISON | 10 | Zehir (DoT) |
| SLOW | 11 | Yavaşlama |
| STUN | 12 | Sersemletme |
| SILENCE | 13 | Sessizlik |

### Buff Ekleme/Kaldırma

```cpp
CAffectManager* affect_mgr = player.GetAffectManager();

// Attack buff ekle (+100 attack, 30 saniye)
affect_mgr->AddAffect(AFFECT_ATTACK_BOOST, 100, 30000);

// Speed buff ekle (+50% hız, 20 saniye)
affect_mgr->AddAffect(AFFECT_SPEED_BOOST, 50, 20000);

// Zehir debuff ekle (10 hasar/sn, 5 saniye)
affect_mgr->AddAffect(AFFECT_POISON, 10, 5000);

// Buff kaldır
affect_mgr->RemoveAffect(AFFECT_ATTACK_BOOST);

// Tüm buffları kaldır
affect_mgr->RemoveAllAffects();
```

### Affect Kontrolü

```cpp
// Buff var mı?
bool has_speed = affect_mgr->HasAffect(AFFECT_SPEED_BOOST);

// Stun durumunda mı?
if (affect_mgr->IsStunned()) {
    std::cout << "Hareket edilemez!" << std::endl;
}

// Aktif buffları listele
const auto& affects = affect_mgr->GetAllAffects();
for (const auto& affect : affects) {
    std::cout << "Type: " << (int)affect.type
              << ", Kalan: " << affect.GetRemainingTime() / 1000.0f << "s" << std::endl;
}
```

---

## Lua Event Sistemi

### Lua Başlatma

```cpp
#include "include/script/LuaBinding.h"

CLuaBinding& lua = CLuaBinding::Instance();
lua.Initialize();
```

### Script Yükleme ve Çalıştırma

```cpp
// Dosyadan yükle
lua.LoadScript("scripts/events/welcome_event.lua");

// Direkt kod çalıştır
std::string code = R"(
    print("Hello from Lua!")
    function myFunc() return 42 end
)";
lua.ExecuteScript(code);

// Fonksiyon çağır
lua.CallFunction("myFunc");
```

### Event Sistemi

```cpp
// Event kaydet
lua.RegisterEvent("login", "scripts/events/welcome_event.lua");
lua.RegisterEvent("kill", "scripts/events/pvp_arena_event.lua");

// Event tetikle
lua.TriggerEvent("login", player);
lua.TriggerEvent("kill", killer, victim_id);
```

### C++'dan Lua'ya Fonksiyonlar

Lua scriptlerinde kullanılabilir fonksiyonlar:

```lua
-- Mesaj gönder
SendMessage(player, "Hoşgeldin!")

-- EXP/Yang ver
GiveExp(player, 1000)
GiveGold(player, 50000)

-- Item ver
GiveItem(player, 27001, 10)  -- İksir x10

-- Buff ekle/kaldır
AddAffect(player, 3, 100, 30000)  -- Attack +100, 30sn
RemoveAffect(player, 3)

-- Işınlama
TeleportPlayer(player, 950000, 250000)

-- Monster spawn
SpawnMonster(2493, 950000, 250000)  -- Azrael

-- Oyuncu bilgileri
local level = GetPlayerLevel(player)
local hp = GetPlayerHP(player)
SetPlayerHP(player, 5000)

-- Quest flag
SetQuestFlag(player, "quest_done", 1)
local flag = GetQuestFlag(player, "quest_done")
```

---

## Kullanım Örnekleri

### Örnek 1: PvP Arena

```cpp
void EnterPvPArena(CCharacter* player)
{
    // HP/SP doldur
    player->SetHP(player->GetMaxHP());
    player->SetSP(player->GetMaxSP());

    // Arena buffları
    CAffectManager* affect = player->GetAffectManager();
    affect->AddAffect(AFFECT_ATTACK_BOOST, 100, 3600000);   // +100 ATK, 1 saat
    affect->AddAffect(AFFECT_DEFENSE_BOOST, 100, 3600000);  // +100 DEF, 1 saat

    // Spawn noktasına ışınla
    TPosition arena_spawn(950000, 250000);
    player->SetPosition(arena_spawn);

    // Lua event tetikle
    CLuaBinding::Instance().TriggerEvent("arena_enter", player);
}
```

### Örnek 2: Boss Kill Ödülleri

```cpp
void OnBossKilled(CCharacter* killer, DWORD boss_vnum)
{
    // Temel ödüller
    killer->GiveExp(100000);
    killer->ChangeGold(1000000);

    // Boss'a özel bufflar
    CAffectManager* affect = killer->GetAffectManager();

    if (boss_vnum == 2493) {  // Azrael
        // 1 saatlik güçlü buff
        affect->AddAffect(AFFECT_ATTACK_BOOST, 200, 3600000);
        affect->AddAffect(AFFECT_HP_REGEN, 100, 3600000);
    }

    // Lua event ile özel ödüller
    CLuaBinding::Instance().TriggerEvent("boss_kill", killer, boss_vnum);
}
```

### Örnek 3: Skill Combo Sistemi

```cpp
class CComboSystem
{
private:
    std::map<CCharacter*, std::vector<DWORD>> m_combo_state;

public:
    void RegisterHit(CCharacter* ch, DWORD skill_vnum)
    {
        auto& combo = m_combo_state[ch];
        combo.push_back(skill_vnum);

        // 3-hit combo
        if (combo.size() == 3) {
            ch->GetAffectManager()->AddAffect(AFFECT_ATTACK_BOOST, 50, 5000);
            SendMessage(ch, "3-HIT COMBO! +50 ATK");
        }

        // 5-hit combo
        if (combo.size() == 5) {
            ch->GetAffectManager()->AddAffect(AFFECT_ATTACK_BOOST, 100, 10000);
            ch->GiveExp(5000);
            SendMessage(ch, "5-HIT MEGA COMBO! +100 ATK, +5000 EXP");
        }

        // Combo sıfırlama (3 saniye timeout)
        // ...
    }
};
```

### Örnek 4: Daily Quest

```lua
-- scripts/quests/daily_pvp.lua

function on_login(player)
    local daily_done = GetQuestFlag(player, "daily_pvp_done")

    if daily_done == 0 then
        SendMessage(player, "Günlük Görev: 10 oyuncu öldür")
        SendMessage(player, "Ödül: 500K Yang, 50K EXP")
        SetQuestFlag(player, "daily_pvp_kills", 0)
    end
end

function on_player_kill(killer)
    local daily_done = GetQuestFlag(killer, "daily_pvp_done")

    if daily_done == 1 then
        return
    end

    local kills = GetQuestFlag(killer, "daily_pvp_kills") + 1
    SetQuestFlag(killer, "daily_pvp_kills", kills)

    SendMessage(killer, "İlerleme: " .. kills .. "/10")

    if kills >= 10 then
        SendMessage(killer, "========================================")
        SendMessage(killer, "  GÜNLÜK GÖREV TAMAMLANDI!")
        SendMessage(killer, "========================================")

        GiveExp(killer, 50000)
        GiveGold(killer, 500000)

        SetQuestFlag(killer, "daily_pvp_done", 1)
    end
end
```

---

## Performans İpuçları

1. **Update Frequency**: Character::Update() her frame değil, 100ms'de bir çağrılabilir
2. **Affect Cleanup**: Süresi dolan affectler otomatik temizlenir
3. **Lua Cache**: Sık kullanılan scriptler memory'de cache'lenir
4. **Damage Calculation**: Kritik ve variance hesaplamaları optimize edilmiştir

## Derleme

```bash
# Gerekli kütüphaneler
sudo apt-get install lua5.3 liblua5.3-dev libmysqlclient-dev

# Test programı derleme
cd examples/game_system_test
g++ -o test_game test_game_systems.cpp \
    ../../src/game/*.cpp \
    ../../src/script/*.cpp \
    ../../src/db/DBManager.cpp \
    -I../../include \
    -llua5.3 -lmysqlclient -lssl -lcrypto -lpthread \
    -std=c++11

./test_game
```

---

## İleri Seviye Konular

- Party sistemi entegrasyonu
- Guild war mekanikleri
- Dungeon instance yönetimi
- Anti-cheat sistemleri
- Server-client senkronizasyonu

Detaylı bilgi için kaynak kodlara bakın.
