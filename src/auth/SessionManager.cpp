#include "../../include/auth/SessionManager.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

CSessionManager::CSessionManager()
    : m_iSessionTimeout(3600) // 1 saat varsayılan
{
}

CSessionManager::~CSessionManager()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapSessions.clear();
    m_mapAccountSessions.clear();
}

CSessionManager& CSessionManager::Instance()
{
    static CSessionManager instance;
    return instance;
}

std::string CSessionManager::GenerateSessionKey()
{
    // Random session key üret (32 karakter hex string)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 16; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }

    return ss.str();
}

std::string CSessionManager::CreateSession(DWORD account_id, const std::string& login, const std::string& ip_address)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Eğer bu hesap zaten giriş yapmışsa, eski oturumu sil
    auto it = m_mapAccountSessions.find(account_id);
    if (it != m_mapAccountSessions.end())
    {
        std::string old_session_key = it->second;
        m_mapSessions.erase(old_session_key);
        std::cout << "Removing old session for account " << account_id << std::endl;
    }

    // Yeni session key üret
    std::string session_key;
    do {
        session_key = GenerateSessionKey();
    } while (m_mapSessions.find(session_key) != m_mapSessions.end());

    // Session bilgisini oluştur
    TSessionInfo session;
    session.account_id = account_id;
    session.login = login;
    session.ip_address = ip_address;
    session.login_time = std::chrono::steady_clock::now();
    session.last_activity = std::chrono::steady_clock::now();
    session.is_active = true;
    session.current_player_id = 0;

    m_mapSessions[session_key] = session;
    m_mapAccountSessions[account_id] = session_key;

    std::cout << "Session created for account " << account_id
              << " (login: " << login << ", IP: " << ip_address << ")" << std::endl;

    return session_key;
}

bool CSessionManager::ValidateSession(const std::string& session_key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(session_key);
    if (it == m_mapSessions.end())
        return false;

    // Oturum süresi dolmuş mu kontrol et
    if (IsSessionExpired(it->second))
    {
        std::cout << "Session expired for account " << it->second.account_id << std::endl;

        // Süresi dolmuş oturumu temizle
        DWORD account_id = it->second.account_id;
        m_mapAccountSessions.erase(account_id);
        m_mapSessions.erase(it);
        return false;
    }

    return it->second.is_active;
}

bool CSessionManager::DestroySession(const std::string& session_key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(session_key);
    if (it == m_mapSessions.end())
        return false;

    DWORD account_id = it->second.account_id;
    std::cout << "Destroying session for account " << account_id << std::endl;

    m_mapAccountSessions.erase(account_id);
    m_mapSessions.erase(it);

    return true;
}

void CSessionManager::CleanupExpiredSessions()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.begin();
    while (it != m_mapSessions.end())
    {
        if (IsSessionExpired(it->second))
        {
            DWORD account_id = it->second.account_id;
            std::cout << "Cleaning up expired session for account " << account_id << std::endl;

            m_mapAccountSessions.erase(account_id);
            it = m_mapSessions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

TSessionInfo* CSessionManager::GetSession(const std::string& session_key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(session_key);
    if (it == m_mapSessions.end())
        return nullptr;

    // Süresi dolmuş oturumlar için nullptr dön
    if (IsSessionExpired(it->second))
        return nullptr;

    return &it->second;
}

bool CSessionManager::UpdateActivity(const std::string& session_key)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(session_key);
    if (it == m_mapSessions.end())
        return false;

    it->second.last_activity = std::chrono::steady_clock::now();
    return true;
}

bool CSessionManager::SetCurrentPlayer(const std::string& session_key, DWORD player_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapSessions.find(session_key);
    if (it == m_mapSessions.end())
        return false;

    it->second.current_player_id = player_id;
    std::cout << "Account " << it->second.account_id
              << " is now playing with character " << player_id << std::endl;

    return true;
}

bool CSessionManager::IsAccountLoggedIn(DWORD account_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapAccountSessions.find(account_id);
    if (it == m_mapAccountSessions.end())
        return false;

    // Session key'i al ve kontrol et
    std::string session_key = it->second;
    auto session_it = m_mapSessions.find(session_key);
    if (session_it == m_mapSessions.end())
        return false;

    return !IsSessionExpired(session_it->second) && session_it->second.is_active;
}

std::string CSessionManager::GetSessionByAccountID(DWORD account_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_mapAccountSessions.find(account_id);
    if (it == m_mapAccountSessions.end())
        return "";

    return it->second;
}

int CSessionManager::GetActiveSessionCount() const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    return static_cast<int>(m_mapSessions.size());
}

bool CSessionManager::IsSessionExpired(const TSessionInfo& session)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.last_activity).count();
    return elapsed > m_iSessionTimeout;
}
