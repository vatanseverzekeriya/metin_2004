#ifndef __INC_NETWORK_MANAGER_H__
#define __INC_NETWORK_MANAGER_H__

#include "protocol.h"
#include "WebSocketServer.h"
#include "UDPServer.h"
#include "../game/Character.h"

// Hibrit network sistemi
// WebSocket: Login, chat, inventory gibi güvenilir iletişim
// UDP: Movement, combat gibi düşük gecikmeli iletişim

class CNetworkManager
{
public:
    static CNetworkManager& Instance();

    bool Initialize(int ws_port, int udp_port);
    void Start();
    void Stop();

    // Packet handler'ları kaydet
    void RegisterHandlers();

    // Oyuncu bağlantı yönetimi
    void OnPlayerLogin(DWORD session_id, DWORD account_id);
    void OnPlayerDisconnect(DWORD session_id);
    void OnPlayerSelectCharacter(DWORD session_id, DWORD character_id);

    // Karakter-Session mapping
    void RegisterCharacter(DWORD character_id, DWORD session_id);
    void UnregisterCharacter(DWORD character_id);
    DWORD GetSessionByCharacter(DWORD character_id);

    // Akıllı paket gönderme (otomatik protokol seçimi)
    bool SendToCharacter(DWORD character_id, BYTE packet_type, const char* data, WORD size);

    // Broadcast
    bool BroadcastToAll(BYTE packet_type, const char* data, WORD size);
    bool BroadcastToNearby(const TPosition& pos, DWORD range, BYTE packet_type, const char* data, WORD size);

    // Template helpers
    template<typename T>
    bool SendToCharacter(DWORD character_id, BYTE packet_type, const T& data)
    {
        char buffer[MAX_PACKET_SIZE];
        TPacketHeader header(packet_type);
        header.size = sizeof(TPacketHeader) + sizeof(T);
        header.timestamp = GetCurrentTime();

        memcpy(buffer, &header, sizeof(TPacketHeader));
        memcpy(buffer + sizeof(TPacketHeader), &data, sizeof(T));

        return SendToCharacter(character_id, packet_type, buffer, header.size);
    }

    template<typename T>
    bool BroadcastToAll(BYTE packet_type, const T& data)
    {
        char buffer[MAX_PACKET_SIZE];
        TPacketHeader header(packet_type);
        header.size = sizeof(TPacketHeader) + sizeof(T);
        header.timestamp = GetCurrentTime();

        memcpy(buffer, &header, sizeof(TPacketHeader));
        memcpy(buffer + sizeof(TPacketHeader), &data, sizeof(T));

        return BroadcastToAll(packet_type, buffer, header.size);
    }

    // Protokol seçimi
    bool ShouldUseUDP(BYTE packet_type) const;
    bool ShouldUseWebSocket(BYTE packet_type) const;

    // İstatistikler
    void GetNetworkStats(DWORD& ws_sessions, DWORD& udp_endpoints,
                        DWORD& total_players, float& avg_latency);

    DWORD GetCurrentTime() const;

private:
    CNetworkManager();
    ~CNetworkManager();

    CNetworkManager(const CNetworkManager&) = delete;
    CNetworkManager& operator=(const CNetworkManager&) = delete;

    // Packet handlers - WebSocket
    void HandleWSLogin(DWORD session_id, const TPacketHeader& header, const char* data, WORD size);
    void HandleWSCharacterSelect(DWORD session_id, const TPacketHeader& header, const char* data, WORD size);
    void HandleWSChat(DWORD session_id, const TPacketHeader& header, const char* data, WORD size);
    void HandleWSPing(DWORD session_id, const TPacketHeader& header, const char* data, WORD size);

    // Packet handlers - UDP
    void HandleUDPMove(const UDPEndpoint& endpoint, const TPacketHeader& header, const char* data, WORD size);
    void HandleUDPAttack(const UDPEndpoint& endpoint, const TPacketHeader& header, const char* data, WORD size);
    void HandleUDPSync(const UDPEndpoint& endpoint, const TPacketHeader& header, const char* data, WORD size);

    // Character-Session mapping
    std::map<DWORD, DWORD> m_mapCharacterToSession; // character_id -> session_id
    std::mutex m_mutexMapping;

    bool m_bRunning;
};

#endif // __INC_NETWORK_MANAGER_H__
