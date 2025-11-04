#ifndef __INC_NETWORK_PROTOCOL_H__
#define __INC_NETWORK_PROTOCOL_H__

#include "../common/types.h"
#include <cstring>

// Protokol versiyonu
#define PROTOCOL_VERSION 1

// Maksimum paket boyutları (mobil için optimize edilmiş)
#define MAX_PACKET_SIZE 2048
#define MAX_USERNAME_LENGTH 24
#define MAX_PASSWORD_LENGTH 32
#define MAX_CHAT_LENGTH 256

// Paket tipleri - Mobil optimize
enum EPacketType : BYTE
{
    // Handshake & Auth (0-9)
    HEADER_CG_HANDSHAKE = 0,
    HEADER_GC_HANDSHAKE = 1,
    HEADER_CG_LOGIN = 2,
    HEADER_GC_LOGIN_SUCCESS = 3,
    HEADER_GC_LOGIN_FAILURE = 4,

    // Character (10-19)
    HEADER_CG_CHARACTER_SELECT = 10,
    HEADER_GC_CHARACTER_INFO = 11,
    HEADER_GC_SPAWN = 12,
    HEADER_GC_DESPAWN = 13,

    // Movement (20-29) - UDP için optimize
    HEADER_CG_MOVE = 20,
    HEADER_GC_MOVE = 21,
    HEADER_CG_SYNC_POSITION = 22,
    HEADER_GC_SYNC_POSITION = 23,

    // Combat (30-39) - Düşük gecikme kritik
    HEADER_CG_ATTACK = 30,
    HEADER_GC_ATTACK = 31,
    HEADER_GC_DAMAGE = 32,
    HEADER_GC_DEATH = 33,
    HEADER_GC_HP_UPDATE = 34,

    // Chat & Social (40-49)
    HEADER_CG_CHAT = 40,
    HEADER_GC_CHAT = 41,

    // Keepalive & Sync (50-59)
    HEADER_CG_PING = 50,
    HEADER_GC_PONG = 51,
    HEADER_CG_HEARTBEAT = 52,
    HEADER_GC_HEARTBEAT = 53,

    // Game State (60-69)
    HEADER_GC_STAT_UPDATE = 60,
    HEADER_GC_LEVEL_UP = 61,
    HEADER_GC_EXP_UPDATE = 62,

    // Error & Disconnect (250-255)
    HEADER_GC_ERROR = 250,
    HEADER_CG_DISCONNECT = 254,
    HEADER_GC_DISCONNECT = 255
};

// Paket başlığı - Kompakt mobil formatı
#pragma pack(push, 1)
struct TPacketHeader
{
    BYTE type;          // Paket tipi
    WORD size;          // Toplam boyut (header dahil)
    DWORD sequence;     // Sıra numarası (packet loss tespiti için)
    DWORD timestamp;    // Sunucu zamanı (ms)

    TPacketHeader() : type(0), size(sizeof(TPacketHeader)), sequence(0), timestamp(0) {}
    TPacketHeader(BYTE t) : type(t), size(sizeof(TPacketHeader)), sequence(0), timestamp(0) {}
};

// CG: Client to Game
// GC: Game to Client

// ========== HANDSHAKE ==========
struct TPacketCGHandshake
{
    BYTE protocol_version;
    char device_id[32];
    char platform[16];  // "android", "ios", "web"
};

struct TPacketGCHandshake
{
    BYTE result;        // 0=success, 1=version mismatch
    DWORD server_time;
    char message[64];
};

// ========== LOGIN ==========
struct TPacketCGLogin
{
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char token[64];     // OAuth token (optional)
};

struct TPacketGCLoginSuccess
{
    DWORD account_id;
    BYTE character_count;
    char session_token[64];
};

struct TPacketGCLoginFailure
{
    BYTE error_code;    // 1=wrong pass, 2=banned, 3=server full
    char message[128];
};

// ========== CHARACTER ==========
struct TPacketCGCharacterSelect
{
    DWORD character_id;
};

struct TPacketGCCharacterInfo
{
    DWORD id;
    char name[MAX_USERNAME_LENGTH];
    BYTE job;
    BYTE level;

    // Stats - Kompakt format
    DWORD hp;
    DWORD max_hp;
    WORD sp;
    WORD max_sp;

    // Position
    LONG x;
    LONG y;
    BYTE dir;

    // Combat stats - Kısa format
    WORD attack;
    WORD defense;
};

struct TPacketGCSpawn
{
    DWORD id;
    char name[MAX_USERNAME_LENGTH];
    BYTE job;
    BYTE level;
    LONG x;
    LONG y;
    BYTE dir;
    DWORD hp;
    DWORD max_hp;
};

struct TPacketGCDespawn
{
    DWORD id;
    BYTE reason; // 0=logout, 1=death, 2=disconnect
};

// ========== MOVEMENT (UDP optimize) ==========
struct TPacketCGMove
{
    LONG x;
    LONG y;
    BYTE dir;
    DWORD move_time; // Client timestamp
};

struct TPacketGCMove
{
    DWORD id;
    LONG x;
    LONG y;
    BYTE dir;
    DWORD duration; // Hareket süresi (ms)
};

struct TPacketCGSyncPosition
{
    LONG x;
    LONG y;
    DWORD timestamp;
};

struct TPacketGCSyncPosition
{
    DWORD id;
    LONG x;
    LONG y;
    BYTE correction; // 0=normal, 1=correction
};

// ========== COMBAT (Düşük gecikme kritik) ==========
struct TPacketCGAttack
{
    DWORD target_id;
    BYTE attack_type; // 0=normal, 1=skill
    WORD skill_id;
};

struct TPacketGCAttack
{
    DWORD attacker_id;
    DWORD victim_id;
    BYTE attack_type;
};

struct TPacketGCDamage
{
    DWORD attacker_id;
    DWORD victim_id;
    DWORD damage;
    BYTE is_critical;
};

struct TPacketGCDeath
{
    DWORD victim_id;
    DWORD killer_id;
};

struct TPacketGCHPUpdate
{
    DWORD id;
    DWORD hp;
    DWORD max_hp;
};

// ========== CHAT ==========
struct TPacketCGChat
{
    BYTE type; // 0=normal, 1=whisper, 2=guild, 3=party
    DWORD target_id; // Whisper için
    char message[MAX_CHAT_LENGTH];
};

struct TPacketGCChat
{
    DWORD sender_id;
    char sender_name[MAX_USERNAME_LENGTH];
    BYTE type;
    char message[MAX_CHAT_LENGTH];
};

// ========== KEEPALIVE & SYNC ==========
struct TPacketCGPing
{
    DWORD client_time;
};

struct TPacketGCPong
{
    DWORD client_time;
    DWORD server_time;
};

struct TPacketCGHeartbeat
{
    DWORD client_fps;
    DWORD client_time;
};

struct TPacketGCHeartbeat
{
    DWORD server_time;
    WORD player_count;
};

// ========== STAT UPDATES ==========
struct TPacketGCStatUpdate
{
    DWORD hp;
    DWORD max_hp;
    WORD sp;
    WORD max_sp;
    DWORD gold;
};

struct TPacketGCLevelUp
{
    BYTE new_level;
    WORD stat_points;
    DWORD new_max_hp;
    WORD new_max_sp;
};

struct TPacketGCExpUpdate
{
    DWORD current_exp;
    DWORD needed_exp;
    WORD exp_percentage;
};

// ========== ERROR ==========
struct TPacketGCError
{
    WORD error_code;
    char message[128];
};

#pragma pack(pop)

// Helper: Paket oluşturma
template<typename T>
inline void MakePacket(char* buffer, BYTE type, const T& data, DWORD sequence = 0)
{
    TPacketHeader header(type);
    header.size = sizeof(TPacketHeader) + sizeof(T);
    header.sequence = sequence;
    header.timestamp = 0; // Sunucu zamanı eklenecek

    memcpy(buffer, &header, sizeof(TPacketHeader));
    memcpy(buffer + sizeof(TPacketHeader), &data, sizeof(T));
}

// Helper: Paket okuma
template<typename T>
inline bool ParsePacket(const char* buffer, WORD size, TPacketHeader& header, T& data)
{
    if (size < sizeof(TPacketHeader) + sizeof(T))
        return false;

    memcpy(&header, buffer, sizeof(TPacketHeader));
    memcpy(&data, buffer + sizeof(TPacketHeader), sizeof(T));
    return true;
}

#endif // __INC_NETWORK_PROTOCOL_H__
