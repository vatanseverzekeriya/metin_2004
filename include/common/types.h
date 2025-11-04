#ifndef __INC_COMMON_TYPES_H__
#define __INC_COMMON_TYPES_H__

#include <cstdint>
#include <string>

// Temel tip tanımlamaları
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t  LONG;

// Oyuncu sınıfları
enum ECharacterClass
{
    CLASS_WARRIOR = 0,
    CLASS_ASSASSIN = 1,
    CLASS_SURA = 2,
    CLASS_SHAMAN = 3,
    CLASS_MAX_NUM = 4
};

// Oyuncu durumları
enum ECharacterState
{
    STATE_IDLE = 0,
    STATE_MOVING = 1,
    STATE_ATTACKING = 2,
    STATE_DEAD = 3
};

// PvP modları
enum EPvPMode
{
    PVP_MODE_NONE = 0,
    PVP_MODE_NORMAL = 1,
    PVP_MODE_GUILD = 2,
    PVP_MODE_PARTY = 3
};

// Pozisyon yapısı
struct TPosition
{
    LONG x;
    LONG y;
    LONG z;

    TPosition() : x(0), y(0), z(0) {}
    TPosition(LONG _x, LONG _y, LONG _z = 0) : x(_x), y(_y), z(_z) {}
};

// Oyuncu istatistikleri
struct TPlayerStats
{
    DWORD level;
    DWORD exp;
    DWORD gold;

    // Temel özellikler
    DWORD hp;
    DWORD max_hp;
    DWORD sp;
    DWORD max_sp;

    // Savaş özellikleri
    DWORD attack;
    DWORD defense;
    DWORD magic_attack;
    DWORD magic_defense;

    TPlayerStats() {
        level = 1;
        exp = 0;
        gold = 0;
        hp = max_hp = 1000;
        sp = max_sp = 100;
        attack = 50;
        defense = 30;
        magic_attack = 20;
        magic_defense = 20;
    }
};

#endif // __INC_COMMON_TYPES_H__
