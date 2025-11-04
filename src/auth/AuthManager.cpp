#include "../../include/auth/AuthManager.h"
#include "../../include/auth/SessionManager.h"
#include "../../include/db/DBManager.h"
#include <iostream>
#include <regex>
#include <algorithm>

CAuthManager::CAuthManager()
{
}

CAuthManager::~CAuthManager()
{
}

CAuthManager& CAuthManager::Instance()
{
    static CAuthManager instance;
    return instance;
}

bool CAuthManager::IsValidLoginName(const std::string& login)
{
    // 4-16 karakter, sadece alfanumerik ve alt çizgi
    if (login.length() < 4 || login.length() > 16)
        return false;

    std::regex pattern("^[a-zA-Z0-9_]+$");
    return std::regex_match(login, pattern);
}

bool CAuthManager::IsValidPassword(const std::string& password)
{
    // 6-32 karakter
    if (password.length() < 6 || password.length() > 32)
        return false;

    // En az bir rakam ve bir harf içermeli
    bool has_digit = false;
    bool has_alpha = false;

    for (char c : password)
    {
        if (isdigit(c)) has_digit = true;
        if (isalpha(c)) has_alpha = true;
    }

    return has_digit && has_alpha;
}

bool CAuthManager::IsValidEmail(const std::string& email)
{
    // Basit email validasyonu
    std::regex pattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return std::regex_match(email, pattern);
}

bool CAuthManager::IsValidCharacterName(const std::string& name)
{
    // 3-16 karakter, sadece alfanumerik
    if (name.length() < 3 || name.length() > 16)
        return false;

    std::regex pattern("^[a-zA-Z0-9]+$");
    return std::regex_match(name, pattern);
}

ERegisterResult CAuthManager::RegisterAccount(const std::string& login,
                                             const std::string& password,
                                             const std::string& email)
{
    std::cout << "Register attempt for: " << login << std::endl;

    // Validasyon kontrolleri
    if (!IsValidLoginName(login))
    {
        std::cout << "Invalid login name format" << std::endl;
        return REGISTER_FAILED_INVALID_LOGIN;
    }

    if (!IsValidPassword(password))
    {
        std::cout << "Invalid password format" << std::endl;
        return REGISTER_FAILED_INVALID_PASSWORD;
    }

    if (!IsValidEmail(email))
    {
        std::cout << "Invalid email format" << std::endl;
        return REGISTER_FAILED_INVALID_EMAIL;
    }

    // Hesap zaten var mı kontrol et
    CDBManager& db = CDBManager::Instance();
    if (db.IsAccountExist(login))
    {
        std::cout << "Account already exists: " << login << std::endl;
        return REGISTER_FAILED_ALREADY_EXISTS;
    }

    // Hesap oluştur
    if (!db.CreateAccount(login, password, email))
    {
        std::cout << "Database error while creating account" << std::endl;
        return REGISTER_FAILED_DB_ERROR;
    }

    std::cout << "Account registered successfully: " << login << std::endl;
    return REGISTER_SUCCESS;
}

EAuthResult CAuthManager::Login(const std::string& login,
                               const std::string& password,
                               const std::string& ip_address,
                               std::string& out_session_key)
{
    std::cout << "Login attempt for: " << login << " from IP: " << ip_address << std::endl;

    CDBManager& db = CDBManager::Instance();

    // Kullanıcı adı ve şifre kontrolü
    if (!db.CheckAccount(login, password))
    {
        std::cout << "Invalid credentials for: " << login << std::endl;
        return AUTH_FAILED_INVALID_CREDENTIALS;
    }

    // Account ID'yi al
    DWORD account_id = db.GetAccountID(login);
    if (account_id == 0)
    {
        std::cout << "Failed to get account ID for: " << login << std::endl;
        return AUTH_FAILED_DB_ERROR;
    }

    // Session oluştur
    CSessionManager& session_mgr = CSessionManager::Instance();
    out_session_key = session_mgr.CreateSession(account_id, login, ip_address);

    std::cout << "Login successful for: " << login << " (Account ID: " << account_id << ")" << std::endl;
    return AUTH_SUCCESS;
}

bool CAuthManager::Logout(const std::string& session_key)
{
    std::cout << "Logout request for session: " << session_key << std::endl;

    CSessionManager& session_mgr = CSessionManager::Instance();
    return session_mgr.DestroySession(session_key);
}

bool CAuthManager::ValidateSession(const std::string& session_key)
{
    CSessionManager& session_mgr = CSessionManager::Instance();
    return session_mgr.ValidateSession(session_key);
}

DWORD CAuthManager::GetAccountIDBySession(const std::string& session_key)
{
    CSessionManager& session_mgr = CSessionManager::Instance();
    TSessionInfo* session = session_mgr.GetSession(session_key);

    if (!session)
        return 0;

    session_mgr.UpdateActivity(session_key);
    return session->account_id;
}

bool CAuthManager::GetCharacterList(const std::string& session_key,
                                   std::vector<std::pair<DWORD, std::string>>& characters)
{
    DWORD account_id = GetAccountIDBySession(session_key);
    if (account_id == 0)
    {
        std::cout << "Invalid session for character list request" << std::endl;
        return false;
    }

    CDBManager& db = CDBManager::Instance();
    return db.GetPlayersByAccount(account_id, characters);
}

ECreateCharacterResult CAuthManager::CreateCharacter(const std::string& session_key,
                                                    const std::string& name,
                                                    BYTE job,
                                                    DWORD& out_player_id)
{
    std::cout << "Character creation request: " << name << " (Job: " << (int)job << ")" << std::endl;

    // Session kontrolü
    DWORD account_id = GetAccountIDBySession(session_key);
    if (account_id == 0)
    {
        std::cout << "Invalid session for character creation" << std::endl;
        return CREATE_CHARACTER_FAILED_DB_ERROR;
    }

    // İsim validasyonu
    if (!IsValidCharacterName(name))
    {
        std::cout << "Invalid character name format: " << name << std::endl;
        return CREATE_CHARACTER_FAILED_INVALID_NAME;
    }

    // Job validasyonu
    if (job >= CLASS_MAX_NUM)
    {
        std::cout << "Invalid job: " << (int)job << std::endl;
        return CREATE_CHARACTER_FAILED_INVALID_JOB;
    }

    // Maksimum karakter sayısı kontrolü (4 karakter limiti)
    CDBManager& db = CDBManager::Instance();
    std::vector<std::pair<DWORD, std::string>> characters;
    if (db.GetPlayersByAccount(account_id, characters) && characters.size() >= 4)
    {
        std::cout << "Maximum character limit reached for account " << account_id << std::endl;
        return CREATE_CHARACTER_FAILED_MAX_CHARACTERS;
    }

    // Karakter oluştur
    if (!db.CreatePlayer(name, job, account_id, out_player_id))
    {
        std::cout << "Failed to create character in database" << std::endl;
        return CREATE_CHARACTER_FAILED_DB_ERROR;
    }

    std::cout << "Character created successfully: " << name
              << " (Player ID: " << out_player_id << ")" << std::endl;
    return CREATE_CHARACTER_SUCCESS;
}

bool CAuthManager::DeleteCharacter(const std::string& session_key, DWORD player_id)
{
    std::cout << "Character deletion request for player ID: " << player_id << std::endl;

    // Session kontrolü
    DWORD account_id = GetAccountIDBySession(session_key);
    if (account_id == 0)
    {
        std::cout << "Invalid session for character deletion" << std::endl;
        return false;
    }

    // Bu karakterin bu hesaba ait olduğunu doğrula
    CDBManager& db = CDBManager::Instance();
    std::vector<std::pair<DWORD, std::string>> characters;
    if (!db.GetPlayersByAccount(account_id, characters))
        return false;

    bool found = false;
    for (const auto& ch : characters)
    {
        if (ch.first == player_id)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        std::cout << "Character " << player_id << " does not belong to account " << account_id << std::endl;
        return false;
    }

    // Karakteri sil
    if (!db.DeletePlayer(player_id))
    {
        std::cout << "Failed to delete character from database" << std::endl;
        return false;
    }

    std::cout << "Character deleted successfully: " << player_id << std::endl;
    return true;
}

bool CAuthManager::SelectCharacter(const std::string& session_key, DWORD player_id)
{
    std::cout << "Character selection request for player ID: " << player_id << std::endl;

    // Session kontrolü
    DWORD account_id = GetAccountIDBySession(session_key);
    if (account_id == 0)
    {
        std::cout << "Invalid session for character selection" << std::endl;
        return false;
    }

    // Bu karakterin bu hesaba ait olduğunu doğrula
    CDBManager& db = CDBManager::Instance();
    std::vector<std::pair<DWORD, std::string>> characters;
    if (!db.GetPlayersByAccount(account_id, characters))
        return false;

    bool found = false;
    for (const auto& ch : characters)
    {
        if (ch.first == player_id)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        std::cout << "Character " << player_id << " does not belong to account " << account_id << std::endl;
        return false;
    }

    // Session'a karakter bilgisini ekle
    CSessionManager& session_mgr = CSessionManager::Instance();
    if (!session_mgr.SetCurrentPlayer(session_key, player_id))
    {
        std::cout << "Failed to set current player in session" << std::endl;
        return false;
    }

    std::cout << "Character selected successfully: " << player_id << std::endl;
    return true;
}

void CAuthManager::CleanupExpiredSessions()
{
    CSessionManager& session_mgr = CSessionManager::Instance();
    session_mgr.CleanupExpiredSessions();
}
