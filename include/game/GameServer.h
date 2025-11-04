#ifndef __INC_GAME_SERVER_H__
#define __INC_GAME_SERVER_H__

#include "../common/types.h"
#include "Character.h"
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <memory>

// Paket tipleri
enum EPacketType
{
    PACKET_LOGIN = 1,
    PACKET_LOGOUT = 2,
    PACKET_MOVE = 3,
    PACKET_ATTACK = 4,
    PACKET_CHAT = 5,
    PACKET_CREATE_CHARACTER = 6,
    PACKET_SELECT_CHARACTER = 7,
    PACKET_CHARACTER_INFO = 8,
    PACKET_DAMAGE = 9,
    PACKET_DEATH = 10,
    PACKET_SPAWN = 11,
    PACKET_DESPAWN = 12,
};

// Paket yapıları
struct TPacketHeader
{
    BYTE type;
    WORD size;
};

struct TPacketLogin
{
    char login[32];
    char password[32];
};

struct TPacketMove
{
    DWORD player_id;
    LONG x;
    LONG y;
};

struct TPacketAttack
{
    DWORD attacker_id;
    DWORD victim_id;
};

struct TPacketChat
{
    DWORD player_id;
    char message[256];
};

struct TPacketCharacterInfo
{
    DWORD player_id;
    char name[32];
    BYTE job;
    TPlayerStats stats;
    TPosition position;
};

class CGameServer
{
public:
    static CGameServer& Instance();

    bool Initialize(int port);
    void Shutdown();

    void Run();
    void Update();

    // Karakter yönetimi
    CCharacter* FindCharacter(DWORD player_id);
    CCharacter* FindCharacterByName(const std::string& name);
    bool AddCharacter(CCharacter* ch);
    void RemoveCharacter(DWORD player_id);

    // Spawn yönetimi
    void SpawnCharacter(CCharacter* ch);
    void DespawnCharacter(CCharacter* ch);

    // Broadcast
    void BroadcastCharacterInfo(CCharacter* ch);
    void BroadcastMove(CCharacter* ch);
    void BroadcastAttack(CCharacter* attacker, CCharacter* victim, DWORD damage);
    void BroadcastDeath(CCharacter* ch, CCharacter* killer);
    void BroadcastChat(CCharacter* ch, const std::string& message);

    // Oyun mekaniği
    void ProcessMove(DWORD player_id, LONG x, LONG y);
    void ProcessAttack(DWORD attacker_id, DWORD victim_id);
    void ProcessChat(DWORD player_id, const std::string& message);

    // Alan kontrolü
    std::vector<CCharacter*> GetNearbyCharacters(const TPosition& pos, DWORD range);

    bool IsRunning() const { return m_bRunning; }

private:
    CGameServer();
    ~CGameServer();

    CGameServer(const CGameServer&) = delete;
    CGameServer& operator=(const CGameServer&) = delete;

    // Karakter haritası
    std::map<DWORD, CCharacter*> m_mapCharacters;
    std::mutex m_mutexCharacters;

    // Sunucu durumu
    bool m_bRunning;
    int m_iPort;

    // Ana döngü
    void MainLoop();
};

#endif // __INC_GAME_SERVER_H__
