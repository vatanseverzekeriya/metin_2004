#include "../../include/network/WebSocketServer.h"
#include <iostream>
#include <chrono>
#include <algorithm>

CWebSocketServer::CWebSocketServer()
    : m_bRunning(false)
    , m_iPort(8080)
    , m_dwNextSessionID(1)
    , m_dwTotalConnections(0)
    , m_dwTotalPacketsReceived(0)
    , m_dwTotalPacketsSent(0)
{
}

CWebSocketServer::~CWebSocketServer()
{
    Stop();
}

CWebSocketServer& CWebSocketServer::Instance()
{
    static CWebSocketServer instance;
    return instance;
}

bool CWebSocketServer::Initialize(int port)
{
    m_iPort = port;

    std::cout << "==================================" << std::endl;
    std::cout << "  WebSocket Server Initializing  " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Port: " << m_iPort << std::endl;
    std::cout << "Protocol Version: " << (int)PROTOCOL_VERSION << std::endl;
    std::cout << "==================================" << std::endl;

    return true;
}

void CWebSocketServer::Start()
{
    if (m_bRunning)
        return;

    m_bRunning = true;

    // Worker thread'leri başlat
    int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) thread_count = 4;

    std::cout << "Starting " << thread_count << " worker threads..." << std::endl;

    for (int i = 0; i < thread_count; ++i)
    {
        m_vecWorkerThreads.emplace_back(&CWebSocketServer::WorkerThread, this);
    }

    std::cout << "WebSocket Server started successfully!" << std::endl;
}

void CWebSocketServer::Stop()
{
    if (!m_bRunning)
        return;

    std::cout << "Stopping WebSocket Server..." << std::endl;
    m_bRunning = false;

    // Worker thread'leri bekle
    for (auto& thread : m_vecWorkerThreads)
    {
        if (thread.joinable())
            thread.join();
    }
    m_vecWorkerThreads.clear();

    // Tüm session'ları temizle
    std::lock_guard<std::mutex> lock(m_mutexSessions);
    m_mapSessions.clear();

    std::cout << "WebSocket Server stopped." << std::endl;
    std::cout << "Statistics:" << std::endl;
    std::cout << "  Total Connections: " << m_dwTotalConnections << std::endl;
    std::cout << "  Packets Received: " << m_dwTotalPacketsReceived << std::endl;
    std::cout << "  Packets Sent: " << m_dwTotalPacketsSent << std::endl;
}

void CWebSocketServer::RegisterHandler(BYTE packet_type, PacketHandler handler)
{
    std::lock_guard<std::mutex> lock(m_mutexHandlers);
    m_mapHandlers[packet_type] = handler;
    std::cout << "Registered handler for packet type: " << (int)packet_type << std::endl;
}

bool CWebSocketServer::SendPacket(DWORD session_id, const char* data, WORD size)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);

    auto it = m_mapSessions.find(session_id);
    if (it == m_mapSessions.end())
        return false;

    // Gerçek implementasyonda WebSocket üzerinden gönderilir
    // Şimdilik sadece simüle ediyoruz
    m_dwTotalPacketsSent++;

    // Session activity güncelle
    it->second.last_activity = GetCurrentTime();

    return true;
}

bool CWebSocketServer::SendPacketToAll(const char* data, WORD size)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);

    for (auto& pair : m_mapSessions)
    {
        // Her session'a gönder (lock içindeyiz, dikkatli ol)
        m_dwTotalPacketsSent++;
    }

    return true;
}

bool CWebSocketServer::SendPacketToAllExcept(DWORD exclude_session_id, const char* data, WORD size)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);

    for (auto& pair : m_mapSessions)
    {
        if (pair.first != exclude_session_id)
        {
            m_dwTotalPacketsSent++;
        }
    }

    return true;
}

WebSocketSession* CWebSocketServer::GetSession(DWORD session_id)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);
    auto it = m_mapSessions.find(session_id);
    return (it != m_mapSessions.end()) ? &it->second : nullptr;
}

void CWebSocketServer::RemoveSession(DWORD session_id)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);
    m_mapSessions.erase(session_id);
    std::cout << "Session removed: " << session_id << std::endl;
}

DWORD CWebSocketServer::GetSessionCount() const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutexSessions));
    return m_mapSessions.size();
}

DWORD CWebSocketServer::GetCurrentTime() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

DWORD CWebSocketServer::GetNextSequence(DWORD session_id)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);
    auto it = m_mapSessions.find(session_id);
    if (it != m_mapSessions.end())
    {
        return ++it->second.sequence_number;
    }
    return 0;
}

void CWebSocketServer::WorkerThread()
{
    std::cout << "Worker thread started: " << std::this_thread::get_id() << std::endl;

    while (m_bRunning)
    {
        // Timeout kontrolü
        CheckTimeouts();

        // Gerçek implementasyonda burada Boost.Asio event loop çalışır
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Worker thread stopped: " << std::this_thread::get_id() << std::endl;
}

void CWebSocketServer::ProcessPacket(DWORD session_id, const char* data, WORD size)
{
    if (size < sizeof(TPacketHeader))
        return;

    TPacketHeader header;
    memcpy(&header, data, sizeof(TPacketHeader));

    m_dwTotalPacketsReceived++;

    // Handler bul ve çalıştır
    std::lock_guard<std::mutex> lock(m_mutexHandlers);
    auto it = m_mapHandlers.find(header.type);
    if (it != m_mapHandlers.end())
    {
        it->second(session_id, header, data + sizeof(TPacketHeader), size - sizeof(TPacketHeader));
    }
    else
    {
        std::cerr << "No handler for packet type: " << (int)header.type << std::endl;
    }
}

void CWebSocketServer::OnConnect(DWORD session_id, const std::string& remote_ip)
{
    WebSocketSession session;
    session.session_id = session_id;
    session.remote_ip = remote_ip;
    session.connect_time = GetCurrentTime();
    session.last_activity = session.connect_time;

    std::lock_guard<std::mutex> lock(m_mutexSessions);
    m_mapSessions[session_id] = session;
    m_dwTotalConnections++;

    std::cout << "New connection [" << session_id << "] from " << remote_ip << std::endl;
}

void CWebSocketServer::OnDisconnect(DWORD session_id)
{
    std::lock_guard<std::mutex> lock(m_mutexSessions);

    auto it = m_mapSessions.find(session_id);
    if (it != m_mapSessions.end())
    {
        DWORD connection_duration = GetCurrentTime() - it->second.connect_time;
        std::cout << "Client disconnected [" << session_id << "] "
                  << "Duration: " << (connection_duration / 1000) << "s" << std::endl;

        m_mapSessions.erase(it);
    }
}

void CWebSocketServer::CheckTimeouts()
{
    const DWORD TIMEOUT_MS = 30000; // 30 saniye
    DWORD current_time = GetCurrentTime();

    std::lock_guard<std::mutex> lock(m_mutexSessions);

    std::vector<DWORD> to_remove;
    for (auto& pair : m_mapSessions)
    {
        if (current_time - pair.second.last_activity > TIMEOUT_MS)
        {
            to_remove.push_back(pair.first);
        }
    }

    for (DWORD session_id : to_remove)
    {
        std::cout << "Session timeout [" << session_id << "]" << std::endl;
        m_mapSessions.erase(session_id);
    }
}
