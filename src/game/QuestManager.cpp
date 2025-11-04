#include "../../include/game/QuestManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Global pointer for accessing character from Lua
static CCharacter* g_pCurrentCharacter = nullptr;

CQuestManager::CQuestManager()
    : m_pLuaState(nullptr)
{
}

CQuestManager::~CQuestManager()
{
    Destroy();
}

CQuestManager& CQuestManager::Instance()
{
    static CQuestManager instance;
    return instance;
}

bool CQuestManager::Initialize()
{
    // Lua state oluştur
    m_pLuaState = luaL_newstate();
    if (!m_pLuaState)
    {
        std::cerr << "Failed to create Lua state!" << std::endl;
        return false;
    }

    // Standart kütüphaneleri yükle
    luaL_openlibs(m_pLuaState);

    // Custom fonksiyonları kaydet
    RegisterLuaFunctions();

    std::cout << "Quest Manager initialized successfully!" << std::endl;
    return true;
}

void CQuestManager::Destroy()
{
    if (m_pLuaState)
    {
        lua_close(m_pLuaState);
        m_pLuaState = nullptr;
    }
    m_mapQuests.clear();
}

void CQuestManager::RegisterLuaFunctions()
{
    // pc namespace oluştur
    lua_newtable(m_pLuaState);

    // Fonksiyonları kaydet
    lua_pushcfunction(m_pLuaState, lua_GetLevel);
    lua_setfield(m_pLuaState, -2, "get_level");

    lua_pushcfunction(m_pLuaState, lua_GetGold);
    lua_setfield(m_pLuaState, -2, "get_gold");

    lua_pushcfunction(m_pLuaState, lua_SetGold);
    lua_setfield(m_pLuaState, -2, "change_gold");

    lua_pushcfunction(m_pLuaState, lua_GiveExp);
    lua_setfield(m_pLuaState, -2, "give_exp");

    lua_pushcfunction(m_pLuaState, lua_Chat);
    lua_setfield(m_pLuaState, -2, "chat");

    lua_pushcfunction(m_pLuaState, lua_Notice);
    lua_setfield(m_pLuaState, -2, "notice");

    lua_pushcfunction(m_pLuaState, lua_GetHP);
    lua_setfield(m_pLuaState, -2, "get_hp");

    lua_pushcfunction(m_pLuaState, lua_SetHP);
    lua_setfield(m_pLuaState, -2, "set_hp");

    lua_pushcfunction(m_pLuaState, lua_GetName);
    lua_setfield(m_pLuaState, -2, "get_name");

    lua_pushcfunction(m_pLuaState, lua_TeleportTo);
    lua_setfield(m_pLuaState, -2, "teleport");

    // Global olarak pc namespace'ini ayarla
    lua_setglobal(m_pLuaState, "pc");
}

bool CQuestManager::LoadQuest(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open quest file: " << filename << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Quest dosyasını Lua'ya yükle
    if (luaL_dostring(m_pLuaState, content.c_str()) != 0)
    {
        std::cerr << "Lua error in " << filename << ": "
                  << lua_tostring(m_pLuaState, -1) << std::endl;
        lua_pop(m_pLuaState, 1);
        return false;
    }

    std::cout << "Loaded quest: " << filename << std::endl;
    return true;
}

bool CQuestManager::LoadAllQuests()
{
    // quest dizinindeki tüm .lua dosyalarını yükle
    std::vector<std::string> quest_files = {
        "quest/welcome_quest.lua",
        "quest/pvp_tutorial.lua",
        "quest/daily_rewards.lua"
    };

    bool all_success = true;
    for (const auto& file : quest_files)
    {
        if (!LoadQuest(file))
        {
            all_success = false;
        }
    }

    return all_success;
}

bool CQuestManager::RunQuest(const std::string& quest_name, CCharacter* ch)
{
    if (!ch || !m_pLuaState)
        return false;

    g_pCurrentCharacter = ch;

    // Quest fonksiyonunu çağır
    lua_getglobal(m_pLuaState, quest_name.c_str());

    if (!lua_isfunction(m_pLuaState, -1))
    {
        std::cerr << "Quest function not found: " << quest_name << std::endl;
        lua_pop(m_pLuaState, 1);
        g_pCurrentCharacter = nullptr;
        return false;
    }

    // Quest'i çalıştır
    if (lua_pcall(m_pLuaState, 0, 0, 0) != 0)
    {
        std::cerr << "Error running quest " << quest_name << ": "
                  << lua_tostring(m_pLuaState, -1) << std::endl;
        lua_pop(m_pLuaState, 1);
        g_pCurrentCharacter = nullptr;
        return false;
    }

    g_pCurrentCharacter = nullptr;
    return true;
}

bool CQuestManager::RunQuestState(const std::string& quest_name,
                                  const std::string& state,
                                  CCharacter* ch)
{
    if (!ch || !m_pLuaState)
        return false;

    g_pCurrentCharacter = ch;

    // quest.state şeklinde fonksiyon çağır
    lua_getglobal(m_pLuaState, quest_name.c_str());
    if (!lua_istable(m_pLuaState, -1))
    {
        lua_pop(m_pLuaState, 1);
        g_pCurrentCharacter = nullptr;
        return false;
    }

    lua_getfield(m_pLuaState, -1, state.c_str());
    if (!lua_isfunction(m_pLuaState, -1))
    {
        lua_pop(m_pLuaState, 2);
        g_pCurrentCharacter = nullptr;
        return false;
    }

    if (lua_pcall(m_pLuaState, 0, 0, 0) != 0)
    {
        std::cerr << "Error running quest state " << quest_name << "." << state << ": "
                  << lua_tostring(m_pLuaState, -1) << std::endl;
        lua_pop(m_pLuaState, 1);
        g_pCurrentCharacter = nullptr;
        return false;
    }

    lua_pop(m_pLuaState, 1); // Pop quest table
    g_pCurrentCharacter = nullptr;
    return true;
}

// Lua API Implementation
int CQuestManager::lua_GetLevel(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        lua_pushnumber(L, g_pCurrentCharacter->GetLevel());
        return 1;
    }
    return 0;
}

int CQuestManager::lua_GetGold(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        lua_pushnumber(L, g_pCurrentCharacter->GetGold());
        return 1;
    }
    return 0;
}

int CQuestManager::lua_SetGold(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        LONG amount = (LONG)luaL_checknumber(L, 1);
        g_pCurrentCharacter->ChangeGold(amount);
    }
    return 0;
}

int CQuestManager::lua_GiveExp(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        DWORD exp = (DWORD)luaL_checknumber(L, 1);
        g_pCurrentCharacter->GiveExp(exp);
    }
    return 0;
}

int CQuestManager::lua_GiveItem(lua_State* L)
{
    // Item sistemi henüz implement edilmedi
    // DWORD item_vnum = (DWORD)luaL_checknumber(L, 1);
    // BYTE count = (BYTE)luaL_optnumber(L, 2, 1);
    std::cout << "GiveItem called (not implemented yet)" << std::endl;
    return 0;
}

int CQuestManager::lua_Chat(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        const char* message = luaL_checkstring(L, 1);
        std::cout << "[" << g_pCurrentCharacter->GetName() << " says]: "
                  << message << std::endl;
    }
    return 0;
}

int CQuestManager::lua_Notice(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        const char* message = luaL_checkstring(L, 1);
        std::cout << "[NOTICE to " << g_pCurrentCharacter->GetName() << "]: "
                  << message << std::endl;
    }
    return 0;
}

int CQuestManager::lua_GetHP(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        lua_pushnumber(L, g_pCurrentCharacter->GetHP());
        return 1;
    }
    return 0;
}

int CQuestManager::lua_SetHP(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        DWORD hp = (DWORD)luaL_checknumber(L, 1);
        g_pCurrentCharacter->SetHP(hp);
    }
    return 0;
}

int CQuestManager::lua_GetName(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        lua_pushstring(L, g_pCurrentCharacter->GetName().c_str());
        return 1;
    }
    return 0;
}

int CQuestManager::lua_TeleportTo(lua_State* L)
{
    if (g_pCurrentCharacter)
    {
        LONG x = (LONG)luaL_checknumber(L, 1);
        LONG y = (LONG)luaL_checknumber(L, 2);
        g_pCurrentCharacter->SetPosition(x, y);
        std::cout << g_pCurrentCharacter->GetName() << " teleported to ("
                  << x << ", " << y << ")" << std::endl;
    }
    return 0;
}
