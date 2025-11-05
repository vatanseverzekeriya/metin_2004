#include "../../include/network/NetworkManager.h"
#include "../../include/game/GameServer.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

/**
 * Optimize edilmiş Network Manager
 *
 * Optimizasyonlar:
 * - Area of Interest (AOI) broadcasting
 * - Packet batching
 * - Delta compression
 * - Adaptive tick rate
 * - Interest management
 * - Priority-based sending
 */

// ========== AREA OF INTEREST SYSTEM ==========

struct AOICell {
    DWORD cell_x;
    DWORD cell_y;
    std::vector<DWORD> character_ids;

    bool operator==(const AOICell& other) const {
        return cell_x == other.cell_x && cell_y == other.cell_y;
    }
};

class CAOIManager {
private:
    static const DWORD CELL_SIZE = 5000; // 50 meters
    std::unordered_map<DWORD, std::vector<AOICell>> m_characterCells;
    std::mutex m_mutex;

public:
    static CAOIManager& Instance() {
        static CAOIManager instance;
        return instance;
    }

    DWORD GetCellX(LONG x) const {
        return x / CELL_SIZE;
    }

    DWORD GetCellY(LONG y) const {
        return y / CELL_SIZE;
    }

    std::vector<AOICell> GetNearbyCells(LONG x, LONG y, DWORD range = 1) {
        DWORD center_x = GetCellX(x);
        DWORD center_y = GetCellY(y);

        std::vector<AOICell> cells;
        for (DWORD dx = 0; dx <= range; dx++) {
            for (DWORD dy = 0; dy <= range; dy++) {
                cells.push_back({center_x + dx, center_y + dy, {}});
                if (dx > 0) cells.push_back({center_x - dx, center_y + dy, {}});
                if (dy > 0) cells.push_back({center_x + dx, center_y - dy, {}});
                if (dx > 0 && dy > 0) cells.push_back({center_x - dx, center_y - dy, {}});
            }
        }

        return cells;
    }

    std::vector<DWORD> GetNearbyCharacters(LONG x, LONG y, DWORD range) {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<AOICell> cells = GetNearbyCells(x, y, range / CELL_SIZE + 1);
        std::vector<DWORD> characters;

        for (const auto& cell : cells) {
            auto key = cell.cell_x * 10000 + cell.cell_y;
            // Find characters in this cell
            for (const auto& [char_id, char_cells] : m_characterCells) {
                for (const auto& char_cell : char_cells) {
                    if (char_cell.cell_x == cell.cell_x &&
                        char_cell.cell_y == cell.cell_y) {
                        characters.push_back(char_id);
                        break;
                    }
                }
            }
        }

        return characters;
    }

    void UpdateCharacterPosition(DWORD character_id, LONG x, LONG y) {
        std::lock_guard<std::mutex> lock(m_mutex);

        AOICell new_cell = {GetCellX(x), GetCellY(y), {}};

        auto& cells = m_characterCells[character_id];

        // Check if moved to new cell
        if (cells.empty() || !(cells[0] == new_cell)) {
            cells.clear();
            cells.push_back(new_cell);
        }
    }

    void RemoveCharacter(DWORD character_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_characterCells.erase(character_id);
    }
};

// ========== PACKET BATCHING SYSTEM ==========

struct BatchedPacket {
    BYTE type;
    std::vector<char> data;
    DWORD target_character_id;
    DWORD timestamp;
};

class CPacketBatcher {
private:
    std::vector<BatchedPacket> m_pendingPackets;
    std::mutex m_mutex;
    std::thread m_flushThread;
    bool m_running = false;

    static const DWORD FLUSH_INTERVAL_MS = 16; // ~60 FPS
    static const DWORD MAX_BATCH_SIZE = 50;

public:
    static CPacketBatcher& Instance() {
        static CPacketBatcher instance;
        return instance;
    }

    void Start() {
        m_running = true;
        m_flushThread = std::thread([this]() {
            while (m_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(FLUSH_INTERVAL_MS));
                Flush();
            }
        });
    }

    void Stop() {
        m_running = false;
        if (m_flushThread.joinable()) {
            m_flushThread.join();
        }
    }

    void AddPacket(BYTE type, const char* data, WORD size, DWORD target = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);

        BatchedPacket packet;
        packet.type = type;
        packet.data.assign(data, data + size);
        packet.target_character_id = target;
        packet.timestamp = GetCurrentTime();

        m_pendingPackets.push_back(packet);

        // Force flush if too many packets
        if (m_pendingPackets.size() >= MAX_BATCH_SIZE) {
            Flush();
        }
    }

    void Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_pendingPackets.empty())
            return;

        // Group packets by type
        std::unordered_map<BYTE, std::vector<BatchedPacket>> grouped;
        for (const auto& packet : m_pendingPackets) {
            grouped[packet.type].push_back(packet);
        }

        // Send batched packets
        for (const auto& [type, packets] : grouped) {
            if (CanBatch(type)) {
                SendBatchedPackets(type, packets);
            } else {
                // Send individually
                for (const auto& packet : packets) {
                    CNetworkManager::Instance().SendToCharacter(
                        packet.target_character_id, type,
                        packet.data.data(), packet.data.size()
                    );
                }
            }
        }

        m_pendingPackets.clear();
    }

private:
    bool CanBatch(BYTE type) const {
        // Position updates can be batched
        return type == HEADER_GC_MOVE || type == HEADER_GC_SPAWN;
    }

    void SendBatchedPackets(BYTE type, const std::vector<BatchedPacket>& packets) {
        // Create batch packet
        std::vector<char> batch_data;

        // Batch header
        TPacketHeader header;
        header.type = type | 0x80; // Set batch flag
        header.size = 0; // Will be calculated
        header.sequence = 0;
        header.timestamp = GetCurrentTime();

        batch_data.insert(batch_data.end(),
                         (char*)&header, (char*)&header + sizeof(header));

        // Add count
        WORD count = packets.size();
        batch_data.insert(batch_data.end(),
                         (char*)&count, (char*)&count + sizeof(count));

        // Add all packets
        for (const auto& packet : packets) {
            batch_data.insert(batch_data.end(),
                            packet.data.begin(), packet.data.end());
        }

        // Update size
        ((TPacketHeader*)batch_data.data())->size = batch_data.size();

        // Broadcast to all
        CNetworkManager::Instance().BroadcastToAll(type,
            batch_data.data(), batch_data.size());
    }

    DWORD GetCurrentTime() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};

// ========== DELTA COMPRESSION ==========

class CDeltaCompression {
private:
    struct LastPosition {
        LONG x;
        LONG y;
        BYTE dir;
        DWORD timestamp;
    };

    std::unordered_map<DWORD, LastPosition> m_lastPositions;
    std::mutex m_mutex;

public:
    static CDeltaCompression& Instance() {
        static CDeltaCompression instance;
        return instance;
    }

    bool ShouldSendDelta(DWORD character_id, LONG x, LONG y) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_lastPositions.find(character_id);
        if (it == m_lastPositions.end())
            return false;

        const auto& last = it->second;

        // Check if delta fits in 16-bit
        LONG dx = x - last.x;
        LONG dy = y - last.y;

        return abs(dx) < 32768 && abs(dy) < 32768;
    }

    void CreateDeltaPacket(DWORD character_id, LONG x, LONG y, BYTE dir,
                          char* out_buffer, WORD& out_size) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto& last = m_lastPositions[character_id];

        SHORT delta_x = (SHORT)(x - last.x);
        SHORT delta_y = (SHORT)(y - last.y);

        TPacketHeader header;
        header.type = HEADER_GC_MOVE | 0x80; // Delta flag
        header.size = sizeof(TPacketHeader) + sizeof(DWORD) + sizeof(SHORT) * 2 + sizeof(BYTE);
        header.sequence = 0;
        header.timestamp = GetCurrentTime();

        char* ptr = out_buffer;
        memcpy(ptr, &header, sizeof(header)); ptr += sizeof(header);
        memcpy(ptr, &character_id, sizeof(DWORD)); ptr += sizeof(DWORD);
        memcpy(ptr, &delta_x, sizeof(SHORT)); ptr += sizeof(SHORT);
        memcpy(ptr, &delta_y, sizeof(SHORT)); ptr += sizeof(SHORT);
        memcpy(ptr, &dir, sizeof(BYTE)); ptr += sizeof(BYTE);

        out_size = header.size;

        // Update last position
        last.x = x;
        last.y = y;
        last.dir = dir;
        last.timestamp = GetCurrentTime();
    }

    void CreateFullPacket(DWORD character_id, LONG x, LONG y, BYTE dir,
                         char* out_buffer, WORD& out_size) {
        std::lock_guard<std::mutex> lock(m_mutex);

        TPacketGCMove move;
        move.id = character_id;
        move.x = x;
        move.y = y;
        move.dir = dir;
        move.duration = 500;

        TPacketHeader header;
        header.type = HEADER_GC_MOVE;
        header.size = sizeof(TPacketHeader) + sizeof(TPacketGCMove);
        header.sequence = 0;
        header.timestamp = GetCurrentTime();

        memcpy(out_buffer, &header, sizeof(header));
        memcpy(out_buffer + sizeof(header), &move, sizeof(move));

        out_size = header.size;

        // Update last position
        auto& last = m_lastPositions[character_id];
        last.x = x;
        last.y = y;
        last.dir = dir;
        last.timestamp = GetCurrentTime();
    }

    void RemoveCharacter(DWORD character_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastPositions.erase(character_id);
    }

private:
    DWORD GetCurrentTime() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};

// ========== ADAPTIVE TICK RATE ==========

class CAdaptiveTickRate {
private:
    struct ClientMetrics {
        DWORD session_id;
        DWORD avg_latency;
        float packet_loss;
        DWORD last_update;
        DWORD update_interval; // Dynamic
    };

    std::unordered_map<DWORD, ClientMetrics> m_clientMetrics;
    std::mutex m_mutex;

    static const DWORD MIN_INTERVAL = 50;  // 20 updates/sec
    static const DWORD MAX_INTERVAL = 200; // 5 updates/sec
    static const DWORD DEFAULT_INTERVAL = 100; // 10 updates/sec

public:
    static CAdaptiveTickRate& Instance() {
        static CAdaptiveTickRate instance;
        return instance;
    }

    void RegisterClient(DWORD session_id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        ClientMetrics metrics;
        metrics.session_id = session_id;
        metrics.avg_latency = 100;
        metrics.packet_loss = 0.0f;
        metrics.last_update = GetCurrentTime();
        metrics.update_interval = DEFAULT_INTERVAL;

        m_clientMetrics[session_id] = metrics;
    }

    void UpdateMetrics(DWORD session_id, DWORD latency, float packet_loss) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_clientMetrics.find(session_id);
        if (it == m_clientMetrics.end())
            return;

        auto& metrics = it->second;
        metrics.avg_latency = (metrics.avg_latency * 9 + latency) / 10;
        metrics.packet_loss = packet_loss;

        // Adjust interval based on metrics
        if (packet_loss > 5.0f || latency > 200) {
            metrics.update_interval = MAX_INTERVAL; // Poor connection
        } else if (packet_loss > 2.0f || latency > 100) {
            metrics.update_interval = (MAX_INTERVAL + DEFAULT_INTERVAL) / 2;
        } else if (latency < 50) {
            metrics.update_interval = MIN_INTERVAL; // Excellent connection
        } else {
            metrics.update_interval = DEFAULT_INTERVAL;
        }
    }

    bool ShouldSendUpdate(DWORD session_id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_clientMetrics.find(session_id);
        if (it == m_clientMetrics.end())
            return true;

        auto& metrics = it->second;
        DWORD now = GetCurrentTime();

        if (now - metrics.last_update >= metrics.update_interval) {
            metrics.last_update = now;
            return true;
        }

        return false;
    }

    void RemoveClient(DWORD session_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clientMetrics.erase(session_id);
    }

private:
    DWORD GetCurrentTime() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};

// ========== OPTIMIZED NETWORK MANAGER EXTENSIONS ==========

void CNetworkManager::InitializeOptimizations()
{
    std::cout << "Initializing network optimizations..." << std::endl;

    // Start packet batcher
    CPacketBatcher::Instance().Start();

    std::cout << "Optimizations initialized!" << std::endl;
}

void CNetworkManager::ShutdownOptimizations()
{
    CPacketBatcher::Instance().Stop();
}

void CNetworkManager::OnPlayerSelectCharacterOptimized(DWORD session_id, DWORD character_id)
{
    OnPlayerSelectCharacter(session_id, character_id);

    // Register for adaptive tick rate
    CAdaptiveTickRate::Instance().RegisterClient(session_id);
}

void CNetworkManager::OnPlayerDisconnectOptimized(DWORD session_id, DWORD character_id)
{
    OnPlayerDisconnect(session_id);

    // Cleanup optimizations
    if (character_id > 0) {
        CAOIManager::Instance().RemoveCharacter(character_id);
        CDeltaCompression::Instance().RemoveCharacter(character_id);
    }
    CAdaptiveTickRate::Instance().RemoveClient(session_id);
}

void CNetworkManager::HandleUDPMoveOptimized(const UDPEndpoint& endpoint,
                                            const TPacketHeader& header,
                                            const char* data, WORD size)
{
    if (size < sizeof(TPacketCGMove))
        return;

    TPacketCGMove move;
    memcpy(&move, data, sizeof(TPacketCGMove));

    if (endpoint.character_id == 0)
        return;

    CCharacter* ch = CGameServer::Instance().FindCharacter(endpoint.character_id);
    if (!ch)
        return;

    // Update position
    ch->MoveTo(move.x, move.y);

    // Update AOI
    CAOIManager::Instance().UpdateCharacterPosition(endpoint.character_id, move.x, move.y);

    // Get session for adaptive rate
    DWORD session_id = GetSessionByCharacter(endpoint.character_id);
    if (!CAdaptiveTickRate::Instance().ShouldSendUpdate(session_id)) {
        // Skip this update due to rate limiting
        return;
    }

    // Create packet (delta or full)
    char packet_buffer[256];
    WORD packet_size = 0;

    if (CDeltaCompression::Instance().ShouldSendDelta(endpoint.character_id, move.x, move.y)) {
        CDeltaCompression::Instance().CreateDeltaPacket(
            endpoint.character_id, move.x, move.y, move.dir,
            packet_buffer, packet_size
        );
    } else {
        CDeltaCompression::Instance().CreateFullPacket(
            endpoint.character_id, move.x, move.y, move.dir,
            packet_buffer, packet_size
        );
    }

    // Get nearby characters using AOI
    std::vector<DWORD> nearby = CAOIManager::Instance().GetNearbyCharacters(
        move.x, move.y, 3000
    );

    // Batch broadcast to nearby
    for (DWORD nearby_id : nearby) {
        if (nearby_id != endpoint.character_id) {
            CPacketBatcher::Instance().AddPacket(
                HEADER_GC_MOVE, packet_buffer, packet_size, nearby_id
            );
        }
    }
}

void CNetworkManager::BroadcastToNearbyOptimized(const TPosition& pos, DWORD range,
                                                BYTE packet_type, const char* data, WORD size)
{
    // Use AOI for efficient nearby search
    std::vector<DWORD> nearby = CAOIManager::Instance().GetNearbyCharacters(
        pos.x, pos.y, range
    );

    // Batch the packets
    for (DWORD character_id : nearby) {
        DWORD session_id = GetSessionByCharacter(character_id);

        // Check adaptive rate
        if (CAdaptiveTickRate::Instance().ShouldSendUpdate(session_id)) {
            CPacketBatcher::Instance().AddPacket(packet_type, data, size, character_id);
        }
    }
}

// ========== INTEREST MANAGEMENT ==========

class CInterestManager {
private:
    struct Interest {
        float priority;
        DWORD last_update;
    };

    std::unordered_map<DWORD, std::unordered_map<DWORD, Interest>> m_interests;
    std::mutex m_mutex;

public:
    static CInterestManager& Instance() {
        static CInterestManager instance;
        return instance;
    }

    float CalculatePriority(CCharacter* observer, CCharacter* target) {
        if (!observer || !target)
            return 0.0f;

        // Distance-based priority
        LONG dx = observer->GetPosition().x - target->GetPosition().x;
        LONG dy = observer->GetPosition().y - target->GetPosition().y;
        float distance = sqrt(dx * dx + dy * dy);

        // Closer = higher priority
        float priority = 1.0f / (distance / 1000.0f + 1.0f);

        // In combat = higher priority
        if (observer->IsInCombatWith(target->GetPlayerID())) {
            priority *= 3.0f;
        }

        // In party = higher priority
        if (observer->IsInSameParty(target->GetPlayerID())) {
            priority *= 2.0f;
        }

        return priority;
    }

    bool ShouldUpdate(DWORD observer_id, DWORD target_id, float priority) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto& interests = m_interests[observer_id];
        auto& interest = interests[target_id];

        DWORD now = GetCurrentTime();

        // High priority: update more frequently
        DWORD interval = (priority > 2.0f) ? 50 :
                        (priority > 1.0f) ? 100 : 200;

        if (now - interest.last_update >= interval) {
            interest.priority = priority;
            interest.last_update = now;
            return true;
        }

        return false;
    }

    void RemoveObserver(DWORD observer_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_interests.erase(observer_id);
    }

private:
    DWORD GetCurrentTime() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
};

// ========== USAGE EXAMPLE ==========

void CNetworkManager::BroadcastWithInterestManagement(CCharacter* source,
                                                      BYTE packet_type,
                                                      const char* data, WORD size)
{
    std::vector<DWORD> nearby = CAOIManager::Instance().GetNearbyCharacters(
        source->GetPosition().x, source->GetPosition().y, 5000
    );

    for (DWORD nearby_id : nearby) {
        CCharacter* target = CGameServer::Instance().FindCharacter(nearby_id);
        if (!target)
            continue;

        float priority = CInterestManager::Instance().CalculatePriority(target, source);

        if (CInterestManager::Instance().ShouldUpdate(nearby_id, source->GetPlayerID(), priority)) {
            SendToCharacter(nearby_id, packet_type, data, size);
        }
    }
}

void CNetworkManager::HandleUDPAttackOptimized(const UDPEndpoint& endpoint,
                                              const TPacketHeader& header,
                                              const char* data, WORD size)
{
    if (size < sizeof(TPacketCGAttack))
        return;

    TPacketCGAttack attack;
    memcpy(&attack, data, sizeof(TPacketCGAttack));

    if (endpoint.character_id == 0)
        return;

    CCharacter* attacker = CGameServer::Instance().FindCharacter(endpoint.character_id);
    CCharacter* victim = CGameServer::Instance().FindCharacter(attack.target_id);

    if (!attacker || !victim)
        return;

    if (attacker->Attack(victim)) {
        DWORD damage = attacker->CalculateDamage(victim);

        TPacketGCDamage gc_damage;
        gc_damage.attacker_id = attacker->GetPlayerID();
        gc_damage.victim_id = victim->GetPlayerID();
        gc_damage.damage = damage;
        gc_damage.is_critical = 0;

        // Use interest management for damage broadcast
        BroadcastWithInterestManagement(attacker, HEADER_GC_DAMAGE,
                                       (const char*)&gc_damage, sizeof(gc_damage));
    }
}
