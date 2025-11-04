/**
 * Metin2 PvP Mobile Protocol
 * Platform-independent definitions
 *
 * Compatible with:
 * - Android (Java/Kotlin)
 * - iOS (Swift/Objective-C)
 * - Unity (C#)
 * - Unreal Engine (C++)
 */

#ifndef __MOBILE_PROTOCOL_H__
#define __MOBILE_PROTOCOL_H__

#include <stdint.h>

// Cross-platform types
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  i32;

// Paket boyut limitleri - Mobil optimize
#define MOBILE_MAX_PACKET_SIZE 2048
#define MOBILE_USERNAME_LENGTH 24
#define MOBILE_PASSWORD_LENGTH 32
#define MOBILE_CHAT_LENGTH 256
#define MOBILE_TOKEN_LENGTH 64

// Paket öncelikleri (QoS)
typedef enum {
    PRIORITY_LOW = 0,       // Chat, inventory
    PRIORITY_NORMAL = 1,    // Login, character select
    PRIORITY_HIGH = 2,      // Movement, attack
    PRIORITY_CRITICAL = 3   // Death, disconnect
} PacketPriority;

// Protokol türleri
typedef enum {
    PROTOCOL_WEBSOCKET = 0, // Güvenilir, sıralı
    PROTOCOL_UDP = 1        // Hızlı, kayıp toleranslı
} ProtocolType;

// Paket tipleri ve protokol eşleşmesi
typedef struct {
    u8 packet_type;
    ProtocolType protocol;
    PacketPriority priority;
    const char* name;
} PacketInfo;

// Paket bilgileri tablosu
static const PacketInfo PACKET_INFO_TABLE[] = {
    // Auth & Handshake - WebSocket
    {0,   PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "HANDSHAKE"},
    {1,   PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "HANDSHAKE_RESPONSE"},
    {2,   PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "LOGIN"},
    {3,   PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "LOGIN_SUCCESS"},
    {4,   PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "LOGIN_FAILURE"},

    // Character - WebSocket
    {10,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "CHARACTER_SELECT"},
    {11,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "CHARACTER_INFO"},
    {12,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "SPAWN"},
    {13,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "DESPAWN"},

    // Movement - UDP (Düşük gecikme kritik)
    {20,  PROTOCOL_UDP, PRIORITY_HIGH, "MOVE"},
    {21,  PROTOCOL_UDP, PRIORITY_HIGH, "MOVE_BROADCAST"},
    {22,  PROTOCOL_UDP, PRIORITY_HIGH, "SYNC_POSITION"},
    {23,  PROTOCOL_UDP, PRIORITY_HIGH, "SYNC_POSITION_RESPONSE"},

    // Combat - UDP (Düşük gecikme kritik)
    {30,  PROTOCOL_UDP, PRIORITY_HIGH, "ATTACK"},
    {31,  PROTOCOL_UDP, PRIORITY_HIGH, "ATTACK_BROADCAST"},
    {32,  PROTOCOL_UDP, PRIORITY_CRITICAL, "DAMAGE"},
    {33,  PROTOCOL_UDP, PRIORITY_CRITICAL, "DEATH"},
    {34,  PROTOCOL_UDP, PRIORITY_HIGH, "HP_UPDATE"},

    // Chat - WebSocket
    {40,  PROTOCOL_WEBSOCKET, PRIORITY_LOW, "CHAT"},
    {41,  PROTOCOL_WEBSOCKET, PRIORITY_LOW, "CHAT_BROADCAST"},

    // Keepalive - UDP (Hafif)
    {50,  PROTOCOL_UDP, PRIORITY_LOW, "PING"},
    {51,  PROTOCOL_UDP, PRIORITY_LOW, "PONG"},
    {52,  PROTOCOL_UDP, PRIORITY_LOW, "HEARTBEAT"},
    {53,  PROTOCOL_UDP, PRIORITY_LOW, "HEARTBEAT_RESPONSE"},

    // Game State - WebSocket
    {60,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "STAT_UPDATE"},
    {61,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "LEVEL_UP"},
    {62,  PROTOCOL_WEBSOCKET, PRIORITY_NORMAL, "EXP_UPDATE"},
};

#define PACKET_INFO_COUNT (sizeof(PACKET_INFO_TABLE) / sizeof(PacketInfo))

// Helper: Paket bilgisi al
static inline const PacketInfo* GetPacketInfo(u8 packet_type) {
    for (u32 i = 0; i < PACKET_INFO_COUNT; i++) {
        if (PACKET_INFO_TABLE[i].packet_type == packet_type) {
            return &PACKET_INFO_TABLE[i];
        }
    }
    return NULL;
}

// Mobil optimize paket başlığı
#pragma pack(push, 1)
typedef struct {
    u8  type;           // 1 byte
    u16 size;           // 2 bytes - Little endian
    u32 sequence;       // 4 bytes - Packet loss detection
    u32 timestamp;      // 4 bytes - Milliseconds
    // Total: 11 bytes (compressed header)
} MobilePacketHeader;
#pragma pack(pop)

// Kompresiyon bayrakları
#define PACKET_FLAG_COMPRESSED   0x01
#define PACKET_FLAG_ENCRYPTED    0x02
#define PACKET_FLAG_FRAGMENTED   0x04

// Bağlantı kalitesi metrikleri
typedef struct {
    u32 packets_sent;
    u32 packets_received;
    u32 packets_lost;
    u32 bytes_sent;
    u32 bytes_received;
    u32 avg_latency_ms;
    u32 peak_latency_ms;
    float packet_loss_rate; // 0.0 - 1.0
    float jitter_ms;        // Latency variation
} NetworkMetrics;

// Bağlantı durumu
typedef enum {
    CONNECTION_STATE_DISCONNECTED = 0,
    CONNECTION_STATE_CONNECTING = 1,
    CONNECTION_STATE_CONNECTED = 2,
    CONNECTION_STATE_AUTHENTICATED = 3,
    CONNECTION_STATE_IN_GAME = 4,
    CONNECTION_STATE_DISCONNECTING = 5
} ConnectionState;

// Mobil özel optimizasyonlar
typedef struct {
    u8 enable_compression;      // Data compression
    u8 enable_delta_encoding;   // Only send changes
    u8 adaptive_quality;        // Auto-adjust based on connection
    u8 battery_saving_mode;     // Reduce packet frequency
    u32 max_packet_rate;        // Packets per second limit
    u32 update_interval_ms;     // Position update interval
} MobileOptimizations;

// Platform bilgisi
typedef struct {
    char device_id[32];
    char platform[16];      // "android", "ios", "web"
    char os_version[16];
    char app_version[16];
    u8 device_type;         // 0=phone, 1=tablet, 2=web
    u8 network_type;        // 0=wifi, 1=4g, 2=5g, 3=unknown
} MobilePlatformInfo;

// Hata kodları
#define ERROR_NONE                  0
#define ERROR_INVALID_PACKET        1
#define ERROR_PACKET_TOO_LARGE      2
#define ERROR_SEQUENCE_MISMATCH     3
#define ERROR_TIMEOUT               4
#define ERROR_AUTHENTICATION_FAILED 5
#define ERROR_SERVER_FULL           6
#define ERROR_VERSION_MISMATCH      7
#define ERROR_BANNED                8

// Network tip tavsiyesi
static inline const char* GetNetworkTypeRecommendation(u8 network_type) {
    switch (network_type) {
        case 0: return "WiFi - Full quality, all features enabled";
        case 1: return "4G - Good quality, minor optimizations";
        case 2: return "5G - Excellent quality, low latency mode";
        case 3: return "Unknown - Conservative mode, reduced quality";
        default: return "Unknown network type";
    }
}

// Battery saving recommendations
static inline u32 GetRecommendedUpdateInterval(u8 battery_level) {
    if (battery_level > 50) {
        return 50;  // 50ms = 20 updates/sec (smooth)
    } else if (battery_level > 20) {
        return 100; // 100ms = 10 updates/sec (balanced)
    } else {
        return 200; // 200ms = 5 updates/sec (power saving)
    }
}

#endif // __MOBILE_PROTOCOL_H__
