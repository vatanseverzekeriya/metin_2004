#ifndef __INC_SCRIPT_LUA_BINDING_H__
#define __INC_SCRIPT_LUA_BINDING_H__

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "../common/types.h"
#include <string>
#include <map>

// Forward declaration
class CCharacter;

// Lua event türleri
enum ELuaEventType
{
    LUA_EVENT_NONE = 0,
    LUA_EVENT_QUEST = 1,            // Quest eventi
    LUA_EVENT_DUNGEON = 2,          // Dungeon eventi
    LUA_EVENT_BOSS_SPAWN = 3,       // Boss spawn eventi
    LUA_EVENT_PVP_ARENA = 4,        // PvP arena eventi
    LUA_EVENT_DROP_ITEM = 5,        // Item drop eventi
    LUA_EVENT_LEVEL_UP = 6,         // Level up eventi
    LUA_EVENT_KILL = 7,             // Kill eventi
    LUA_EVENT_LOGIN = 8,            // Login eventi
    LUA_EVENT_LOGOUT = 9,           // Logout eventi
    LUA_EVENT_CUSTOM = 100          // Özel eventler
};

// Lua script manager
class CLuaBinding
{
public:
    static CLuaBinding& Instance();

    // Lua state yönetimi
    bool Initialize();
    void Destroy();

    // Script yükleme ve çalıştırma
    bool LoadScript(const std::string& filename);
    bool ExecuteScript(const std::string& script_code);

    // Fonksiyon çağırma
    bool CallFunction(const std::string& func_name);
    bool CallFunction(const std::string& func_name, CCharacter* ch);
    bool CallFunction(const std::string& func_name, CCharacter* ch, DWORD arg1);
    bool CallFunction(const std::string& func_name, CCharacter* ch, const std::string& arg1);

    // Event sistemi
    bool RegisterEvent(const std::string& event_name, const std::string& script_file);
    bool TriggerEvent(const std::string& event_name, CCharacter* ch, DWORD arg = 0);

    // Global değişkenler
    void SetGlobalNumber(const std::string& name, lua_Number value);
    void SetGlobalString(const std::string& name, const std::string& value);
    lua_Number GetGlobalNumber(const std::string& name);
    std::string GetGlobalString(const std::string& name);

    // Lua state al (gelişmiş kullanım için)
    lua_State* GetLuaState() { return L; }

private:
    CLuaBinding();
    ~CLuaBinding();

    CLuaBinding(const CLuaBinding&) = delete;
    CLuaBinding& operator=(const CLuaBinding&) = delete;

    lua_State* L;
    std::map<std::string, std::string> m_mapEvents; // event_name -> script_file

    // C++ fonksiyonlarını Lua'ya kaydet
    void RegisterCFunctions();

    // Lua'dan çağrılabilecek C++ fonksiyonları
    static int lua_SendMessage(lua_State* L);
    static int lua_GiveExp(lua_State* L);
    static int lua_GiveGold(lua_State* L);
    static int lua_GiveItem(lua_State* L);
    static int lua_AddAffect(lua_State* L);
    static int lua_RemoveAffect(lua_State* L);
    static int lua_TeleportPlayer(lua_State* L);
    static int lua_SpawnMonster(lua_State* L);
    static int lua_GetPlayerLevel(lua_State* L);
    static int lua_GetPlayerHP(lua_State* L);
    static int lua_SetPlayerHP(lua_State* L);
    static int lua_GetQuestFlag(lua_State* L);
    static int lua_SetQuestFlag(lua_State* L);
    static int lua_IsPartyMember(lua_State* L);
    static int lua_GetPartyMemberCount(lua_State* L);

    // Yardımcı fonksiyonlar
    CCharacter* GetCharacterFromLua(lua_State* L, int index);
    void PushCharacterToLua(lua_State* L, CCharacter* ch);
};

// Lua script helper macros
#define LUA_CHECK_ARGS(L, n) \
    if (lua_gettop(L) < n) { \
        lua_pushstring(L, "Not enough arguments"); \
        lua_error(L); \
        return 0; \
    }

#define LUA_GET_CHARACTER(L, idx) \
    CLuaBinding::Instance().GetCharacterFromLua(L, idx)

#endif // __INC_SCRIPT_LUA_BINDING_H__
