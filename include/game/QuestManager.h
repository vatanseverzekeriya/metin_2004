#ifndef __INC_GAME_QUEST_MANAGER_H__
#define __INC_GAME_QUEST_MANAGER_H__

#include "../common/types.h"
#include "Character.h"

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <string>
#include <map>
#include <vector>

class CQuestManager
{
public:
    static CQuestManager& Instance();

    bool Initialize();
    void Destroy();

    // Quest yükleme
    bool LoadQuest(const std::string& filename);
    bool LoadAllQuests();

    // Quest çalıştırma
    bool RunQuest(const std::string& quest_name, CCharacter* ch);
    bool RunQuestState(const std::string& quest_name, const std::string& state, CCharacter* ch);

    // Lua state
    lua_State* GetLuaState() { return m_pLuaState; }

    // Lua API fonksiyonları (C++ tarafında implement edilir)
    static int lua_GetLevel(lua_State* L);
    static int lua_GetGold(lua_State* L);
    static int lua_SetGold(lua_State* L);
    static int lua_GiveExp(lua_State* L);
    static int lua_GiveItem(lua_State* L);
    static int lua_Chat(lua_State* L);
    static int lua_Notice(lua_State* L);
    static int lua_GetHP(lua_State* L);
    static int lua_SetHP(lua_State* L);
    static int lua_GetName(lua_State* L);
    static int lua_TeleportTo(lua_State* L);

private:
    CQuestManager();
    ~CQuestManager();

    CQuestManager(const CQuestManager&) = delete;
    CQuestManager& operator=(const CQuestManager&) = delete;

    void RegisterLuaFunctions();

    lua_State* m_pLuaState;
    std::map<std::string, std::string> m_mapQuests; // quest_name -> filename
};

#endif // __INC_GAME_QUEST_MANAGER_H__
