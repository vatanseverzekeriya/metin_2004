#ifndef __INC_NETWORK_WEBSOCKET_SERVER_H__
#define __INC_NETWORK_WEBSOCKET_SERVER_H__

#include "protocol.h"
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <memory>
#include <vector>

// Boost.Beast için forward declarations
namespace boost {
    namespace asio {
        class io_context;
    }
}

// WebSocket client bağlantı bilgisi
struct WebSocketSession
{
    DWORD session_id;
    DWORD account_id;
    DWORD character_id;
    std::string remote_ip;
    DWORD connect_time;
    DWORD last_activity;
    DWORD sequence_number;
    bool authenticated;

    WebSocketSession()
        : session_id(0), account_id(0), character_id(0)
        , connect_time(0), last_activity(0), sequence_number(0)
        , authenticated(false) {}
};

// Paket handler callback tipi
typedef std::function<void(DWORD session_id, const TPacketHeader&, const char* data, WORD size)> PacketHandler;

class CWebSocketServer
{
public:
    static CWebSocketServer& Instance();

    // Sunucu yönetimi
    bool Initialize(int port);
    void Start();
    void Stop();
    bool IsRunning() const { return m_bRunning; }

    // Paket handler kaydetme
    void RegisterHandler(BYTE packet_type, PacketHandler handler);

    // Paket gönderme
    bool SendPacket(DWORD session_id, const char* data, WORD size);
    bool SendPacketToAll(const char* data, WORD size);
    bool SendPacketToAllExcept(DWORD exclude_session_id, const char* data, WORD size);

    // Template helper - Kolay paket gönderme
    template<typename T>
    bool SendPacket(DWORD session_id, BYTE type, const T& packet_data)
    {
        char buffer[MAX_PACKET_SIZE];
        TPacketHeader header(type);
        header.size = sizeof(TPacketHeader) + sizeof(T);
        header.sequence = GetNextSequence(session_id);
        header.timestamp = GetCurrentTime();

        memcpy(buffer, &header, sizeof(TPacketHeader));
        memcpy(buffer + sizeof(TPacketHeader), &packet_data, sizeof(T));

        return SendPacket(session_id, buffer, header.size);
    }

    template<typename T>
    bool BroadcastPacket(BYTE type, const T& packet_data)
    {
        char buffer[MAX_PACKET_SIZE];
        TPacketHeader header(type);
        header.size = sizeof(TPacketHeader) + sizeof(T);
        header.timestamp = GetCurrentTime();

        memcpy(buffer, &header, sizeof(TPacketHeader));
        memcpy(buffer + sizeof(TPacketHeader), &packet_data, sizeof(T));

        return SendPacketToAll(buffer, header.size);
    }

    // Session yönetimi
    WebSocketSession* GetSession(DWORD session_id);
    void RemoveSession(DWORD session_id);
    DWORD GetSessionCount() const;

    // Yardımcı fonksiyonlar
    DWORD GetCurrentTime() const;
    DWORD GetNextSequence(DWORD session_id);

private:
    CWebSocketServer();
    ~CWebSocketServer();

    CWebSocketServer(const CWebSocketServer&) = delete;
    CWebSocketServer& operator=(const CWebSocketServer&) = delete;

    void WorkerThread();
    void ProcessPacket(DWORD session_id, const char* data, WORD size);
    void OnConnect(DWORD session_id, const std::string& remote_ip);
    void OnDisconnect(DWORD session_id);
    void CheckTimeouts();

    // Sunucu durumu
    bool m_bRunning;
    int m_iPort;

    // Boost.Asio (gerçek implementasyon için)
    // std::unique_ptr<boost::asio::io_context> m_pIOContext;

    // Session yönetimi
    std::map<DWORD, WebSocketSession> m_mapSessions;
    std::mutex m_mutexSessions;
    DWORD m_dwNextSessionID;

    // Packet handlers
    std::map<BYTE, PacketHandler> m_mapHandlers;
    std::mutex m_mutexHandlers;

    // Worker thread
    std::vector<std::thread> m_vecWorkerThreads;

    // İstatistikler
    DWORD m_dwTotalConnections;
    DWORD m_dwTotalPacketsReceived;
    DWORD m_dwTotalPacketsSent;
};

#endif // __INC_NETWORK_WEBSOCKET_SERVER_H__
