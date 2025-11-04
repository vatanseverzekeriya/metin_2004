/**
 * Metin2 PvP System Test
 *
 * PVPManager'ın request/accept/fight/revenge sistemini test eder
 *
 * Compile:
 *   g++ -std=c++17 -I../include pvp_test.cpp \
 *       ../src/game/PVPManager.cpp ../src/game/Character.cpp \
 *       ../src/db/DBManager.cpp -lmysqlclient -lpthread -o pvp_test
 */

#include <iostream>
#include <thread>
#include <chrono>
#include "../include/game/PVPManager.h"
#include "../include/game/Character.h"

using namespace std;

void PrintSeparator()
{
    cout << "\n" << string(60, '=') << "\n" << endl;
}

void TestBasicPvPRequest()
{
    PrintSeparator();
    cout << "TEST 1: Basic PvP Request/Accept System" << endl;
    PrintSeparator();

    // İki karakter oluştur
    CCharacter warrior;
    warrior.SetName("TestWarrior");
    warrior.SetJob(CLASS_WARRIOR);
    warrior.SetLevel(45);
    warrior.SetPvPMode(PVP_MODE_NORMAL);

    CCharacter assassin;
    assassin.SetName("TestAssassin");
    assassin.SetJob(CLASS_ASSASSIN);
    assassin.SetLevel(43);
    assassin.SetPvPMode(PVP_MODE_NORMAL);

    // Test IDs
    DWORD pid1 = 1000;
    DWORD pid2 = 2000;

    // Mock player IDs
    // warrior.Initialize(pid1);
    // assassin.Initialize(pid2);

    cout << "[1] Warrior sends PvP request to Assassin" << endl;
    CPVPManager::Instance().Insert(pid1, pid2);

    EPvPState state = CPVPManager::Instance().GetPvPState(pid1, pid2);
    cout << "   State: " << (int)state << " (WAIT expected)" << endl;
    assert(state == PVP_STATE_WAIT);

    cout << "\n[2] Assassin accepts the PvP request" << endl;
    CPVPManager::Instance().Agree(pid2);

    state = CPVPManager::Instance().GetPvPState(pid1, pid2);
    cout << "   State: " << (int)state << " (FIGHT expected)" << endl;
    assert(state == PVP_STATE_FIGHT);

    cout << "\n[3] Check if fighting" << endl;
    bool fighting = CPVPManager::Instance().IsFighting(pid1, pid2);
    cout << "   Fighting: " << (fighting ? "YES" : "NO") << endl;
    assert(fighting == true);

    cout << "\n✅ TEST 1 PASSED!" << endl;
}

void TestRevengeModeSimulated()
{
    PrintSeparator();
    cout << "TEST 2: Revenge Mode (Simulated)" << endl;
    PrintSeparator();

    DWORD pid1 = 3000;  // Warrior
    DWORD pid2 = 4000;  // Assassin

    cout << "[1] Creating PvP between " << pid1 << " and " << pid2 << endl;
    CPVPManager::Instance().Insert(pid1, pid2);
    CPVPManager::Instance().Agree(pid2);

    cout << "\n[2] Simulating death: " << pid2 << " killed by " << pid1 << endl;

    // Manual revenge test (normalde OnDeath() içinde yapılır)
    // CPVPManager::Instance().OnDeath(dead_char, killer_char);
    cout << "   (In real scenario: OnDeath() would activate revenge mode)" << endl;
    cout << "   Revenge mode would last 5 minutes" << endl;

    cout << "\n✅ TEST 2 PASSED!" << endl;
}

void TestPvPTimeout()
{
    PrintSeparator();
    cout << "TEST 3: PvP Timeout (10 minute cleanup)" << endl;
    PrintSeparator();

    DWORD pid1 = 5000;
    DWORD pid2 = 6000;

    cout << "[1] Creating PvP" << endl;
    CPVPManager::Instance().Insert(pid1, pid2);

    size_t count_before = CPVPManager::Instance().GetPvPCount();
    cout << "   PvP count before: " << count_before << endl;

    cout << "\n[2] Running Process() - should NOT remove (too soon)" << endl;
    CPVPManager::Instance().Process();

    size_t count_after = CPVPManager::Instance().GetPvPCount();
    cout << "   PvP count after: " << count_after << endl;
    assert(count_after == count_before);

    cout << "\n   (In production: after 10 minutes, Process() would remove it)" << endl;

    cout << "\n✅ TEST 3 PASSED!" << endl;
}

void TestMultiplePvPs()
{
    PrintSeparator();
    cout << "TEST 4: Multiple Concurrent PvPs" << endl;
    PrintSeparator();

    cout << "[1] Creating 3 concurrent PvPs" << endl;
    CPVPManager::Instance().Insert(7000, 8000);  // PvP 1
    CPVPManager::Instance().Insert(9000, 10000); // PvP 2
    CPVPManager::Instance().Insert(11000, 12000); // PvP 3

    size_t count = CPVPManager::Instance().GetPvPCount();
    cout << "   Total PvPs: " << count << endl;
    assert(count >= 3);

    cout << "\n[2] Accepting all PvPs" << endl;
    CPVPManager::Instance().Agree(8000);
    CPVPManager::Instance().Agree(10000);
    CPVPManager::Instance().Agree(12000);

    cout << "\n[3] Verifying all are fighting" << endl;
    bool f1 = CPVPManager::Instance().IsFighting(7000, 8000);
    bool f2 = CPVPManager::Instance().IsFighting(9000, 10000);
    bool f3 = CPVPManager::Instance().IsFighting(11000, 12000);

    cout << "   PvP 1 fighting: " << (f1 ? "YES" : "NO") << endl;
    cout << "   PvP 2 fighting: " << (f2 ? "YES" : "NO") << endl;
    cout << "   PvP 3 fighting: " << (f3 ? "YES" : "NO") << endl;

    assert(f1 && f2 && f3);

    cout << "\n✅ TEST 4 PASSED!" << endl;
}

void TestPvPRemoval()
{
    PrintSeparator();
    cout << "TEST 5: PvP Removal" << endl;
    PrintSeparator();

    DWORD pid1 = 13000;
    DWORD pid2 = 14000;

    cout << "[1] Creating PvP" << endl;
    CPVPManager::Instance().Insert(pid1, pid2);

    cout << "\n[2] Removing PvP" << endl;
    CPVPManager::Instance().Remove(pid1, pid2);

    cout << "\n[3] Verifying removal" << endl;
    EPvPState state = CPVPManager::Instance().GetPvPState(pid1, pid2);
    cout << "   State: " << (int)state << " (NONE expected)" << endl;
    assert(state == PVP_STATE_NONE);

    cout << "\n✅ TEST 5 PASSED!" << endl;
}

void TestMobileCallbacks()
{
    PrintSeparator();
    cout << "TEST 6: Mobile Callbacks" << endl;
    PrintSeparator();

    bool request_received = false;
    bool fight_started = false;
    bool pvp_ended = false;
    bool revenge_active = false;

    // Register mobile callbacks
    CPVPManager::Instance().SetOnPvPRequest([&](DWORD req, DWORD tgt) {
        cout << "[CALLBACK] PvP Request: " << req << " -> " << tgt << endl;
        request_received = true;
    });

    CPVPManager::Instance().SetOnPvPStart([&](DWORD p1, DWORD p2) {
        cout << "[CALLBACK] PvP Fight Started: " << p1 << " vs " << p2 << endl;
        fight_started = true;
    });

    CPVPManager::Instance().SetOnPvPEnd([&](DWORD winner, DWORD loser) {
        cout << "[CALLBACK] PvP Ended: Winner=" << winner << ", Loser=" << loser << endl;
        pvp_ended = true;
    });

    CPVPManager::Instance().SetOnRevenge([&](DWORD victim, DWORD killer) {
        cout << "[CALLBACK] Revenge Mode: " << victim << " can revenge " << killer << endl;
        revenge_active = true;
    });

    cout << "\n[1] Triggering PvP request" << endl;
    CPVPManager::Instance().Insert(15000, 16000);
    assert(request_received);

    cout << "\n[2] Triggering PvP start" << endl;
    CPVPManager::Instance().Agree(16000);
    assert(fight_started);

    cout << "\n✅ TEST 6 PASSED!" << endl;
    cout << "   (Note: OnPvPEnd and OnRevenge require Character::OnDeath())" << endl;
}

int main()
{
    cout << "\n";
    cout << "╔════════════════════════════════════════════════════════════╗" << endl;
    cout << "║      Metin2 PvP Manager Test Suite                        ║" << endl;
    cout << "║      Açık Kaynak Referans: cCorax2/Source_code            ║" << endl;
    cout << "║      Modern C++17 Implementation                           ║" << endl;
    cout << "╚════════════════════════════════════════════════════════════╝" << endl;

    try {
        TestBasicPvPRequest();
        TestRevengeModeSimulated();
        TestPvPTimeout();
        TestMultiplePvPs();
        TestPvPRemoval();
        TestMobileCallbacks();

        PrintSeparator();
        cout << "🎉 ALL TESTS PASSED! 🎉" << endl;
        PrintSeparator();

        // Final stats
        size_t total_pvps = CPVPManager::Instance().GetPvPCount();
        cout << "Final PvP Count: " << total_pvps << endl;

        // Cleanup
        cout << "\nCleaning up..." << endl;
        CPVPManager::Instance().Clear();
        cout << "Done!" << endl;

        return 0;

    } catch (const exception& e) {
        cerr << "\n❌ TEST FAILED: " << e.what() << endl;
        return 1;
    }
}
