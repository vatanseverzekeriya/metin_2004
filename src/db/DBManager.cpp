#include "../../include/db/DBManager.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <iostream>

CDBManager::CDBManager()
    : m_pConnection(nullptr)
    , m_iPort(3306)
    , m_bIsConnected(false)
{
}

CDBManager::~CDBManager()
{
    Destroy();
}

CDBManager& CDBManager::Instance()
{
    static CDBManager instance;
    return instance;
}

bool CDBManager::Initialize(const std::string& host,
                            const std::string& user,
                            const std::string& password,
                            const std::string& database,
                            int port)
{
    m_strHost = host;
    m_strUser = user;
    m_strPassword = password;
    m_strDatabase = database;
    m_iPort = port;

    return Connect();
}

bool CDBManager::Connect()
{
    m_pConnection = mysql_init(nullptr);
    if (!m_pConnection)
    {
        std::cerr << "MySQL init failed" << std::endl;
        return false;
    }

    // Bağlantı ayarları
    bool reconnect = true;
    mysql_options(m_pConnection, MYSQL_OPT_RECONNECT, &reconnect);
    mysql_options(m_pConnection, MYSQL_SET_CHARSET_NAME, "utf8");

    // Bağlan
    if (!mysql_real_connect(m_pConnection,
                           m_strHost.c_str(),
                           m_strUser.c_str(),
                           m_strPassword.c_str(),
                           m_strDatabase.c_str(),
                           m_iPort,
                           nullptr,
                           CLIENT_MULTI_STATEMENTS))
    {
        std::cerr << "MySQL connection failed: " << mysql_error(m_pConnection) << std::endl;
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
        return false;
    }

    m_bIsConnected = true;
    std::cout << "Database connected successfully!" << std::endl;
    return true;
}

void CDBManager::Disconnect()
{
    if (m_pConnection)
    {
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
    }
    m_bIsConnected = false;
}

void CDBManager::Destroy()
{
    Disconnect();
}

MYSQL_RES* CDBManager::Query(const char* query, ...)
{
    if (!m_bIsConnected || !m_pConnection)
        return nullptr;

    char szQuery[4096];
    va_list args;
    va_start(args, query);
    vsnprintf(szQuery, sizeof(szQuery), query, args);
    va_end(args);

    if (mysql_query(m_pConnection, szQuery))
    {
        std::cerr << "Query failed: " << mysql_error(m_pConnection) << std::endl;
        std::cerr << "Query: " << szQuery << std::endl;
        return nullptr;
    }

    return mysql_store_result(m_pConnection);
}

bool CDBManager::Execute(const char* query, ...)
{
    if (!m_bIsConnected || !m_pConnection)
        return false;

    char szQuery[4096];
    va_list args;
    va_start(args, query);
    vsnprintf(szQuery, sizeof(szQuery), query, args);
    va_end(args);

    if (mysql_query(m_pConnection, szQuery))
    {
        std::cerr << "Execute failed: " << mysql_error(m_pConnection) << std::endl;
        std::cerr << "Query: " << szQuery << std::endl;
        return false;
    }

    return true;
}

std::string CDBManager::EscapeString(const std::string& str)
{
    if (!m_pConnection)
        return str;

    char* escaped = new char[str.length() * 2 + 1];
    mysql_real_escape_string(m_pConnection, escaped, str.c_str(), str.length());
    std::string result(escaped);
    delete[] escaped;
    return result;
}

bool CDBManager::SavePlayer(DWORD player_id, const TPlayerStats& stats, const TPosition& pos)
{
    return Execute(
        "UPDATE player SET "
        "level=%u, exp=%u, gold=%u, "
        "hp=%u, max_hp=%u, sp=%u, max_sp=%u, "
        "attack=%u, defense=%u, magic_attack=%u, magic_defense=%u, "
        "pos_x=%ld, pos_y=%ld, pos_z=%ld "
        "WHERE id=%u",
        stats.level, stats.exp, stats.gold,
        stats.hp, stats.max_hp, stats.sp, stats.max_sp,
        stats.attack, stats.defense, stats.magic_attack, stats.magic_defense,
        pos.x, pos.y, pos.z,
        player_id
    );
}

bool CDBManager::LoadPlayer(DWORD player_id, TPlayerStats& stats, TPosition& pos)
{
    MYSQL_RES* res = Query(
        "SELECT level, exp, gold, hp, max_hp, sp, max_sp, "
        "attack, defense, magic_attack, magic_defense, "
        "pos_x, pos_y, pos_z FROM player WHERE id=%u",
        player_id
    );

    if (!res)
        return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    stats.level = atoi(row[0]);
    stats.exp = atoi(row[1]);
    stats.gold = atoi(row[2]);
    stats.hp = atoi(row[3]);
    stats.max_hp = atoi(row[4]);
    stats.sp = atoi(row[5]);
    stats.max_sp = atoi(row[6]);
    stats.attack = atoi(row[7]);
    stats.defense = atoi(row[8]);
    stats.magic_attack = atoi(row[9]);
    stats.magic_defense = atoi(row[10]);
    pos.x = atol(row[11]);
    pos.y = atol(row[12]);
    pos.z = atol(row[13]);

    mysql_free_result(res);
    return true;
}

bool CDBManager::CreatePlayer(const std::string& name, BYTE job, DWORD& out_player_id)
{
    std::string escaped_name = EscapeString(name);

    if (!Execute(
        "INSERT INTO player (name, job, level, exp, gold, hp, max_hp, sp, max_sp, "
        "attack, defense, magic_attack, magic_defense, pos_x, pos_y, pos_z) "
        "VALUES ('%s', %u, 1, 0, 0, 1000, 1000, 100, 100, 50, 30, 20, 20, 957200, 244900, 0)",
        escaped_name.c_str(), job
    ))
        return false;

    out_player_id = (DWORD)mysql_insert_id(m_pConnection);
    return true;
}

bool CDBManager::CheckAccount(const std::string& login, const std::string& password)
{
    std::string escaped_login = EscapeString(login);
    std::string escaped_password = EscapeString(password);

    MYSQL_RES* res = Query(
        "SELECT id FROM account WHERE login='%s' AND password=PASSWORD('%s')",
        escaped_login.c_str(), escaped_password.c_str()
    );

    if (!res)
        return false;

    bool exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

DWORD CDBManager::GetAccountID(const std::string& login)
{
    std::string escaped_login = EscapeString(login);

    MYSQL_RES* res = Query(
        "SELECT id FROM account WHERE login='%s'",
        escaped_login.c_str()
    );

    if (!res)
        return 0;

    MYSQL_ROW row = mysql_fetch_row(res);
    DWORD account_id = row ? atoi(row[0]) : 0;
    mysql_free_result(res);
    return account_id;
}

bool CDBManager::UpdatePvPStats(DWORD player_id, DWORD kills, DWORD deaths)
{
    return Execute(
        "UPDATE player SET pvp_kills=%u, pvp_deaths=%u WHERE id=%u",
        kills, deaths, player_id
    );
}

bool CDBManager::GetPvPStats(DWORD player_id, DWORD& kills, DWORD& deaths)
{
    MYSQL_RES* res = Query(
        "SELECT pvp_kills, pvp_deaths FROM player WHERE id=%u",
        player_id
    );

    if (!res)
        return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    kills = atoi(row[0]);
    deaths = atoi(row[1]);
    mysql_free_result(res);
    return true;
}
