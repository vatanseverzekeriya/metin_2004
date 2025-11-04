#ifndef __INC_AUTH_SESSION_MANAGER_H__
#define __INC_AUTH_SESSION_MANAGER_H__

#include "../common/types.h"
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <mutex>

// Oturum bilgisi yapısı
struct TSessionInfo
{
    DWORD account_id;
    std::string login;
    std::string ip_address;
    std::chrono::steady_clock::time_point login_time;
    std::chrono::steady_clock::time_point last_activity;
    bool is_active;
    DWORD current_player_id; // Şu anda oynanan karakter ID'si

    TSessionInfo()
        : account_id(0)
        , is_active(true)
        , current_player_id(0)
        , login_time(std::chrono::steady_clock::now())
        , last_activity(std::chrono::steady_clock::now())
    {}
};

// Session manager sınıfı - Singleton pattern
class CSessionManager
{
public:
    static CSessionManager& Instance();

    // Oturum işlemleri
    std::string CreateSession(DWORD account_id, const std::string& login, const std::string& ip_address);
    bool ValidateSession(const std::string& session_key);
    bool DestroySession(const std::string& session_key);
    void CleanupExpiredSessions();

    // Oturum bilgileri
    TSessionInfo* GetSession(const std::string& session_key);
    bool UpdateActivity(const std::string& session_key);
    bool SetCurrentPlayer(const std::string& session_key, DWORD player_id);

    // Hesap kontrolü
    bool IsAccountLoggedIn(DWORD account_id);
    std::string GetSessionByAccountID(DWORD account_id);
    int GetActiveSessionCount() const;

    // Konfigürasyon
    void SetSessionTimeout(int seconds) { m_iSessionTimeout = seconds; }
    int GetSessionTimeout() const { return m_iSessionTimeout; }

private:
    CSessionManager();
    ~CSessionManager();

    CSessionManager(const CSessionManager&) = delete;
    CSessionManager& operator=(const CSessionManager&) = delete;

    std::string GenerateSessionKey();
    bool IsSessionExpired(const TSessionInfo& session);

    std::map<std::string, TSessionInfo> m_mapSessions;  // session_key -> session info
    std::map<DWORD, std::string> m_mapAccountSessions;  // account_id -> session_key
    std::mutex m_mutex;
    int m_iSessionTimeout; // Saniye cinsinden (varsayılan: 3600 = 1 saat)
};

#endif // __INC_AUTH_SESSION_MANAGER_H__
