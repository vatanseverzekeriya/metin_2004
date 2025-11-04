#ifndef __INC_NETWORK_UDP_SERVER_H__
#define __INC_NETWORK_UDP_SERVER_H__

#include "protocol.h"
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <vector>
#include <queue>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

// UDP client endpoint bilgisi
struct UDPEndpoint
{
    sockaddr_in address;
    DWORD session_id;
    DWORD character_id;
    DWORD last_activity;
    DWORD packet_count;

    // Packet loss tracking
    DWORD last_sequence;
    DWORD lost_packets;

    // Latency tracking
    DWORD last_ping_time;
    DWORD latency_ms;

    UDPEndpoint()
        : session_id(0), character_id(0), last_activity(0)
        , packet_count(0), last_sequence(0), lost_packets(0)
        , last_ping_time(0), latency_ms(0)
    {
        memset(&address, 0, sizeof(address));
    }
};

// UDP paket handler callback
typedef std::function<void(const UDPEndpoint&, const TPacketHeader&, const char* data, WORD size)> UDPPacketHandler;

class CUDPServer
{
public:
    static CUDPServer& Instance();

    // Sunucu yönetimi
    bool Initialize(int port);
    void Start();
    void Stop();
    bool IsRunning() const { return m_bRunning; }

    // Paket handler kaydetme
    void RegisterHandler(BYTE packet_type, UDPPacketHandler handler);

    // Paket gönderme - Adrese göre
    bool SendPacket(const sockaddr_in& addr, const char* data, WORD size);
    bool SendPacket(const UDPEndpoint& endpoint, const char* data, WORD size);

    // Template helper
    template<typename T>
    bool SendPacket(const UDPEndpoint& endpoint, BYTE type, const T& packet_data, DWORD sequence = 0)
    {
        char buffer[MAX_PACKET_SIZE];
        TPacketHeader header(type);
        header.size = sizeof(TPacketHeader) + sizeof(T);
        header.sequence = sequence;
        header.timestamp = GetCurrentTime();

        memcpy(buffer, &header, sizeof(TPacketHeader));
        memcpy(buffer + sizeof(TPacketHeader), &packet_data, sizeof(T));

        return SendPacket(endpoint, buffer, header.size);
    }

    // Broadcast
    bool BroadcastPacket(const char* data, WORD size);

    template<typename T>
    bool BroadcastPacket(BYTE type, const T& packet_data)
    {
        char buffer[MAX_PACKET_SIZE];
        TPacketHeader header(type);
        header.size = sizeof(TPacketHeader) + sizeof(T);
        header.timestamp = GetCurrentTime();

        memcpy(buffer, &header, sizeof(TPacketHeader));
        memcpy(buffer + sizeof(TPacketHeader), &packet_data, sizeof(T));

        return BroadcastPacket(buffer, header.size);
    }

    // Endpoint yönetimi
    UDPEndpoint* FindEndpoint(const sockaddr_in& addr);
    UDPEndpoint* FindEndpointByCharacter(DWORD character_id);
    void RegisterEndpoint(const sockaddr_in& addr, DWORD session_id, DWORD character_id);
    void RemoveEndpoint(const sockaddr_in& addr);
    void UpdateActivity(const sockaddr_in& addr);

    // İstatistikler
    DWORD GetEndpointCount() const;
    void GetStatistics(DWORD& total_packets_in, DWORD& total_packets_out,
                      DWORD& total_bytes_in, DWORD& total_bytes_out);

    DWORD GetCurrentTime() const;

private:
    CUDPServer();
    ~CUDPServer();

    CUDPServer(const CUDPServer&) = delete;
    CUDPServer& operator=(const CUDPServer&) = delete;

    void ReceiveThread();
    void SendThread();
    void ProcessPacket(const sockaddr_in& from, const char* data, WORD size);
    void CheckTimeouts();
    std::string AddressToString(const sockaddr_in& addr);
    bool CompareAddress(const sockaddr_in& a, const sockaddr_in& b);

    // Socket
    SOCKET m_socket;
    int m_iPort;
    bool m_bRunning;

    // Endpoints
    std::vector<UDPEndpoint> m_vecEndpoints;
    std::mutex m_mutexEndpoints;

    // Packet handlers
    std::map<BYTE, UDPPacketHandler> m_mapHandlers;
    std::mutex m_mutexHandlers;

    // Send queue
    struct QueuedPacket
    {
        sockaddr_in address;
        char data[MAX_PACKET_SIZE];
        WORD size;
    };
    std::queue<QueuedPacket> m_queueSend;
    std::mutex m_mutexSendQueue;

    // Threads
    std::thread m_threadReceive;
    std::thread m_threadSend;

    // İstatistikler
    DWORD m_dwTotalPacketsIn;
    DWORD m_dwTotalPacketsOut;
    DWORD m_dwTotalBytesIn;
    DWORD m_dwTotalBytesOut;
};

#endif // __INC_NETWORK_UDP_SERVER_H__
