/**
 * Metin2 PvP Server - Oyun Sistemleri Test Programı
 *
 * Bu program şunları test eder:
 * - Hareket sistemi
 * - Combat ve skill sistemi
 * - Can/Mana rejenerasyonu
 * - Buff/Debuff sistemi
 * - Lua event sistemi
 */

#include "../../include/game/Character.h"
#include "../../include/game/Movement.h"
#include "../../include/game/Combat.h"
#include "../../include/game/AffectManager.h"
#include "../../include/script/LuaBinding.h"
#include "../../include/db/DBManager.h"

#include <iostream>
#include <thread>
#include <chrono>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

void PrintSeparator() {
    std::cout << COLOR_CYAN << "========================================" << COLOR_RESET << std::endl;
}

void PrintSuccess(const std::string& msg) {
    std::cout << COLOR_GREEN << "[✓] " << msg << COLOR_RESET << std::endl;
}

void PrintError(const std::string& msg) {
    std::cout << COLOR_RED << "[✗] " << msg << COLOR_RESET << std::endl;
}

void PrintInfo(const std::string& msg) {
    std::cout << COLOR_BLUE << "[INFO] " << msg << COLOR_RESET << std::endl;
}

// Test 1: Hareket Sistemi
bool TestMovementSystem()
{
    PrintSeparator();
    PrintInfo("Test 1: Hareket Sistemi");
    PrintSeparator();

    CMovement movement;

    TPosition start(100, 100);
    TPosition target(500, 500);

    // Hareket başlat
    if (!movement.StartMove(start, target, MOVE_TYPE_RUN))
    {
        PrintError("Hareket başlatılamadı!");
        return false;
    }

    PrintSuccess("Hareket başlatıldı");
    std::cout << "Başlangıç: (" << start.x << ", " << start.y << ")" << std::endl;
    std::cout << "Hedef: (" << target.x << ", " << target.y << ")" << std::endl;
    std::cout << "Mesafe: " << movement.CalculateDistance(start, target) << std::endl;

    // Hareketi simüle et
    int updates = 0;
    while (movement.IsMoving() && updates < 100)
    {
        movement.UpdateMove(0.016f);  // 60 FPS (16ms)
        updates++;

        if (updates % 20 == 0)
        {
            TPosition current = movement.GetCurrentPosition();
            float progress = movement.GetProgress() * 100.0f;
            std::cout << "İlerleme: " << (int)progress << "% - Pozisyon: ("
                      << current.x << ", " << current.y << ")" << std::endl;
        }
    }

    if (movement.HasArrived())
    {
        PrintSuccess("Hedefe ulaşıldı!");
        return true;
    }
    else
    {
        PrintError("Hedefe ulaşılamadı!");
        return false;
    }
}

// Test 2: Combat Sistemi
bool TestCombatSystem()
{
    PrintSeparator();
    PrintInfo("Test 2: Combat ve Skill Sistemi");
    PrintSeparator();

    // Saldırgan ve hedef karakterler
    CCharacter attacker;
    CCharacter victim;

    attacker.SetName("Saldırgan");
    victim.SetName("Hedef");

    attacker.SetLevel(50);
    victim.SetLevel(45);

    TPlayerStats attacker_stats;
    attacker_stats.attack = 500;
    attacker_stats.defense = 200;
    attacker_stats.hp = 5000;
    attacker_stats.max_hp = 5000;
    attacker_stats.sp = 500;
    attacker_stats.max_sp = 500;
    attacker.SetStats(attacker_stats);

    TPlayerStats victim_stats;
    victim_stats.attack = 300;
    victim_stats.defense = 250;
    victim_stats.hp = 4000;
    victim_stats.max_hp = 4000;
    victim.SetStats(victim_stats);

    attacker.SetPosition(100, 100);
    victim.SetPosition(150, 100);  // 50 birim uzakta

    PrintSuccess("Karakterler oluşturuldu");
    std::cout << attacker.GetName() << " - Lv." << attacker.GetLevel()
              << " ATK:" << attacker.GetAttack() << " DEF:" << attacker.GetDefense() << std::endl;
    std::cout << victim.GetName() << " - Lv." << victim.GetLevel()
              << " ATK:" << victim.GetAttack() << " DEF:" << victim.GetDefense() << std::endl;

    // Combat sistemi
    CCombatSystem& combat = CCombatSystem::Instance();

    // Normal saldırı
    std::cout << "\n" << COLOR_YELLOW << "Normal Saldırı:" << COLOR_RESET << std::endl;
    TDamageInfo dmg = combat.CalculateDamage(&attacker, &victim, 0);
    combat.ApplyDamage(dmg);

    std::cout << victim.GetName() << " HP: " << victim.GetHP() << "/" << victim.GetMaxHP() << std::endl;

    // Skill saldırısı
    std::cout << "\n" << COLOR_YELLOW << "Skill Saldırısı:" << COLOR_RESET << std::endl;

    // Test skill kaydet
    TSkillProto fire_skill;
    fire_skill.vnum = 100;
    fire_skill.name = "Ateş Topu";
    fire_skill.type = SKILL_TYPE_ATTACK;
    fire_skill.target_type = SKILL_TARGET_ENEMY;
    fire_skill.damage_base = 300;
    fire_skill.damage_multiplier = 1.5f;
    fire_skill.sp_cost = 50;
    fire_skill.cooldown_ms = 3000;
    fire_skill.range = 500;
    combat.RegisterSkill(fire_skill);

    if (combat.UseSkill(&attacker, 100, &victim))
    {
        PrintSuccess("Skill kullanıldı!");
    }

    std::cout << victim.GetName() << " HP: " << victim.GetHP() << "/" << victim.GetMaxHP() << std::endl;

    // Kritik vuruş testi
    std::cout << "\n" << COLOR_YELLOW << "Kritik Vuruş Testi (20 deneme):" << COLOR_RESET << std::endl;
    int critical_count = 0;
    for (int i = 0; i < 20; i++)
    {
        if (combat.IsCriticalHit(&attacker, &victim))
            critical_count++;
    }
    std::cout << "Kritik sayısı: " << critical_count << "/20" << std::endl;

    return true;
}

// Test 3: Buff/Debuff Sistemi
bool TestAffectSystem()
{
    PrintSeparator();
    PrintInfo("Test 3: Buff/Debuff Sistemi");
    PrintSeparator();

    CCharacter player;
    player.SetName("Test Oyuncu");
    player.SetLevel(30);

    TPlayerStats stats;
    stats.hp = 3000;
    stats.max_hp = 3000;
    stats.sp = 300;
    stats.max_sp = 300;
    stats.attack = 300;
    stats.defense = 150;
    player.SetStats(stats);

    CAffectManager* affect_mgr = player.GetAffectManager();
    if (!affect_mgr)
    {
        PrintError("Affect Manager yok!");
        return false;
    }

    // Attack buff ekle
    std::cout << "\n" << COLOR_YELLOW << "Attack Buff Ekleme:" << COLOR_RESET << std::endl;
    std::cout << "Önceki Attack: " << player.GetAttack() << std::endl;
    std::cout << "Önceki Attack Bonus: " << affect_mgr->GetAttackBonus() << std::endl;

    affect_mgr->AddAffect(AFFECT_ATTACK_BOOST, 100, 10000);  // +100 attack, 10 saniye

    std::cout << "Sonraki Attack Bonus: " << affect_mgr->GetAttackBonus() << std::endl;
    std::cout << "Toplam Attack: " << (player.GetAttack() + affect_mgr->GetAttackBonus()) << std::endl;

    // Speed buff ekle
    std::cout << "\n" << COLOR_YELLOW << "Speed Buff Ekleme:" << COLOR_RESET << std::endl;
    affect_mgr->AddAffect(AFFECT_SPEED_BOOST, 50, 10000);  // +50% speed, 10 saniye
    std::cout << "Speed Multiplier: " << affect_mgr->GetSpeedMultiplier() << std::endl;

    // Zehir debuff ekle
    std::cout << "\n" << COLOR_YELLOW << "Zehir Debuff Ekleme:" << COLOR_RESET << std::endl;
    affect_mgr->AddAffect(AFFECT_POISON, 10, 5000);  // 10 hasar/saniye, 5 saniye

    // Rejenerasyon testi
    std::cout << "\n" << COLOR_YELLOW << "Rejenerasyon Testi (5 saniye):" << COLOR_RESET << std::endl;
    player.DecreaseHP(1000);  // HP azalt
    std::cout << "HP azaltıldı: " << player.GetHP() << "/" << player.GetMaxHP() << std::endl;

    for (int i = 0; i < 5; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        affect_mgr->Update(1.0f);  // 1 saniye güncelle
        std::cout << "Saniye " << (i+1) << " - HP: " << player.GetHP() << std::endl;
    }

    // Affect listesi
    std::cout << "\n" << COLOR_YELLOW << "Aktif Affectler:" << COLOR_RESET << std::endl;
    const auto& affects = affect_mgr->GetAllAffects();
    for (const auto& affect : affects)
    {
        std::cout << "- Type: " << (int)affect.type
                  << ", Value: " << affect.value
                  << ", Kalan süre: " << affect.GetRemainingTime() / 1000.0f << "s" << std::endl;
    }

    return true;
}

// Test 4: Lua Event Sistemi
bool TestLuaEventSystem()
{
    PrintSeparator();
    PrintInfo("Test 4: Lua Event Sistemi");
    PrintSeparator();

    CLuaBinding& lua = CLuaBinding::Instance();

    // Lua başlat
    if (!lua.Initialize())
    {
        PrintError("Lua başlatılamadı!");
        return false;
    }

    PrintSuccess("Lua sistemi başlatıldı");

    // Basit Lua scripti çalıştır
    std::cout << "\n" << COLOR_YELLOW << "Basit Lua Scripti:" << COLOR_RESET << std::endl;

    std::string test_script = R"(
        print("Hello from Lua!")
        function test_function()
            print("Test function called")
            return 42
        end
    )";

    if (lua.ExecuteScript(test_script))
    {
        PrintSuccess("Script çalıştırıldı");
    }

    // Fonksiyon çağır
    if (lua.CallFunction("test_function"))
    {
        PrintSuccess("Lua fonksiyonu çağrıldı");
    }

    // Global değişkenler
    std::cout << "\n" << COLOR_YELLOW << "Global Değişkenler:" << COLOR_RESET << std::endl;
    lua.SetGlobalNumber("server_max_level", 99);
    lua.SetGlobalString("server_name", "Metin2 PvP Server");

    std::cout << "Max Level: " << lua.GetGlobalNumber("server_max_level") << std::endl;
    std::cout << "Server Name: " << lua.GetGlobalString("server_name") << std::endl;

    // Event sistemi
    std::cout << "\n" << COLOR_YELLOW << "Event Sistemi:" << COLOR_RESET << std::endl;
    lua.RegisterEvent("login", "../../scripts/events/welcome_event.lua");
    PrintSuccess("Event kaydedildi: login -> welcome_event.lua");

    // Character ile event tetikle
    CCharacter player;
    player.SetName("TestPlayer");
    player.SetLevel(1);

    if (lua.TriggerEvent("login", &player))
    {
        PrintSuccess("Login event tetiklendi!");
    }

    return true;
}

// Test 5: Entegre Senaryo
bool TestIntegratedScenario()
{
    PrintSeparator();
    PrintInfo("Test 5: Entegre Senaryo (PvP Savaşı)");
    PrintSeparator();

    // İki oyuncu oluştur
    CCharacter player1;
    CCharacter player2;

    player1.SetName("Savaşçı");
    player2.SetName("Ninja");

    player1.SetLevel(50);
    player2.SetLevel(48);

    TPlayerStats stats1, stats2;
    stats1.hp = 5000;
    stats1.max_hp = 5000;
    stats1.sp = 500;
    stats1.max_sp = 500;
    stats1.attack = 450;
    stats1.defense = 300;
    player1.SetStats(stats1);

    stats2.hp = 4000;
    stats2.max_hp = 4000;
    stats2.sp = 600;
    stats2.max_sp = 600;
    stats2.attack = 500;
    stats2.defense = 250;
    player2.SetStats(stats2);

    player1.SetPosition(1000, 1000);
    player2.SetPosition(1100, 1000);  // 100 birim uzakta

    PrintSuccess("Oyuncular hazır");
    std::cout << player1.GetName() << " vs " << player2.GetName() << std::endl;

    // Savaş simülasyonu
    CCombatSystem& combat = CCombatSystem::Instance();

    std::cout << "\n" << COLOR_MAGENTA << "==== SAVAŞ BAŞLADI ====" << COLOR_RESET << std::endl;

    for (int turn = 1; turn <= 10 && !player1.IsDead() && !player2.IsDead(); turn++)
    {
        std::cout << "\n" << COLOR_CYAN << "=== TUR " << turn << " ===" << COLOR_RESET << std::endl;

        // Player1 saldırır
        TDamageInfo dmg1 = combat.CalculateDamage(&player1, &player2);
        combat.ApplyDamage(dmg1);
        std::cout << player2.GetName() << " HP: " << player2.GetHP() << "/" << player2.GetMaxHP() << std::endl;

        if (player2.IsDead())
        {
            std::cout << COLOR_RED << player2.GetName() << " ÖLDÜ!" << COLOR_RESET << std::endl;
            break;
        }

        // Player2 saldırır
        TDamageInfo dmg2 = combat.CalculateDamage(&player2, &player1);
        combat.ApplyDamage(dmg2);
        std::cout << player1.GetName() << " HP: " << player1.GetHP() << "/" << player1.GetMaxHP() << std::endl;

        if (player1.IsDead())
        {
            std::cout << COLOR_RED << player1.GetName() << " ÖLDÜ!" << COLOR_RESET << std::endl;
            break;
        }

        // Her 3 turda buff
        if (turn % 3 == 0)
        {
            player1.GetAffectManager()->AddAffect(AFFECT_ATTACK_BOOST, 50, 3000);
            std::cout << COLOR_GREEN << player1.GetName() << " attack buff aldı!" << COLOR_RESET << std::endl;
        }
    }

    std::cout << "\n" << COLOR_MAGENTA << "==== SAVAŞ BİTTİ ====" << COLOR_RESET << std::endl;

    if (!player1.IsDead() && player2.IsDead())
    {
        std::cout << COLOR_GREEN << "KAZANAN: " << player1.GetName() << COLOR_RESET << std::endl;
    }
    else if (player1.IsDead() && !player2.IsDead())
    {
        std::cout << COLOR_GREEN << "KAZANAN: " << player2.GetName() << COLOR_RESET << std::endl;
    }
    else
    {
        std::cout << COLOR_YELLOW << "BERABERE!" << COLOR_RESET << std::endl;
    }

    return true;
}

int main()
{
    std::cout << COLOR_CYAN << R"(
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║        Metin2 PvP Server - Oyun Sistemleri Testi              ║
║                                                                ║
║  Hareket, Combat, Buff/Debuff, Lua Event sistemlerini         ║
║  kapsamlı bir şekilde test eder.                              ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝
)" << COLOR_RESET << std::endl;

    int tests_passed = 0;
    int tests_total = 5;

    // Test 1
    if (TestMovementSystem())
        tests_passed++;

    // Test 2
    if (TestCombatSystem())
        tests_passed++;

    // Test 3
    if (TestAffectSystem())
        tests_passed++;

    // Test 4
    if (TestLuaEventSystem())
        tests_passed++;

    // Test 5
    if (TestIntegratedScenario())
        tests_passed++;

    // Sonuçlar
    PrintSeparator();
    std::cout << "\n" << COLOR_CYAN << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║          TEST SONUÇLARI                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << COLOR_RESET << std::endl;

    std::cout << "Geçen Testler: " << COLOR_GREEN << tests_passed << COLOR_RESET
              << " / " << tests_total << std::endl;

    if (tests_passed == tests_total)
    {
        PrintSuccess("TÜM TESTLER BAŞARILI!");
        return 0;
    }
    else
    {
        PrintError("BAZI TESTLER BAŞARISIZ!");
        return 1;
    }
}
