#ifndef __INC_AUTH_MANAGER_H__
#define __INC_AUTH_MANAGER_H__

#include "../common/types.h"
#include <string>
#include <vector>

// Kimlik doğrulama sonuçları
enum EAuthResult
{
    AUTH_SUCCESS = 0,
    AUTH_FAILED_INVALID_CREDENTIALS = 1,
    AUTH_FAILED_ALREADY_LOGGED_IN = 2,
    AUTH_FAILED_ACCOUNT_BLOCKED = 3,
    AUTH_FAILED_DB_ERROR = 4,
    AUTH_FAILED_INVALID_SESSION = 5
};

// Kayıt sonuçları
enum ERegisterResult
{
    REGISTER_SUCCESS = 0,
    REGISTER_FAILED_ALREADY_EXISTS = 1,
    REGISTER_FAILED_INVALID_LOGIN = 2,
    REGISTER_FAILED_INVALID_PASSWORD = 3,
    REGISTER_FAILED_INVALID_EMAIL = 4,
    REGISTER_FAILED_DB_ERROR = 5
};

// Karakter oluşturma sonuçları
enum ECreateCharacterResult
{
    CREATE_CHARACTER_SUCCESS = 0,
    CREATE_CHARACTER_FAILED_NAME_EXISTS = 1,
    CREATE_CHARACTER_FAILED_INVALID_NAME = 2,
    CREATE_CHARACTER_FAILED_INVALID_JOB = 3,
    CREATE_CHARACTER_FAILED_MAX_CHARACTERS = 4,
    CREATE_CHARACTER_FAILED_DB_ERROR = 5
};

class CAuthManager
{
public:
    static CAuthManager& Instance();

    // Hesap işlemleri
    ERegisterResult RegisterAccount(const std::string& login,
                                   const std::string& password,
                                   const std::string& email);

    EAuthResult Login(const std::string& login,
                     const std::string& password,
                     const std::string& ip_address,
                     std::string& out_session_key);

    bool Logout(const std::string& session_key);

    // Session doğrulama
    bool ValidateSession(const std::string& session_key);
    DWORD GetAccountIDBySession(const std::string& session_key);

    // Karakter işlemleri
    bool GetCharacterList(const std::string& session_key,
                         std::vector<std::pair<DWORD, std::string>>& characters);

    ECreateCharacterResult CreateCharacter(const std::string& session_key,
                                          const std::string& name,
                                          BYTE job,
                                          DWORD& out_player_id);

    bool DeleteCharacter(const std::string& session_key, DWORD player_id);

    bool SelectCharacter(const std::string& session_key, DWORD player_id);

    // Yardımcı fonksiyonlar
    bool IsValidLoginName(const std::string& login);
    bool IsValidPassword(const std::string& password);
    bool IsValidEmail(const std::string& email);
    bool IsValidCharacterName(const std::string& name);

    // Temizlik
    void CleanupExpiredSessions();

private:
    CAuthManager();
    ~CAuthManager();

    CAuthManager(const CAuthManager&) = delete;
    CAuthManager& operator=(const CAuthManager&) = delete;
};

#endif // __INC_AUTH_MANAGER_H__
