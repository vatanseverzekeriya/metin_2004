#include "../../include/script/LuaBinding.h"
#include "../../include/game/Character.h"
#include "../../include/game/AffectManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

CLuaBinding::CLuaBinding()
    : L(nullptr)
{
}

CLuaBinding::~CLuaBinding()
{
    Destroy();
}

CLuaBinding& CLuaBinding::Instance()
{
    static CLuaBinding instance;
    return instance;
}

bool CLuaBinding::Initialize()
{
    // Lua state oluştur
    L = luaL_newstate();
    if (!L)
    {
        std::cerr << "Failed to create Lua state" << std::endl;
        return false;
    }

    // Standart kütüphaneleri yükle
    luaL_openlibs(L);

    // C++ fonksiyonlarını kaydet
    RegisterCFunctions();

    std::cout << "Lua system initialized (version: " << LUA_VERSION << ")" << std::endl;
    return true;
}

void CLuaBinding::Destroy()
{
    if (L)
    {
        lua_close(L);
        L = nullptr;
    }
}

void CLuaBinding::RegisterCFunctions()
{
    // Game fonksiyonları
    lua_register(L, "SendMessage", lua_SendMessage);
    lua_register(L, "GiveExp", lua_GiveExp);
    lua_register(L, "GiveGold", lua_GiveGold);
    lua_register(L, "GiveItem", lua_GiveItem);
    lua_register(L, "AddAffect", lua_AddAffect);
    lua_register(L, "RemoveAffect", lua_RemoveAffect);
    lua_register(L, "TeleportPlayer", lua_TeleportPlayer);
    lua_register(L, "SpawnMonster", lua_SpawnMonster);

    // Query fonksiyonları
    lua_register(L, "GetPlayerLevel", lua_GetPlayerLevel);
    lua_register(L, "GetPlayerHP", lua_GetPlayerHP);
    lua_register(L, "SetPlayerHP", lua_SetPlayerHP);

    // Quest fonksiyonları
    lua_register(L, "GetQuestFlag", lua_GetQuestFlag);
    lua_register(L, "SetQuestFlag", lua_SetQuestFlag);

    // Party fonksiyonları
    lua_register(L, "IsPartyMember", lua_IsPartyMember);
    lua_register(L, "GetPartyMemberCount", lua_GetPartyMemberCount);

    std::cout << "Registered " << 13 << " Lua functions" << std::endl;
}

bool CLuaBinding::LoadScript(const std::string& filename)
{
    if (!L)
        return false;

    std::cout << "Loading Lua script: " << filename << std::endl;

    if (luaL_dofile(L, filename.c_str()) != LUA_OK)
    {
        const char* error = lua_tostring(L, -1);
        std::cerr << "Lua script error: " << error << std::endl;
        lua_pop(L, 1);
        return false;
    }

    std::cout << "Script loaded successfully: " << filename << std::endl;
    return true;
}

bool CLuaBinding::ExecuteScript(const std::string& script_code)
{
    if (!L)
        return false;

    if (luaL_dostring(L, script_code.c_str()) != LUA_OK)
    {
        const char* error = lua_tostring(L, -1);
        std::cerr << "Lua execution error: " << error << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool CLuaBinding::CallFunction(const std::string& func_name)
{
    if (!L)
        return false;

    lua_getglobal(L, func_name.c_str());

    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        const char* error = lua_tostring(L, -1);
        std::cerr << "Lua function call error: " << error << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool CLuaBinding::CallFunction(const std::string& func_name, CCharacter* ch)
{
    if (!L || !ch)
        return false;

    lua_getglobal(L, func_name.c_str());

    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    // Character pointer'ı lightuserdata olarak push et
    lua_pushlightuserdata(L, ch);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
        const char* error = lua_tostring(L, -1);
        std::cerr << "Lua function call error: " << error << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool CLuaBinding::CallFunction(const std::string& func_name, CCharacter* ch, DWORD arg1)
{
    if (!L || !ch)
        return false;

    lua_getglobal(L, func_name.c_str());

    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    lua_pushlightuserdata(L, ch);
    lua_pushinteger(L, arg1);

    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        const char* error = lua_tostring(L, -1);
        std::cerr << "Lua function call error: " << error << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool CLuaBinding::CallFunction(const std::string& func_name, CCharacter* ch, const std::string& arg1)
{
    if (!L || !ch)
        return false;

    lua_getglobal(L, func_name.c_str());

    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    lua_pushlightuserdata(L, ch);
    lua_pushstring(L, arg1.c_str());

    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        const char* error = lua_tostring(L, -1);
        std::cerr << "Lua function call error: " << error << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool CLuaBinding::RegisterEvent(const std::string& event_name, const std::string& script_file)
{
    m_mapEvents[event_name] = script_file;
    std::cout << "Event registered: " << event_name << " -> " << script_file << std::endl;
    return true;
}

bool CLuaBinding::TriggerEvent(const std::string& event_name, CCharacter* ch, DWORD arg)
{
    auto it = m_mapEvents.find(event_name);
    if (it == m_mapEvents.end())
        return false;

    // Script dosyasını yükle (henüz yüklenmediyse)
    // Not: Cache mekanizması eklenebilir
    LoadScript(it->second);

    // "on_event" fonksiyonunu çağır
    std::string func_name = "on_" + event_name;
    return CallFunction(func_name, ch, arg);
}

void CLuaBinding::SetGlobalNumber(const std::string& name, lua_Number value)
{
    if (!L)
        return;

    lua_pushnumber(L, value);
    lua_setglobal(L, name.c_str());
}

void CLuaBinding::SetGlobalString(const std::string& name, const std::string& value)
{
    if (!L)
        return;

    lua_pushstring(L, value.c_str());
    lua_setglobal(L, name.c_str());
}

lua_Number CLuaBinding::GetGlobalNumber(const std::string& name)
{
    if (!L)
        return 0;

    lua_getglobal(L, name.c_str());
    lua_Number value = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return value;
}

std::string CLuaBinding::GetGlobalString(const std::string& name)
{
    if (!L)
        return "";

    lua_getglobal(L, name.c_str());
    const char* value = lua_tostring(L, -1);
    std::string result = value ? value : "";
    lua_pop(L, 1);
    return result;
}

CCharacter* CLuaBinding::GetCharacterFromLua(lua_State* L, int index)
{
    if (!lua_islightuserdata(L, index))
        return nullptr;

    return static_cast<CCharacter*>(lua_touserdata(L, index));
}

void CLuaBinding::PushCharacterToLua(lua_State* L, CCharacter* ch)
{
    lua_pushlightuserdata(L, ch);
}

// ============================================================================
// Lua'dan çağrılabilir C++ fonksiyonları
// ============================================================================

int CLuaBinding::lua_SendMessage(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    const char* message = lua_tostring(L, 2);

    if (ch && message)
    {
        std::cout << "[MSG to " << ch->GetName() << "] " << message << std::endl;
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_GiveExp(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    DWORD exp = (DWORD)lua_tointeger(L, 2);

    if (ch)
    {
        ch->GiveExp(exp);
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_GiveGold(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    LONG gold = (LONG)lua_tointeger(L, 2);

    if (ch)
    {
        ch->ChangeGold(gold);
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_GiveItem(lua_State* L)
{
    LUA_CHECK_ARGS(L, 3);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    DWORD item_vnum = (DWORD)lua_tointeger(L, 2);
    DWORD count = (DWORD)lua_tointeger(L, 3);

    if (ch)
    {
        std::cout << ch->GetName() << " received item " << item_vnum << " x" << count << std::endl;
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_AddAffect(lua_State* L)
{
    LUA_CHECK_ARGS(L, 4);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    EAffectType affect_type = (EAffectType)lua_tointeger(L, 2);
    DWORD value = (DWORD)lua_tointeger(L, 3);
    DWORD duration_ms = (DWORD)lua_tointeger(L, 4);

    if (ch && ch->GetAffectManager())
    {
        bool success = ch->GetAffectManager()->AddAffect(affect_type, value, duration_ms);
        lua_pushboolean(L, success ? 1 : 0);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_RemoveAffect(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    EAffectType affect_type = (EAffectType)lua_tointeger(L, 2);

    if (ch && ch->GetAffectManager())
    {
        bool success = ch->GetAffectManager()->RemoveAffect(affect_type);
        lua_pushboolean(L, success ? 1 : 0);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_TeleportPlayer(lua_State* L)
{
    LUA_CHECK_ARGS(L, 3);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    LONG x = (LONG)lua_tointeger(L, 2);
    LONG y = (LONG)lua_tointeger(L, 3);

    if (ch)
    {
        ch->SetPosition(x, y);
        std::cout << ch->GetName() << " teleported to (" << x << ", " << y << ")" << std::endl;
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_SpawnMonster(lua_State* L)
{
    LUA_CHECK_ARGS(L, 3);

    DWORD monster_vnum = (DWORD)lua_tointeger(L, 1);
    LONG x = (LONG)lua_tointeger(L, 2);
    LONG y = (LONG)lua_tointeger(L, 3);

    std::cout << "Spawning monster " << monster_vnum << " at (" << x << ", " << y << ")" << std::endl;
    lua_pushboolean(L, 1);

    return 1;
}

int CLuaBinding::lua_GetPlayerLevel(lua_State* L)
{
    LUA_CHECK_ARGS(L, 1);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);

    if (ch)
    {
        lua_pushinteger(L, ch->GetLevel());
    }
    else
    {
        lua_pushinteger(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_GetPlayerHP(lua_State* L)
{
    LUA_CHECK_ARGS(L, 1);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);

    if (ch)
    {
        lua_pushinteger(L, ch->GetHP());
    }
    else
    {
        lua_pushinteger(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_SetPlayerHP(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    DWORD hp = (DWORD)lua_tointeger(L, 2);

    if (ch)
    {
        ch->SetHP(hp);
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_GetQuestFlag(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    const char* flag_name = lua_tostring(L, 2);

    if (ch && flag_name)
    {
        // Quest flag sistemi eklenecek
        lua_pushinteger(L, 0);
    }
    else
    {
        lua_pushinteger(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_SetQuestFlag(lua_State* L)
{
    LUA_CHECK_ARGS(L, 3);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);
    const char* flag_name = lua_tostring(L, 2);
    DWORD value = (DWORD)lua_tointeger(L, 3);

    if (ch && flag_name)
    {
        std::cout << ch->GetName() << ": Quest flag '" << flag_name << "' = " << value << std::endl;
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_IsPartyMember(lua_State* L)
{
    LUA_CHECK_ARGS(L, 2);

    CCharacter* ch1 = LUA_GET_CHARACTER(L, 1);
    CCharacter* ch2 = LUA_GET_CHARACTER(L, 2);

    if (ch1 && ch2)
    {
        // Party sistemi eklenecek
        lua_pushboolean(L, 0);
    }
    else
    {
        lua_pushboolean(L, 0);
    }

    return 1;
}

int CLuaBinding::lua_GetPartyMemberCount(lua_State* L)
{
    LUA_CHECK_ARGS(L, 1);

    CCharacter* ch = LUA_GET_CHARACTER(L, 1);

    if (ch)
    {
        // Party sistemi eklenecek
        lua_pushinteger(L, 1);
    }
    else
    {
        lua_pushinteger(L, 0);
    }

    return 1;
}
