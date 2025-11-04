#include "../../include/game/GameServer.h"
#include "../../include/db/DBManager.h"
#include "../../include/game/Character.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <ctime>

// Sunucu kapatma için signal handler
void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nShutdown signal received..." << std::endl;
        CGameServer::Instance().Shutdown();
        exit(0);
    }
}

// Test amaçlı karakter oluşturma
void CreateTestCharacters()
{
    std::cout << "\n=== Creating Test Characters ===" << std::endl;

    // Test karakteri 1: Warrior
    CCharacter* warrior = new CCharacter();
    DWORD warrior_id;
    if (CDBManager::Instance().CreatePlayer("TestWarrior", CLASS_WARRIOR, warrior_id))
    {
        warrior->Initialize(warrior_id);
        warrior->SetPosition(957200, 244900, 0);
        warrior->SetPvPMode(PVP_MODE_NORMAL);
        CGameServer::Instance().AddCharacter(warrior);
        CGameServer::Instance().SpawnCharacter(warrior);
    }

    // Test karakteri 2: Assassin
    CCharacter* assassin = new CCharacter();
    DWORD assassin_id;
    if (CDBManager::Instance().CreatePlayer("TestAssassin", CLASS_ASSASSIN, assassin_id))
    {
        assassin->Initialize(assassin_id);
        assassin->SetPosition(957300, 245000, 0);
        assassin->SetPvPMode(PVP_MODE_NORMAL);
        CGameServer::Instance().AddCharacter(assassin);
        CGameServer::Instance().SpawnCharacter(assassin);
    }

    std::cout << "=== Test Characters Created ===" << std::endl << std::endl;
}

// PvP test senaryosu
void TestPvPScenario()
{
    std::cout << "\n=== Testing PvP Scenario ===" << std::endl;

    CCharacter* warrior = CGameServer::Instance().FindCharacterByName("TestWarrior");
    CCharacter* assassin = CGameServer::Instance().FindCharacterByName("TestAssassin");

    if (!warrior || !assassin)
    {
        std::cout << "Test characters not found!" << std::endl;
        return;
    }

    std::cout << "\n--- Initial Stats ---" << std::endl;
    std::cout << warrior->GetName() << " - Level: " << warrior->GetLevel()
              << " HP: " << warrior->GetHP() << "/" << warrior->GetMaxHP()
              << " ATK: " << warrior->GetAttack() << std::endl;
    std::cout << assassin->GetName() << " - Level: " << assassin->GetLevel()
              << " HP: " << assassin->GetHP() << "/" << assassin->GetMaxHP()
              << " ATK: " << assassin->GetAttack() << std::endl;

    std::cout << "\n--- Combat Simulation ---" << std::endl;

    // Warrior assassin'e yaklaşıyor
    warrior->MoveTo(957300, 245000);

    // 10 tur savaş
    for (int i = 0; i < 10 && !warrior->IsDead() && !assassin->IsDead(); ++i)
    {
        std::cout << "\n>> Round " << (i + 1) << std::endl;

        // Warrior saldırıyor
        warrior->Attack(assassin);
        std::cout << assassin->GetName() << " HP: " << assassin->GetHP()
                  << "/" << assassin->GetMaxHP() << std::endl;

        if (assassin->IsDead())
            break;

        // Assassin karşılık veriyor
        assassin->Attack(warrior);
        std::cout << warrior->GetName() << " HP: " << warrior->GetHP()
                  << "/" << warrior->GetMaxHP() << std::endl;
    }

    std::cout << "\n--- Combat Results ---" << std::endl;
    if (warrior->IsDead())
        std::cout << "Winner: " << assassin->GetName() << std::endl;
    else if (assassin->IsDead())
        std::cout << "Winner: " << warrior->GetName() << std::endl;
    else
        std::cout << "Combat ended - Both alive" << std::endl;

    std::cout << "\n--- Final PvP Stats ---" << std::endl;
    DWORD w_kills, w_deaths, a_kills, a_deaths;
    CDBManager::Instance().GetPvPStats(warrior->GetPlayerID(), w_kills, w_deaths);
    CDBManager::Instance().GetPvPStats(assassin->GetPlayerID(), a_kills, a_deaths);

    std::cout << warrior->GetName() << " - Kills: " << w_kills
              << " Deaths: " << w_deaths << std::endl;
    std::cout << assassin->GetName() << " - Kills: " << a_kills
              << " Deaths: " << a_deaths << std::endl;

    std::cout << "=== PvP Test Complete ===" << std::endl << std::endl;
}

int main(int argc, char* argv[])
{
    // Random seed
    srand(time(nullptr));

    // Signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "==================================" << std::endl;
    std::cout << "   Metin2 PvP Server v1.0        " << std::endl;
    std::cout << "==================================" << std::endl;

    // Veritabanına bağlan
    std::cout << "\nConnecting to database..." << std::endl;
    if (!CDBManager::Instance().Initialize(
        "localhost",    // host
        "metin2",       // user
        "metin2pass",   // password
        "metin2",       // database
        3306            // port
    ))
    {
        std::cerr << "Failed to connect to database!" << std::endl;
        std::cerr << "Please check your database configuration." << std::endl;
        std::cerr << "\nNote: This is a demo. You can modify the database" << std::endl;
        std::cerr << "settings in src/game/main.cpp" << std::endl;
        return 1;
    }

    // Oyun sunucusunu başlat
    std::cout << "\nInitializing game server..." << std::endl;
    if (!CGameServer::Instance().Initialize(13000))
    {
        std::cerr << "Failed to initialize game server!" << std::endl;
        return 1;
    }

    // Test karakterleri oluştur
    CreateTestCharacters();

    // PvP senaryosunu test et
    TestPvPScenario();

    std::cout << "\n==================================" << std::endl;
    std::cout << "Server is running..." << std::endl;
    std::cout << "Press Ctrl+C to shutdown" << std::endl;
    std::cout << "==================================" << std::endl;

    // Sunucuyu çalıştır
    CGameServer::Instance().Run();

    // Cleanup
    CDBManager::Instance().Destroy();

    return 0;
}
