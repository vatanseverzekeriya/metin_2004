# Metin2 PvP - Mobil Network Protokol Dokümantasyonu

## İçindekiler

1. [Genel Bakış](#genel-bakış)
2. [Hibrit Protokol Mimarisi](#hibrit-protokol-mimarisi)
3. [WebSocket Protokolü](#websocket-protokolü)
4. [UDP Protokolü](#udp-protokolü)
5. [Paket Yapıları](#paket-yapıları)
6. [Mobil Client Örnekleri](#mobil-client-örnekleri)
7. [Performans Optimizasyonları](#performans-optimizasyonları)
8. [Güvenlik](#güvenlik)
9. [Hata Yönetimi](#hata-yönetimi)

## Genel Bakış

Metin2 PvP sunucusu, mobil cihazlar için optimize edilmiş **hibrit network protokolü** kullanır:

- **WebSocket**: Güvenilir, sıralı iletişim (Login, Chat, Inventory)
- **UDP**: Düşük gecikmeli, kayıp toleranslı iletişim (Movement, Combat)

### Neden Hibrit Protokol?

| İşlem Tipi | Protokol | Sebep |
|------------|----------|-------|
| Login, Auth | WebSocket | Güvenilirlik kritik |
| Chat, Inventory | WebSocket | Veri bütünlüğü önemli |
| Movement | UDP | Düşük gecikme kritik, kayıp tolere edilebilir |
| Combat | UDP | Real-time gereksinimleri |
| Ping/Pong | UDP | Hafif, sık çalışan |

## Hibrit Protokol Mimarisi

```
┌─────────────────────────────────────────┐
│         Mobile Client                    │
│  ┌──────────────┐  ┌─────────────────┐  │
│  │  WebSocket   │  │      UDP        │  │
│  │   Client     │  │    Client       │  │
│  └──────┬───────┘  └────────┬────────┘  │
└─────────┼────────────────────┼───────────┘
          │                    │
          │ TCP/SSL           │ UDP
          │ Port 8080          │ Port 8081
          │                    │
┌─────────▼────────────────────▼───────────┐
│         Network Manager                   │
│  ┌──────────────┐  ┌─────────────────┐  │
│  │  WebSocket   │  │   UDP Server    │  │
│  │   Server     │  │                 │  │
│  └──────┬───────┘  └────────┬────────┘  │
│         │                    │           │
│         └────────┬───────────┘           │
│                  │                       │
│         ┌────────▼────────┐              │
│         │  Game Server    │              │
│         └─────────────────┘              │
└──────────────────────────────────────────┘
```

### Protokol Seçimi (Otomatik)

```cpp
bool CNetworkManager::ShouldUseUDP(BYTE packet_type) const
{
    switch (packet_type)
    {
    case HEADER_CG_MOVE:
    case HEADER_GC_MOVE:
    case HEADER_CG_ATTACK:
    case HEADER_GC_ATTACK:
    case HEADER_GC_DAMAGE:
    case HEADER_CG_SYNC_POSITION:
        return true;  // UDP - Düşük gecikme
    default:
        return false; // WebSocket - Güvenilir
    }
}
```

## WebSocket Protokolü

### Bağlantı

```
Client → Server: ws://server_ip:8080
Server → Client: Connection Accepted
```

### Kullanım Alanları

1. **Authentication & Login**
   - Güvenli bağlantı
   - Token exchange
   - Session management

2. **Character Management**
   - Character selection
   - Inventory operations
   - Quest system

3. **Chat & Social**
   - Chat messages
   - Friend lists
   - Guild communications

### Örnek: Login İşlemi

```cpp
// C++ Server
void HandleWSLogin(DWORD session_id, const TPacketHeader& header,
                   const char* data, WORD size)
{
    TPacketCGLogin login;
    memcpy(&login, data, sizeof(TPacketCGLogin));

    if (CDBManager::Instance().CheckAccount(login.username, login.password))
    {
        TPacketGCLoginSuccess success;
        success.account_id = GetAccountID(login.username);
        CWebSocketServer::Instance().SendPacket(session_id,
            HEADER_GC_LOGIN_SUCCESS, success);
    }
}
```

```kotlin
// Android Client
fun sendLogin(username: String, password: String) {
    val buffer = ByteBuffer.allocate(256).order(ByteOrder.LITTLE_ENDIAN)

    buffer.put(HEADER_CG_LOGIN)
    buffer.putShort((8 + 56).toShort())
    buffer.putInt(++sequenceNumber)
    buffer.putInt(System.currentTimeMillis().toInt())

    buffer.put(username.toByteArray().copyOf(24))
    buffer.put(password.toByteArray().copyOf(32))

    sendWebSocket(buffer.array(), buffer.position())
}
```

```swift
// iOS Client
func sendLogin(username: String, password: String) {
    var buffer = Data()

    buffer.append(PacketType.cgLogin.rawValue)
    buffer.append(UInt16(8 + 56).littleEndianData)
    sequenceNumber += 1
    buffer.append(sequenceNumber.littleEndianData)
    buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)

    buffer.append(username.paddedData(to: 24))
    buffer.append(password.paddedData(to: 32))

    sendWebSocket(data: buffer)
}
```

## UDP Protokolü

### Bağlantı

```
Client ←→ Server: UDP Datagrams (Port 8081)
```

### Kullanım Alanları

1. **Movement & Position**
   - Real-time position updates
   - Direction changes
   - Position sync

2. **Combat**
   - Attack commands
   - Damage notifications
   - Skill usage

3. **Keepalive**
   - Ping/Pong
   - Connection health check

### Örnek: Movement İşlemi

```cpp
// C++ Server
void HandleUDPMove(const UDPEndpoint& endpoint, const TPacketHeader& header,
                   const char* data, WORD size)
{
    TPacketCGMove move;
    memcpy(&move, data, sizeof(TPacketCGMove));

    CCharacter* ch = CGameServer::Instance().FindCharacter(endpoint.character_id);
    if (ch) {
        ch->MoveTo(move.x, move.y);

        // Broadcast to nearby players
        TPacketGCMove gc_move;
        gc_move.id = ch->GetPlayerID();
        gc_move.x = move.x;
        gc_move.y = move.y;
        gc_move.dir = move.dir;

        CNetworkManager::Instance().BroadcastToNearby(
            ch->GetPosition(), 3000, HEADER_GC_MOVE,
            (const char*)&gc_move, sizeof(gc_move));
    }
}
```

```kotlin
// Android Client
fun sendMove(x: Int, y: Int, dir: Byte) {
    val buffer = ByteBuffer.allocate(64).order(ByteOrder.LITTLE_ENDIAN)

    buffer.put(HEADER_CG_MOVE)
    buffer.putShort((8 + 9).toShort())
    buffer.putInt(++sequenceNumber)
    buffer.putInt(System.currentTimeMillis().toInt())

    buffer.putInt(x)
    buffer.putInt(y)
    buffer.put(dir)

    sendUDP(buffer.array(), buffer.position())
}
```

```swift
// iOS Client
func sendMove(x: Int32, y: Int32, dir: UInt8) {
    var buffer = Data()

    buffer.append(PacketType.cgMove.rawValue)
    buffer.append(UInt16(8 + 9).littleEndianData)
    sequenceNumber += 1
    buffer.append(sequenceNumber.littleEndianData)
    buffer.append(UInt32(Date().timeIntervalSince1970 * 1000).littleEndianData)

    buffer.append(x.littleEndianData)
    buffer.append(y.littleEndianData)
    buffer.append(dir)

    sendUDP(data: buffer)
}
```

## Paket Yapıları

### Paket Başlığı (Header)

```cpp
#pragma pack(push, 1)
struct TPacketHeader
{
    BYTE  type;        // 1 byte - Paket tipi
    WORD  size;        // 2 bytes - Toplam boyut (header + body)
    DWORD sequence;    // 4 bytes - Sıra numarası
    DWORD timestamp;   // 4 bytes - Sunucu zamanı (ms)
};
#pragma pack(pop)
// Total: 11 bytes (compact for mobile)
```

### Örnek Paket: Login

```cpp
struct TPacketCGLogin
{
    char username[24];  // 24 bytes
    char password[32];  // 32 bytes
    char token[64];     // 64 bytes (optional OAuth)
};
// Total: 120 bytes
// Full packet: 11 (header) + 120 (body) = 131 bytes
```

### Örnek Paket: Move (UDP optimized)

```cpp
struct TPacketCGMove
{
    LONG x;            // 4 bytes
    LONG y;            // 4 bytes
    BYTE dir;          // 1 byte
    DWORD move_time;   // 4 bytes
};
// Total: 13 bytes
// Full packet: 11 + 13 = 24 bytes (minimal overhead!)
```

## Mobil Client Örnekleri

### Android (Kotlin)

#### Bağlantı

```kotlin
val client = NetworkClient("192.168.1.100")

client.onConnected = {
    println("Connected!")
    client.sendLogin("testuser", "password123")
}

client.connectWebSocket()
client.startUDP()
```

#### Paket Alma

```kotlin
client.onPacketReceived = { type, data ->
    when (type) {
        HEADER_GC_LOGIN_SUCCESS -> handleLoginSuccess(data)
        HEADER_GC_MOVE -> handleMove(data)
        HEADER_GC_DAMAGE -> handleDamage(data)
    }
}
```

### iOS (Swift)

#### Bağlantı

```swift
let client = NetworkClient(serverIP: "192.168.1.100")

client.onConnected = {
    print("Connected!")
    self.client.sendLogin(username: "testuser", password: "password123")
}

client.connectWebSocket()
client.startUDP()
```

#### Paket Alma

```swift
client.onPacketReceived = { type, data in
    switch type {
    case PacketType.gcLoginSuccess.rawValue:
        self.handleLoginSuccess(data: data)
    case PacketType.gcMove.rawValue:
        self.handleMove(data: data)
    case PacketType.gcDamage.rawValue:
        self.handleDamage(data: data)
    default:
        break
    }
}
```

## Performans Optimizasyonları

### 1. Paket Boyutu Optimizasyonu

**Öncesi (Geleneksel):**
```cpp
struct OldPacket {
    char player_name[256];  // 256 bytes
    double position_x;      // 8 bytes
    double position_y;      // 8 bytes
    // Total: 272 bytes
};
```

**Sonrası (Mobil Optimize):**
```cpp
struct NewPacket {
    LONG x;  // 4 bytes (cm cinsinden)
    LONG y;  // 4 bytes
    BYTE dir; // 1 byte
    // Total: 9 bytes (30x daha küçük!)
};
```

### 2. Delta Encoding

Sadece değişiklikleri gönder:

```cpp
struct PositionUpdate {
    BYTE flags;      // Hangi alanlar değişti?
    SHORT delta_x;   // -32768 to 32767 (yeterli)
    SHORT delta_y;
    // 5 bytes instead of 9!
};
```

### 3. Adaptive Update Rate

```cpp
// Bağlantı kalitesine göre güncelleme sıklığı
if (packet_loss > 5%) {
    update_interval = 200ms;  // Daha az paket
} else if (latency < 50ms) {
    update_interval = 50ms;   // Smooth updates
} else {
    update_interval = 100ms;  // Balanced
}
```

### 4. Priority Queue

```cpp
enum PacketPriority {
    LOW = 0,      // Chat
    NORMAL = 1,   // Login
    HIGH = 2,     // Movement
    CRITICAL = 3  // Combat, Death
};

// Critical paketler önce gönderilir
```

### 5. Batch Updates

```cpp
// Birden fazla pozisyon güncellemesini tek pakette gönder
struct BatchPositionUpdate {
    BYTE count;
    struct {
        DWORD character_id;
        LONG x, y;
        BYTE dir;
    } updates[10];
};
```

## Güvenlik

### 1. Packet Validation

```cpp
bool ValidatePacket(const TPacketHeader& header, WORD received_size)
{
    // Size check
    if (header.size != received_size)
        return false;

    // Sequence check (detect replay attacks)
    if (header.sequence <= last_sequence)
        return false;

    // Timestamp check (detect old packets)
    if (abs(header.timestamp - GetCurrentTime()) > 5000)
        return false;

    return true;
}
```

### 2. Rate Limiting

```cpp
// Flood koruması
struct RateLimiter {
    DWORD packets_per_second;
    DWORD last_reset_time;
    DWORD packet_count;

    bool CheckLimit() {
        DWORD now = GetCurrentTime();
        if (now - last_reset_time > 1000) {
            last_reset_time = now;
            packet_count = 0;
        }

        packet_count++;
        return packet_count < packets_per_second;
    }
};
```

### 3. Encryption (Optional)

```cpp
// WebSocket üzerinden TLS/SSL
ws://server -> wss://server (SSL)

// UDP için lightweight encryption
struct EncryptedUDPPacket {
    BYTE iv[16];        // Initialization vector
    BYTE data[2032];    // Encrypted data
};
```

## Hata Yönetimi

### Packet Loss Detection

```cpp
struct PacketLossTracker {
    DWORD expected_sequence;
    DWORD lost_packets;

    void OnPacketReceived(DWORD sequence) {
        if (sequence > expected_sequence) {
            lost_packets += (sequence - expected_sequence);
        }
        expected_sequence = sequence + 1;
    }

    float GetLossRate() {
        return (float)lost_packets / expected_sequence;
    }
};
```

### Timeout Handling

```cpp
const DWORD TIMEOUT_MS = 30000; // 30 seconds

if (current_time - last_packet_time > TIMEOUT_MS) {
    // Client timeout, disconnect
    DisconnectClient(session_id);
}
```

### Reconnection

```kotlin
// Android
fun handleDisconnect() {
    reconnectAttempts++

    if (reconnectAttempts < MAX_RETRIES) {
        val delay = min(2000 * reconnectAttempts, 10000)
        Handler().postDelayed({
            connectWebSocket()
        }, delay)
    }
}
```

## Latency Optimization

### Ping/Pong System

```cpp
// Client gönderir
struct TPacketCGPing {
    DWORD client_time;
};

// Server yanıtlar
struct TPacketGCPong {
    DWORD client_time;  // Client'ın gönderdiği zaman
    DWORD server_time;  // Server zamanı
};

// Client hesaplar
DWORD latency = current_time - client_time;
DWORD server_offset = server_time - (client_time + latency / 2);
```

### Client-Side Prediction

```kotlin
// Client tarafında hareketi hemen uygula (smooth)
fun moveCharacter(x: Int, y: Int) {
    // Immediately move on screen
    character.setPosition(x, y)

    // Send to server (will sync if needed)
    sendMove(x, y, character.direction)
}
```

### Server Reconciliation

```cpp
// Server pozisyonu client'ınkinden çok farklıysa düzelt
if (distance(server_pos, client_pos) > TOLERANCE) {
    TPacketGCSyncPosition sync;
    sync.x = server_pos.x;
    sync.y = server_pos.y;
    sync.correction = 1;  // Bu bir düzeltme
    SendPacket(character_id, HEADER_GC_SYNC_POSITION, sync);
}
```

## Benchmark & Metrics

### Beklenen Performans

| Metrik | Hedef | Mükemmel |
|--------|-------|----------|
| Latency (WiFi) | < 50ms | < 30ms |
| Latency (4G) | < 100ms | < 80ms |
| Packet Loss | < 2% | < 0.5% |
| Bandwidth (Idle) | < 5 KB/s | < 3 KB/s |
| Bandwidth (Active) | < 20 KB/s | < 15 KB/s |

### Monitoring

```cpp
struct NetworkMetrics {
    DWORD packets_sent;
    DWORD packets_received;
    DWORD packets_lost;
    DWORD bytes_sent;
    DWORD bytes_received;
    DWORD avg_latency_ms;
    float packet_loss_rate;
};
```

## Best Practices

### ✅ DO

- Her zaman little-endian kullan
- Paket boyutlarını minimize et
- Critical işlemler için WebSocket kullan
- Real-time işlemler için UDP kullan
- Packet loss'u tolere et (interpolation)
- Client-side prediction kullan
- Rate limiting uygula

### ❌ DON'T

- Büyük paketler gönderme (>2KB)
- Her frame'de pozisyon gönderme (gereksiz)
- UDP'de guaranteed delivery bekleme
- Şifrelenmemiş kanaldan şifre gönderme
- Sync operasyonlar kullanma (blocking)
- Client verilerine körü körüne güvenme

## Troubleshooting

### Problem: Yüksek Latency

**Çözüm:**
- UDP kullanımını kontrol et
- Paket boyutlarını azalt
- Update rate'i düşür
- Server yükünü kontrol et

### Problem: Packet Loss

**Çözüm:**
- WiFi/4G bağlantısını kontrol et
- UDP packet boyutunu küçült (<1400 bytes)
- Update interval'i artır
- Interpolation kullan

### Problem: Senkronizasyon Hatası

**Çözüm:**
- Timestamp kullan
- Server authoritative yap
- Client prediction + reconciliation
- Periodic sync packets gönder

## Sonuç

Hibrit protokol sistemi:
- **WebSocket**: Güvenilir iletişim
- **UDP**: Düşük gecikmeli iletişim

Bu kombinasyon mobil MMORPG için idealdir!

### İletişim

Sorular için: [GitHub Issues](https://github.com/yourusername/metin2-pvp-server/issues)
