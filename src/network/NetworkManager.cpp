#include "../../include/network/NetworkManager.h"
#include "../../include/game/GameServer.h"
#include "../../include/db/DBManager.h"
#include <iostream>
#include <chrono>

CNetworkManager::CNetworkManager()
    : m_bRunning(false)
{
}

CNetworkManager::~CNetworkManager()
{
    Stop();
}

CNetworkManager& CNetworkManager::Instance()
{
    static CNetworkManager instance;
    return instance;
}

bool CNetworkManager::Initialize(int ws_port, int udp_port)
{
    std::cout << "==================================" << std::endl;
    std::cout << "  Network Manager Initializing   " << std::endl;
    std::cout << "==================================" << std::endl;

    // WebSocket sunucusunu başlat
    if (!CWebSocketServer::Instance().Initialize(ws_port))
    {
        std::cerr << "Failed to initialize WebSocket server!" << std::endl;
        return false;
    }

    // UDP sunucusunu başlat
    if (!CUDPServer::Instance().Initialize(udp_port))
    {
        std::cerr << "Failed to initialize UDP server!" << std::endl;
        return false;
    }

    // Handler'ları kaydet
    RegisterHandlers();

    std::cout << "Network Manager initialized successfully!" << std::endl;
    std::cout << "  WebSocket Port: " << ws_port << std::endl;
    std::cout << "  UDP Port: " << udp_port << std::endl;
    std::cout << "==================================" << std::endl;

    return true;
}

void CNetworkManager::Start()
{
    if (m_bRunning)
        return;

    m_bRunning = true;

    // Sunucuları başlat
    CWebSocketServer::Instance().Start();
    CUDPServer::Instance().Start();

    std::cout << "Network Manager started!" << std::endl;
}

void CNetworkManager::Stop()
{
    if (!m_bRunning)
        return;

    std::cout << "Stopping Network Manager..." << std::endl;
    m_bRunning = false;

    // Sunucuları durdur
    CWebSocketServer::Instance().Stop();
    CUDPServer::Instance().Stop();

    std::cout << "Network Manager stopped." << std::endl;
}

void CNetworkManager::RegisterHandlers()
{
    // WebSocket handlers
    CWebSocketServer::Instance().RegisterHandler(HEADER_CG_LOGIN,
        [this](DWORD sid, const TPacketHeader& h, const char* d, WORD s) {
            HandleWSLogin(sid, h, d, s);
        });

    CWebSocketServer::Instance().RegisterHandler(HEADER_CG_CHARACTER_SELECT,
        [this](DWORD sid, const TPacketHeader& h, const char* d, WORD s) {
            HandleWSCharacterSelect(sid, h, d, s);
        });

    CWebSocketServer::Instance().RegisterHandler(HEADER_CG_CHAT,
        [this](DWORD sid, const TPacketHeader& h, const char* d, WORD s) {
            HandleWSChat(sid, h, d, s);
        });

    CWebSocketServer::Instance().RegisterHandler(HEADER_CG_PING,
        [this](DWORD sid, const TPacketHeader& h, const char* d, WORD s) {
            HandleWSPing(sid, h, d, s);
        });

    // UDP handlers
    CUDPServer::Instance().RegisterHandler(HEADER_CG_MOVE,
        [this](const UDPEndpoint& ep, const TPacketHeader& h, const char* d, WORD s) {
            HandleUDPMove(ep, h, d, s);
        });

    CUDPServer::Instance().RegisterHandler(HEADER_CG_ATTACK,
        [this](const UDPEndpoint& ep, const TPacketHeader& h, const char* d, WORD s) {
            HandleUDPAttack(ep, h, d, s);
        });

    CUDPServer::Instance().RegisterHandler(HEADER_CG_SYNC_POSITION,
        [this](const UDPEndpoint& ep, const TPacketHeader& h, const char* d, WORD s) {
            HandleUDPSync(ep, h, d, s);
        });

    std::cout << "Registered all packet handlers" << std::endl;
}

void CNetworkManager::OnPlayerLogin(DWORD session_id, DWORD account_id)
{
    WebSocketSession* session = CWebSocketServer::Instance().GetSession(session_id);
    if (session)
    {
        session->account_id = account_id;
        session->authenticated = true;
        std::cout << "Player logged in [Session: " << session_id << ", Account: " << account_id << "]" << std::endl;
    }
}

void CNetworkManager::OnPlayerDisconnect(DWORD session_id)
{
    WebSocketSession* session = CWebSocketServer::Instance().GetSession(session_id);
    if (session && session->character_id > 0)
    {
        UnregisterCharacter(session->character_id);

        // Karakteri oyundan çıkar
        CCharacter* ch = CGameServer::Instance().FindCharacter(session->character_id);
        if (ch)
        {
            ch->Save();
            CGameServer::Instance().DespawnCharacter(ch);
            CGameServer::Instance().RemoveCharacter(session->character_id);
        }
    }

    std::cout << "Player disconnected [Session: " << session_id << "]" << std::endl;
}

void CNetworkManager::OnPlayerSelectCharacter(DWORD session_id, DWORD character_id)
{
    WebSocketSession* session = CWebSocketServer::Instance().GetSession(session_id);
    if (session)
    {
        session->character_id = character_id;
        RegisterCharacter(character_id, session_id);
        std::cout << "Player selected character [Session: " << session_id
                  << ", Character: " << character_id << "]" << std::endl;
    }
}

void CNetworkManager::RegisterCharacter(DWORD character_id, DWORD session_id)
{
    std::lock_guard<std::mutex> lock(m_mutexMapping);
    m_mapCharacterToSession[character_id] = session_id;
}

void CNetworkManager::UnregisterCharacter(DWORD character_id)
{
    std::lock_guard<std::mutex> lock(m_mutexMapping);
    m_mapCharacterToSession.erase(character_id);
}

DWORD CNetworkManager::GetSessionByCharacter(DWORD character_id)
{
    std::lock_guard<std::mutex> lock(m_mutexMapping);
    auto it = m_mapCharacterToSession.find(character_id);
    return (it != m_mapCharacterToSession.end()) ? it->second : 0;
}

bool CNetworkManager::SendToCharacter(DWORD character_id, BYTE packet_type, const char* data, WORD size)
{
    DWORD session_id = GetSessionByCharacter(character_id);
    if (session_id == 0)
        return false;

    // Protokol seçimi
    if (ShouldUseUDP(packet_type))
    {
        // UDP ile gönder
        UDPEndpoint* endpoint = CUDPServer::Instance().FindEndpointByCharacter(character_id);
        if (endpoint)
        {
            return CUDPServer::Instance().SendPacket(*endpoint, data, size);
        }
        else
        {
            // UDP endpoint yok, WebSocket'e düş
            return CWebSocketServer::Instance().SendPacket(session_id, data, size);
        }
    }
    else
    {
        // WebSocket ile gönder
        return CWebSocketServer::Instance().SendPacket(session_id, data, size);
    }
}

bool CNetworkManager::BroadcastToAll(BYTE packet_type, const char* data, WORD size)
{
    if (ShouldUseUDP(packet_type))
    {
        return CUDPServer::Instance().BroadcastPacket(data, size);
    }
    else
    {
        return CWebSocketServer::Instance().SendPacketToAll(data, size);
    }
}

bool CNetworkManager::BroadcastToNearby(const TPosition& pos, DWORD range,
                                       BYTE packet_type, const char* data, WORD size)
{
    // Yakındaki karakterleri bul
    std::vector<CCharacter*> nearby = CGameServer::Instance().GetNearbyCharacters(pos, range);

    for (CCharacter* ch : nearby)
    {
        SendToCharacter(ch->GetPlayerID(), packet_type, data, size);
    }

    return true;
}

bool CNetworkManager::ShouldUseUDP(BYTE packet_type) const
{
    // Düşük gecikme gerektiren paketler UDP kullanır
    switch (packet_type)
    {
    case HEADER_CG_MOVE:
    case HEADER_GC_MOVE:
    case HEADER_CG_ATTACK:
    case HEADER_GC_ATTACK:
    case HEADER_GC_DAMAGE:
    case HEADER_CG_SYNC_POSITION:
    case HEADER_GC_SYNC_POSITION:
    case HEADER_CG_HEARTBEAT:
        return true;
    default:
        return false;
    }
}

bool CNetworkManager::ShouldUseWebSocket(BYTE packet_type) const
{
    return !ShouldUseUDP(packet_type);
}

void CNetworkManager::GetNetworkStats(DWORD& ws_sessions, DWORD& udp_endpoints,
                                     DWORD& total_players, float& avg_latency)
{
    ws_sessions = CWebSocketServer::Instance().GetSessionCount();
    udp_endpoints = CUDPServer::Instance().GetEndpointCount();

    std::lock_guard<std::mutex> lock(m_mutexMapping);
    total_players = m_mapCharacterToSession.size();

    // TODO: Latency hesaplama
    avg_latency = 0.0f;
}

DWORD CNetworkManager::GetCurrentTime() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// ========== PACKET HANDLERS ==========

void CNetworkManager::HandleWSLogin(DWORD session_id, const TPacketHeader& header,
                                   const char* data, WORD size)
{
    if (size < sizeof(TPacketCGLogin))
        return;

    TPacketCGLogin login;
    memcpy(&login, data, sizeof(TPacketCGLogin));

    std::cout << "Login request from session " << session_id
              << " User: " << login.username << std::endl;

    // Veritabanı kontrolü
    if (CDBManager::Instance().CheckAccount(login.username, login.password))
    {
        DWORD account_id = CDBManager::Instance().GetAccountID(login.username);
        OnPlayerLogin(session_id, account_id);

        // Success paketi gönder
        TPacketGCLoginSuccess success;
        success.account_id = account_id;
        success.character_count = 1; // TODO: Gerçek karakter sayısı
        strcpy(success.session_token, "TOKEN_12345");

        CWebSocketServer::Instance().SendPacket(session_id, HEADER_GC_LOGIN_SUCCESS, success);
    }
    else
    {
        // Failure paketi gönder
        TPacketGCLoginFailure failure;
        failure.error_code = 1; // Wrong password
        strcpy(failure.message, "Invalid username or password");

        CWebSocketServer::Instance().SendPacket(session_id, HEADER_GC_LOGIN_FAILURE, failure);
    }
}

void CNetworkManager::HandleWSCharacterSelect(DWORD session_id, const TPacketHeader& header,
                                             const char* data, WORD size)
{
    if (size < sizeof(TPacketCGCharacterSelect))
        return;

    TPacketCGCharacterSelect select;
    memcpy(&select, data, sizeof(TPacketCGCharacterSelect));

    std::cout << "Character select: " << select.character_id << " by session " << session_id << std::endl;

    // Karakteri yükle
    CCharacter* ch = new CCharacter();
    if (ch->Initialize(select.character_id))
    {
        CGameServer::Instance().AddCharacter(ch);
        CGameServer::Instance().SpawnCharacter(ch);

        OnPlayerSelectCharacter(session_id, select.character_id);

        // Karakter bilgilerini gönder
        TPacketGCCharacterInfo info;
        info.id = ch->GetPlayerID();
        strncpy(info.name, ch->GetName().c_str(), sizeof(info.name) - 1);
        info.job = ch->GetJob();
        info.level = ch->GetLevel();
        info.hp = ch->GetHP();
        info.max_hp = ch->GetMaxHP();
        info.sp = ch->GetSP();
        info.max_sp = ch->GetMaxSP();
        info.x = ch->GetPosition().x;
        info.y = ch->GetPosition().y;
        info.dir = 0;
        info.attack = ch->GetAttack();
        info.defense = ch->GetDefense();

        CWebSocketServer::Instance().SendPacket(session_id, HEADER_GC_CHARACTER_INFO, info);
    }
    else
    {
        delete ch;

        TPacketGCError error;
        error.error_code = 100;
        strcpy(error.message, "Failed to load character");

        CWebSocketServer::Instance().SendPacket(session_id, HEADER_GC_ERROR, error);
    }
}

void CNetworkManager::HandleWSChat(DWORD session_id, const TPacketHeader& header,
                                  const char* data, WORD size)
{
    if (size < sizeof(TPacketCGChat))
        return;

    TPacketCGChat chat;
    memcpy(&chat, data, sizeof(TPacketCGChat));

    WebSocketSession* session = CWebSocketServer::Instance().GetSession(session_id);
    if (!session || session->character_id == 0)
        return;

    CCharacter* ch = CGameServer::Instance().FindCharacter(session->character_id);
    if (!ch)
        return;

    std::cout << "[CHAT] " << ch->GetName() << ": " << chat.message << std::endl;

    // Broadcast chat
    TPacketGCChat gc_chat;
    gc_chat.sender_id = ch->GetPlayerID();
    strncpy(gc_chat.sender_name, ch->GetName().c_str(), sizeof(gc_chat.sender_name) - 1);
    gc_chat.type = chat.type;
    strncpy(gc_chat.message, chat.message, sizeof(gc_chat.message) - 1);

    CWebSocketServer::Instance().BroadcastPacket(HEADER_GC_CHAT, gc_chat);
}

void CNetworkManager::HandleWSPing(DWORD session_id, const TPacketHeader& header,
                                  const char* data, WORD size)
{
    if (size < sizeof(TPacketCGPing))
        return;

    TPacketCGPing ping;
    memcpy(&ping, data, sizeof(TPacketCGPing));

    // Pong gönder
    TPacketGCPong pong;
    pong.client_time = ping.client_time;
    pong.server_time = GetCurrentTime();

    CWebSocketServer::Instance().SendPacket(session_id, HEADER_GC_PONG, pong);
}

void CNetworkManager::HandleUDPMove(const UDPEndpoint& endpoint, const TPacketHeader& header,
                                   const char* data, WORD size)
{
    if (size < sizeof(TPacketCGMove))
        return;

    TPacketCGMove move;
    memcpy(&move, data, sizeof(TPacketCGMove));

    if (endpoint.character_id == 0)
        return;

    CCharacter* ch = CGameServer::Instance().FindCharacter(endpoint.character_id);
    if (!ch)
        return;

    // Pozisyonu güncelle
    ch->MoveTo(move.x, move.y);

    // Broadcast move
    TPacketGCMove gc_move;
    gc_move.id = ch->GetPlayerID();
    gc_move.x = move.x;
    gc_move.y = move.y;
    gc_move.dir = move.dir;
    gc_move.duration = 500; // 500ms

    // Yakındakilere broadcast
    BroadcastToNearby(ch->GetPosition(), 3000, HEADER_GC_MOVE,
                     (const char*)&gc_move, sizeof(gc_move));
}

void CNetworkManager::HandleUDPAttack(const UDPEndpoint& endpoint, const TPacketHeader& header,
                                     const char* data, WORD size)
{
    if (size < sizeof(TPacketCGAttack))
        return;

    TPacketCGAttack attack;
    memcpy(&attack, data, sizeof(TPacketCGAttack));

    if (endpoint.character_id == 0)
        return;

    CCharacter* attacker = CGameServer::Instance().FindCharacter(endpoint.character_id);
    CCharacter* victim = CGameServer::Instance().FindCharacter(attack.target_id);

    if (!attacker || !victim)
        return;

    // Saldırı işle
    if (attacker->Attack(victim))
    {
        DWORD damage = attacker->CalculateDamage(victim);

        // Damage broadcast
        TPacketGCDamage gc_damage;
        gc_damage.attacker_id = attacker->GetPlayerID();
        gc_damage.victim_id = victim->GetPlayerID();
        gc_damage.damage = damage;
        gc_damage.is_critical = 0;

        BroadcastToNearby(attacker->GetPosition(), 3000, HEADER_GC_DAMAGE,
                         (const char*)&gc_damage, sizeof(gc_damage));

        // Ölüm kontrolü
        if (victim->IsDead())
        {
            TPacketGCDeath gc_death;
            gc_death.victim_id = victim->GetPlayerID();
            gc_death.killer_id = attacker->GetPlayerID();

            BroadcastToNearby(victim->GetPosition(), 5000, HEADER_GC_DEATH,
                             (const char*)&gc_death, sizeof(gc_death));
        }
    }
}

void CNetworkManager::HandleUDPSync(const UDPEndpoint& endpoint, const TPacketHeader& header,
                                   const char* data, WORD size)
{
    if (size < sizeof(TPacketCGSyncPosition))
        return;

    TPacketCGSyncPosition sync;
    memcpy(&sync, data, sizeof(TPacketCGSyncPosition));

    if (endpoint.character_id == 0)
        return;

    CCharacter* ch = CGameServer::Instance().FindCharacter(endpoint.character_id);
    if (!ch)
        return;

    // Pozisyon senkronizasyonu
    ch->SetPosition(sync.x, sync.y);
}
