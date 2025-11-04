#ifndef __INC_DB_MANAGER_H__
#define __INC_DB_MANAGER_H__

#include <mysql/mysql.h>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "../common/types.h"

class CDBManager
{
public:
    static CDBManager& Instance();

    bool Initialize(const std::string& host,
                   const std::string& user,
                   const std::string& password,
                   const std::string& database,
                   int port = 3306);

    void Destroy();

    // Sorgu işlemleri
    MYSQL_RES* Query(const char* query, ...);
    bool Execute(const char* query, ...);

    // Oyuncu veritabanı işlemleri
    bool SavePlayer(DWORD player_id, const TPlayerStats& stats, const TPosition& pos);
    bool LoadPlayer(DWORD player_id, TPlayerStats& stats, TPosition& pos);
    bool CreatePlayer(const std::string& name, BYTE job, DWORD account_id, DWORD& out_player_id);
    bool DeletePlayer(DWORD player_id);
    bool GetPlayerName(DWORD player_id, std::string& name);

    // Hesap işlemleri
    bool CreateAccount(const std::string& login, const std::string& password, const std::string& email);
    bool CheckAccount(const std::string& login, const std::string& password);
    DWORD GetAccountID(const std::string& login);
    bool IsAccountExist(const std::string& login);
    bool GetPlayersByAccount(DWORD account_id, std::vector<std::pair<DWORD, std::string>>& players);

    // PvP istatistikleri
    bool UpdatePvPStats(DWORD player_id, DWORD kills, DWORD deaths);
    bool GetPvPStats(DWORD player_id, DWORD& kills, DWORD& deaths);

    // Escape string
    std::string EscapeString(const std::string& str);

    // Password hashing (SHA256)
    std::string HashPassword(const std::string& password);

private:
    CDBManager();
    ~CDBManager();

    CDBManager(const CDBManager&) = delete;
    CDBManager& operator=(const CDBManager&) = delete;

    bool Connect();
    void Disconnect();

    MYSQL* m_pConnection;
    std::string m_strHost;
    std::string m_strUser;
    std::string m_strPassword;
    std::string m_strDatabase;
    int m_iPort;

    bool m_bIsConnected;
};

#endif // __INC_DB_MANAGER_H__
