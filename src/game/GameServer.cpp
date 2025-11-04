#include "../../include/game/GameServer.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

CGameServer::CGameServer()
    : m_bRunning(false)
    , m_iPort(13000)
{
}

CGameServer::~CGameServer()
{
    Shutdown();
}

CGameServer& CGameServer::Instance()
{
    static CGameServer instance;
    return instance;
}

bool CGameServer::Initialize(int port)
{
    m_iPort = port;
    m_bRunning = true;

    std::cout << "==================================" << std::endl;
    std::cout << "   Metin2 PvP Server Starting    " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Port: " << m_iPort << std::endl;
    std::cout << "==================================" << std::endl;

    return true;
}

void CGameServer::Shutdown()
{
    if (!m_bRunning)
        return;

    std::cout << "Shutting down server..." << std::endl;
    m_bRunning = false;

    // Tüm karakterleri kaydet ve temizle
    std::lock_guard<std::mutex> lock(m_mutexCharacters);
    for (auto& pair : m_mapCharacters)
    {
        if (pair.second)
        {
            pair.second->Save();
            delete pair.second;
        }
    }
    m_mapCharacters.clear();

    std::cout << "Server shutdown complete." << std::endl;
}

void CGameServer::Run()
{
    MainLoop();
}

void CGameServer::MainLoop()
{
    const int FPS = 25; // 25 FPS
    const int FRAME_TIME = 1000 / FPS; // milliseconds

    auto last_update = std::chrono::steady_clock::now();

    while (m_bRunning)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update).count();

        if (elapsed >= FRAME_TIME)
        {
            Update();
            last_update = now;
        }

        // CPU'yu rahatlatmak için kısa bir uyku
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void CGameServer::Update()
{
    std::lock_guard<std::mutex> lock(m_mutexCharacters);

    // Tüm karakterleri güncelle
    for (auto& pair : m_mapCharacters)
    {
        if (pair.second)
        {
            pair.second->Update();
        }
    }
}

CCharacter* CGameServer::FindCharacter(DWORD player_id)
{
    std::lock_guard<std::mutex> lock(m_mutexCharacters);
    auto it = m_mapCharacters.find(player_id);
    return (it != m_mapCharacters.end()) ? it->second : nullptr;
}

CCharacter* CGameServer::FindCharacterByName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutexCharacters);
    for (auto& pair : m_mapCharacters)
    {
        if (pair.second && pair.second->GetName() == name)
            return pair.second;
    }
    return nullptr;
}

bool CGameServer::AddCharacter(CCharacter* ch)
{
    if (!ch)
        return false;

    std::lock_guard<std::mutex> lock(m_mutexCharacters);
    m_mapCharacters[ch->GetPlayerID()] = ch;

    std::cout << "Character added: " << ch->GetName()
              << " (ID: " << ch->GetPlayerID() << ")" << std::endl;

    return true;
}

void CGameServer::RemoveCharacter(DWORD player_id)
{
    std::lock_guard<std::mutex> lock(m_mutexCharacters);
    auto it = m_mapCharacters.find(player_id);

    if (it != m_mapCharacters.end())
    {
        if (it->second)
        {
            std::cout << "Character removed: " << it->second->GetName()
                      << " (ID: " << player_id << ")" << std::endl;
            delete it->second;
        }
        m_mapCharacters.erase(it);
    }
}

void CGameServer::SpawnCharacter(CCharacter* ch)
{
    if (!ch)
        return;

    std::cout << ch->GetName() << " has spawned at ("
              << ch->GetPosition().x << ", " << ch->GetPosition().y << ")" << std::endl;

    BroadcastCharacterInfo(ch);
}

void CGameServer::DespawnCharacter(CCharacter* ch)
{
    if (!ch)
        return;

    std::cout << ch->GetName() << " has despawned." << std::endl;
}

void CGameServer::BroadcastCharacterInfo(CCharacter* ch)
{
    if (!ch)
        return;

    // Gerçek implementasyonda burada network paketleri gönderilir
    std::cout << "[BROADCAST] Character Info: " << ch->GetName()
              << " Level: " << ch->GetLevel()
              << " HP: " << ch->GetHP() << "/" << ch->GetMaxHP() << std::endl;
}

void CGameServer::BroadcastMove(CCharacter* ch)
{
    if (!ch)
        return;

    std::cout << "[BROADCAST] " << ch->GetName() << " moved to ("
              << ch->GetPosition().x << ", " << ch->GetPosition().y << ")" << std::endl;
}

void CGameServer::BroadcastAttack(CCharacter* attacker, CCharacter* victim, DWORD damage)
{
    if (!attacker || !victim)
        return;

    std::cout << "[BROADCAST] " << attacker->GetName() << " attacked "
              << victim->GetName() << " for " << damage << " damage" << std::endl;
}

void CGameServer::BroadcastDeath(CCharacter* ch, CCharacter* killer)
{
    if (!ch)
        return;

    std::cout << "[BROADCAST] " << ch->GetName() << " has died";
    if (killer)
        std::cout << " (killed by " << killer->GetName() << ")";
    std::cout << std::endl;
}

void CGameServer::BroadcastChat(CCharacter* ch, const std::string& message)
{
    if (!ch)
        return;

    std::cout << "[CHAT] " << ch->GetName() << ": " << message << std::endl;
}

void CGameServer::ProcessMove(DWORD player_id, LONG x, LONG y)
{
    CCharacter* ch = FindCharacter(player_id);
    if (!ch)
        return;

    if (ch->MoveTo(x, y))
    {
        BroadcastMove(ch);
    }
}

void CGameServer::ProcessAttack(DWORD attacker_id, DWORD victim_id)
{
    CCharacter* attacker = FindCharacter(attacker_id);
    CCharacter* victim = FindCharacter(victim_id);

    if (!attacker || !victim)
        return;

    if (attacker->Attack(victim))
    {
        DWORD damage = attacker->CalculateDamage(victim);
        BroadcastAttack(attacker, victim, damage);

        if (victim->IsDead())
        {
            BroadcastDeath(victim, attacker);
        }
    }
}

void CGameServer::ProcessChat(DWORD player_id, const std::string& message)
{
    CCharacter* ch = FindCharacter(player_id);
    if (!ch)
        return;

    BroadcastChat(ch, message);
}

std::vector<CCharacter*> CGameServer::GetNearbyCharacters(const TPosition& pos, DWORD range)
{
    std::vector<CCharacter*> result;
    std::lock_guard<std::mutex> lock(m_mutexCharacters);

    for (auto& pair : m_mapCharacters)
    {
        if (!pair.second)
            continue;

        const TPosition& ch_pos = pair.second->GetPosition();
        LONG dx = ch_pos.x - pos.x;
        LONG dy = ch_pos.y - pos.y;
        DWORD dist = (DWORD)sqrt(dx * dx + dy * dy);

        if (dist <= range)
        {
            result.push_back(pair.second);
        }
    }

    return result;
}
