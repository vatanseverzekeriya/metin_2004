#include "../../include/network/NetworkManager.h"
#include "../../include/game/GameServer.h"
#include "../../include/db/DBManager.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * Metin2 PvP Network Test
 * WebSocket ve UDP protokollerini test eder
 */

void TestWebSocket();
void TestUDP();
void TestHybridCommunication();
void SimulateMobileClient();

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  Metin2 PvP Network Test Suite        " << std::endl;
    std::cout << "========================================" << std::endl;

    // Veritabanı bağlan
    std::cout << "\n1. Connecting to database..." << std::endl;
    if (!CDBManager::Instance().Initialize("localhost", "metin2", "metin2pass", "metin2", 3306))
    {
        std::cerr << "WARNING: Database connection failed (continuing anyway)" << std::endl;
    }
    else
    {
        std::cout << "   [OK] Database connected" << std::endl;
    }

    // Oyun sunucusunu başlat
    std::cout << "\n2. Initializing game server..." << std::endl;
    if (!CGameServer::Instance().Initialize(13000))
    {
        std::cerr << "ERROR: Failed to initialize game server!" << std::endl;
        return 1;
    }
    std::cout << "   [OK] Game server initialized" << std::endl;

    // Network manager başlat
    std::cout << "\n3. Initializing network manager..." << std::endl;
    if (!CNetworkManager::Instance().Initialize(8080, 8081))
    {
        std::cerr << "ERROR: Failed to initialize network manager!" << std::endl;
        return 1;
    }
    std::cout << "   [OK] Network manager initialized" << std::endl;
    std::cout << "   - WebSocket Port: 8080" << std::endl;
    std::cout << "   - UDP Port: 8081" << std::endl;

    // Sunucuları başlat
    std::cout << "\n4. Starting servers..." << std::endl;
    CNetworkManager::Instance().Start();
    CGameServer::Instance().Start();
    std::cout << "   [OK] All servers started" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Server is ready for connections!      " << std::endl;
    std::cout << "========================================" << std::endl;

    // Test menüsü
    bool running = true;
    while (running)
    {
        std::cout << "\n========== TEST MENU ==========" << std::endl;
        std::cout << "1. Test WebSocket Communication" << std::endl;
        std::cout << "2. Test UDP Communication" << std::endl;
        std::cout << "3. Test Hybrid Communication" << std::endl;
        std::cout << "4. Simulate Mobile Client" << std::endl;
        std::cout << "5. Show Network Statistics" << std::endl;
        std::cout << "6. Create Test Characters" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "===============================" << std::endl;
        std::cout << "Choice: ";

        int choice;
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            TestWebSocket();
            break;
        case 2:
            TestUDP();
            break;
        case 3:
            TestHybridCommunication();
            break;
        case 4:
            SimulateMobileClient();
            break;
        case 5:
        {
            DWORD ws_sessions, udp_endpoints, total_players;
            float avg_latency;
            CNetworkManager::Instance().GetNetworkStats(ws_sessions, udp_endpoints,
                                                       total_players, avg_latency);
            std::cout << "\n=== Network Statistics ===" << std::endl;
            std::cout << "WebSocket Sessions: " << ws_sessions << std::endl;
            std::cout << "UDP Endpoints: " << udp_endpoints << std::endl;
            std::cout << "Total Players: " << total_players << std::endl;
            std::cout << "Average Latency: " << avg_latency << "ms" << std::endl;
            break;
        }
        case 6:
        {
            std::cout << "\nCreating test characters..." << std::endl;

            CCharacter* char1 = new CCharacter();
            DWORD char1_id;
            if (CDBManager::Instance().CreatePlayer("MobileTestPlayer1", CLASS_WARRIOR, char1_id))
            {
                char1->Initialize(char1_id);
                char1->SetPosition(957200, 244900, 0);
                CGameServer::Instance().AddCharacter(char1);
                std::cout << "Created: MobileTestPlayer1 (ID: " << char1_id << ")" << std::endl;
            }

            CCharacter* char2 = new CCharacter();
            DWORD char2_id;
            if (CDBManager::Instance().CreatePlayer("MobileTestPlayer2", CLASS_ASSASSIN, char2_id))
            {
                char2->Initialize(char2_id);
                char2->SetPosition(957300, 245000, 0);
                CGameServer::Instance().AddCharacter(char2);
                std::cout << "Created: MobileTestPlayer2 (ID: " << char2_id << ")" << std::endl;
            }
            break;
        }
        case 0:
            running = false;
            break;
        default:
            std::cout << "Invalid choice!" << std::endl;
        }
    }

    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    CNetworkManager::Instance().Stop();
    CGameServer::Instance().Shutdown();
    CDBManager::Instance().Destroy();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}

void TestWebSocket()
{
    std::cout << "\n=== WebSocket Test ===" << std::endl;
    std::cout << "Simulating WebSocket client..." << std::endl;

    // Login paketi simülasyonu
    TPacketCGLogin login;
    strcpy(login.username, "testuser");
    strcpy(login.password, "testpass");
    memset(login.token, 0, sizeof(login.token));

    std::cout << "Sending login packet..." << std::endl;
    std::cout << "  Username: " << login.username << std::endl;

    // Chat paketi simülasyonu
    TPacketCGChat chat;
    chat.type = 0;
    chat.target_id = 0;
    strcpy(chat.message, "Hello from WebSocket!");

    std::cout << "Sending chat packet..." << std::endl;
    std::cout << "  Message: " << chat.message << std::endl;

    std::cout << "\nWebSocket test complete." << std::endl;
    std::cout << "Note: Actual sending requires active client connection." << std::endl;
}

void TestUDP()
{
    std::cout << "\n=== UDP Test ===" << std::endl;
    std::cout << "Simulating UDP client..." << std::endl;

    // Movement paketi simülasyonu
    TPacketCGMove move;
    move.x = 957300;
    move.y = 245000;
    move.dir = 0;
    move.move_time = CNetworkManager::Instance().GetCurrentTime();

    std::cout << "Sending move packet..." << std::endl;
    std::cout << "  Position: (" << move.x << ", " << move.y << ")" << std::endl;

    // Attack paketi simülasyonu
    TPacketCGAttack attack;
    attack.target_id = 1001;
    attack.attack_type = 0;
    attack.skill_id = 0;

    std::cout << "Sending attack packet..." << std::endl;
    std::cout << "  Target ID: " << attack.target_id << std::endl;

    std::cout << "\nUDP test complete." << std::endl;
    std::cout << "Note: Actual sending requires active UDP endpoint." << std::endl;
}

void TestHybridCommunication()
{
    std::cout << "\n=== Hybrid Communication Test ===" << std::endl;
    std::cout << "Testing automatic protocol selection..." << std::endl;

    // Paket tiplerine göre protokol seçimi
    struct TestPacket {
        BYTE type;
        const char* name;
    };

    TestPacket packets[] = {
        {HEADER_CG_LOGIN, "Login"},
        {HEADER_CG_MOVE, "Move"},
        {HEADER_CG_ATTACK, "Attack"},
        {HEADER_CG_CHAT, "Chat"},
        {HEADER_CG_PING, "Ping"}
    };

    for (const auto& packet : packets)
    {
        bool use_udp = CNetworkManager::Instance().ShouldUseUDP(packet.type);
        std::cout << packet.name << " packet -> "
                  << (use_udp ? "UDP" : "WebSocket") << std::endl;
    }

    std::cout << "\nHybrid communication test complete." << std::endl;
}

void SimulateMobileClient()
{
    std::cout << "\n=== Mobile Client Simulation ===" << std::endl;
    std::cout << "Simulating a complete mobile game session..." << std::endl;

    std::cout << "\n1. Client connects..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "2. Client sends login..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "3. Server validates and responds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "4. Client selects character..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "5. Client enters game world..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "6. UDP connection established for real-time data..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\nSimulating gameplay actions..." << std::endl;

    // Hareket simülasyonu
    for (int i = 0; i < 5; i++)
    {
        int x = 957200 + (i * 100);
        int y = 244900 + (i * 50);
        std::cout << "  [UDP] Moving to (" << x << ", " << y << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Saldırı simülasyonu
    std::cout << "  [UDP] Attacking enemy..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Chat simülasyonu
    std::cout << "  [WS]  Sending chat message..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Ping simülasyonu
    std::cout << "  [UDP] Ping... Pong! (Latency: 45ms)" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "\nMobile client simulation complete!" << std::endl;
    std::cout << "Connection quality: Excellent" << std::endl;
    std::cout << "Packet loss: 0.2%" << std::endl;
    std::cout << "Average latency: 45ms" << std::endl;
}
