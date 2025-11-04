#include "../../include/network/UDPServer.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <sstream>

CUDPServer::CUDPServer()
    : m_socket(INVALID_SOCKET)
    , m_iPort(8081)
    , m_bRunning(false)
    , m_dwTotalPacketsIn(0)
    , m_dwTotalPacketsOut(0)
    , m_dwTotalBytesIn(0)
    , m_dwTotalBytesOut(0)
{
}

CUDPServer::~CUDPServer()
{
    Stop();
}

CUDPServer& CUDPServer::Instance()
{
    static CUDPServer instance;
    return instance;
}

bool CUDPServer::Initialize(int port)
{
    m_iPort = port;

#ifdef _WIN32
    // Windows Winsock başlatma
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
#endif

    // UDP socket oluştur
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create UDP socket" << std::endl;
        return false;
    }

    // Bind için adres yapısı
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_iPort);

    // Bind
    if (bind(m_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to bind UDP socket on port " << m_iPort << std::endl;
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        return false;
    }

    // Non-blocking mode (optional)
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(m_socket, FIONBIO, &mode);
#else
    int flags = fcntl(m_socket, F_GETFL, 0);
    fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

    std::cout << "==================================" << std::endl;
    std::cout << "  UDP Server Initialized         " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Port: " << m_iPort << std::endl;
    std::cout << "Socket: " << m_socket << std::endl;
    std::cout << "==================================" << std::endl;

    return true;
}

void CUDPServer::Start()
{
    if (m_bRunning)
        return;

    m_bRunning = true;

    // Thread'leri başlat
    m_threadReceive = std::thread(&CUDPServer::ReceiveThread, this);
    m_threadSend = std::thread(&CUDPServer::SendThread, this);

    std::cout << "UDP Server started successfully!" << std::endl;
}

void CUDPServer::Stop()
{
    if (!m_bRunning)
        return;

    std::cout << "Stopping UDP Server..." << std::endl;
    m_bRunning = false;

    // Thread'leri bekle
    if (m_threadReceive.joinable())
        m_threadReceive.join();
    if (m_threadSend.joinable())
        m_threadSend.join();

    // Socket'i kapat
    if (m_socket != INVALID_SOCKET)
    {
#ifdef _WIN32
        closesocket(m_socket);
        WSACleanup();
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET;
    }

    std::cout << "UDP Server stopped." << std::endl;
    std::cout << "Statistics:" << std::endl;
    std::cout << "  Packets IN:  " << m_dwTotalPacketsIn << " (" << m_dwTotalBytesIn << " bytes)" << std::endl;
    std::cout << "  Packets OUT: " << m_dwTotalPacketsOut << " (" << m_dwTotalBytesOut << " bytes)" << std::endl;
}

void CUDPServer::RegisterHandler(BYTE packet_type, UDPPacketHandler handler)
{
    std::lock_guard<std::mutex> lock(m_mutexHandlers);
    m_mapHandlers[packet_type] = handler;
    std::cout << "Registered UDP handler for packet type: " << (int)packet_type << std::endl;
}

bool CUDPServer::SendPacket(const sockaddr_in& addr, const char* data, WORD size)
{
    if (m_socket == INVALID_SOCKET || !m_bRunning)
        return false;

    // Send queue'ya ekle
    QueuedPacket packet;
    packet.address = addr;
    packet.size = size;
    memcpy(packet.data, data, size);

    {
        std::lock_guard<std::mutex> lock(m_mutexSendQueue);
        m_queueSend.push(packet);
    }

    return true;
}

bool CUDPServer::SendPacket(const UDPEndpoint& endpoint, const char* data, WORD size)
{
    return SendPacket(endpoint.address, data, size);
}

bool CUDPServer::BroadcastPacket(const char* data, WORD size)
{
    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    for (const auto& endpoint : m_vecEndpoints)
    {
        SendPacket(endpoint.address, data, size);
    }

    return true;
}

UDPEndpoint* CUDPServer::FindEndpoint(const sockaddr_in& addr)
{
    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    for (auto& endpoint : m_vecEndpoints)
    {
        if (CompareAddress(endpoint.address, addr))
            return &endpoint;
    }

    return nullptr;
}

UDPEndpoint* CUDPServer::FindEndpointByCharacter(DWORD character_id)
{
    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    for (auto& endpoint : m_vecEndpoints)
    {
        if (endpoint.character_id == character_id)
            return &endpoint;
    }

    return nullptr;
}

void CUDPServer::RegisterEndpoint(const sockaddr_in& addr, DWORD session_id, DWORD character_id)
{
    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    // Zaten var mı kontrol et
    for (auto& endpoint : m_vecEndpoints)
    {
        if (CompareAddress(endpoint.address, addr))
        {
            endpoint.session_id = session_id;
            endpoint.character_id = character_id;
            endpoint.last_activity = GetCurrentTime();
            std::cout << "Updated UDP endpoint: " << AddressToString(addr)
                      << " CharID: " << character_id << std::endl;
            return;
        }
    }

    // Yeni endpoint ekle
    UDPEndpoint endpoint;
    endpoint.address = addr;
    endpoint.session_id = session_id;
    endpoint.character_id = character_id;
    endpoint.last_activity = GetCurrentTime();

    m_vecEndpoints.push_back(endpoint);

    std::cout << "Registered UDP endpoint: " << AddressToString(addr)
              << " CharID: " << character_id << std::endl;
}

void CUDPServer::RemoveEndpoint(const sockaddr_in& addr)
{
    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    auto it = std::remove_if(m_vecEndpoints.begin(), m_vecEndpoints.end(),
        [&addr, this](const UDPEndpoint& ep) {
            return CompareAddress(ep.address, addr);
        });

    if (it != m_vecEndpoints.end())
    {
        std::cout << "Removed UDP endpoint: " << AddressToString(addr) << std::endl;
        m_vecEndpoints.erase(it, m_vecEndpoints.end());
    }
}

void CUDPServer::UpdateActivity(const sockaddr_in& addr)
{
    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    for (auto& endpoint : m_vecEndpoints)
    {
        if (CompareAddress(endpoint.address, addr))
        {
            endpoint.last_activity = GetCurrentTime();
            endpoint.packet_count++;
            break;
        }
    }
}

DWORD CUDPServer::GetEndpointCount() const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutexEndpoints));
    return m_vecEndpoints.size();
}

void CUDPServer::GetStatistics(DWORD& total_packets_in, DWORD& total_packets_out,
                               DWORD& total_bytes_in, DWORD& total_bytes_out)
{
    total_packets_in = m_dwTotalPacketsIn;
    total_packets_out = m_dwTotalPacketsOut;
    total_bytes_in = m_dwTotalBytesIn;
    total_bytes_out = m_dwTotalBytesOut;
}

DWORD CUDPServer::GetCurrentTime() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void CUDPServer::ReceiveThread()
{
    std::cout << "UDP Receive thread started" << std::endl;

    char buffer[MAX_PACKET_SIZE];
    sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (m_bRunning)
    {
        // Paket al
        int received = recvfrom(m_socket, buffer, MAX_PACKET_SIZE, 0,
                               (sockaddr*)&from_addr, &from_len);

        if (received > 0)
        {
            m_dwTotalPacketsIn++;
            m_dwTotalBytesIn += received;

            // Activity güncelle
            UpdateActivity(from_addr);

            // Paketi işle
            ProcessPacket(from_addr, buffer, received);
        }
        else if (received == 0)
        {
            // Connection closed (UDP'de normalde olmaz)
            break;
        }
        else
        {
            // Error veya WOULDBLOCK
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN)
#endif
            {
                // Gerçek hata
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        // Timeout kontrolü
        static DWORD last_check = 0;
        DWORD now = GetCurrentTime();
        if (now - last_check > 5000) // Her 5 saniyede bir
        {
            CheckTimeouts();
            last_check = now;
        }
    }

    std::cout << "UDP Receive thread stopped" << std::endl;
}

void CUDPServer::SendThread()
{
    std::cout << "UDP Send thread started" << std::endl;

    while (m_bRunning)
    {
        QueuedPacket packet;
        bool has_packet = false;

        {
            std::lock_guard<std::mutex> lock(m_mutexSendQueue);
            if (!m_queueSend.empty())
            {
                packet = m_queueSend.front();
                m_queueSend.pop();
                has_packet = true;
            }
        }

        if (has_packet)
        {
            // Paketi gönder
            int sent = sendto(m_socket, packet.data, packet.size, 0,
                            (sockaddr*)&packet.address, sizeof(packet.address));

            if (sent > 0)
            {
                m_dwTotalPacketsOut++;
                m_dwTotalBytesOut += sent;
            }
        }
        else
        {
            // Queue boş, biraz bekle
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "UDP Send thread stopped" << std::endl;
}

void CUDPServer::ProcessPacket(const sockaddr_in& from, const char* data, WORD size)
{
    if (size < sizeof(TPacketHeader))
        return;

    TPacketHeader header;
    memcpy(&header, data, sizeof(TPacketHeader));

    // Endpoint bul
    UDPEndpoint* endpoint = FindEndpoint(from);
    if (!endpoint)
    {
        // Yeni endpoint, henüz register olmamış
        // Login handshake'i bekliyoruz
        std::cout << "Packet from unregistered endpoint: " << AddressToString(from) << std::endl;
    }

    // Handler bul ve çalıştır
    std::lock_guard<std::mutex> lock(m_mutexHandlers);
    auto it = m_mapHandlers.find(header.type);
    if (it != m_mapHandlers.end())
    {
        UDPEndpoint temp_endpoint;
        if (endpoint)
            temp_endpoint = *endpoint;
        else
            temp_endpoint.address = from;

        it->second(temp_endpoint, header, data + sizeof(TPacketHeader), size - sizeof(TPacketHeader));
    }
}

void CUDPServer::CheckTimeouts()
{
    const DWORD TIMEOUT_MS = 30000; // 30 saniye
    DWORD current_time = GetCurrentTime();

    std::lock_guard<std::mutex> lock(m_mutexEndpoints);

    auto it = std::remove_if(m_vecEndpoints.begin(), m_vecEndpoints.end(),
        [current_time, TIMEOUT_MS, this](const UDPEndpoint& ep) {
            if (current_time - ep.last_activity > TIMEOUT_MS)
            {
                std::cout << "UDP endpoint timeout: " << AddressToString(ep.address) << std::endl;
                return true;
            }
            return false;
        });

    m_vecEndpoints.erase(it, m_vecEndpoints.end());
}

std::string CUDPServer::AddressToString(const sockaddr_in& addr)
{
    std::stringstream ss;
    ss << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
    return ss.str();
}

bool CUDPServer::CompareAddress(const sockaddr_in& a, const sockaddr_in& b)
{
    return (a.sin_addr.s_addr == b.sin_addr.s_addr) &&
           (a.sin_port == b.sin_port);
}
