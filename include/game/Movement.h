#ifndef __INC_GAME_MOVEMENT_H__
#define __INC_GAME_MOVEMENT_H__

#include "../common/types.h"
#include <vector>
#include <chrono>

// Hareket türleri
enum EMovementType
{
    MOVE_TYPE_WALK = 0,
    MOVE_TYPE_RUN = 1,
    MOVE_TYPE_DASH = 2  // Hızlı kaçış/koşu
};

// Hareket durumu
enum EMovementState
{
    MOVEMENT_STATE_IDLE = 0,
    MOVEMENT_STATE_MOVING = 1,
    MOVEMENT_STATE_ARRIVED = 2
};

// Yön hesaplaması için
struct TDirection
{
    float dx;
    float dy;
    float distance;

    TDirection() : dx(0), dy(0), distance(0) {}
};

// Hareket bilgisi
struct TMovementData
{
    TPosition start_pos;
    TPosition target_pos;
    TPosition current_pos;

    EMovementType move_type;
    EMovementState state;

    float move_speed;           // Birim/saniye
    float elapsed_time;         // Geçen süre (saniye)
    float total_distance;
    float moved_distance;

    std::chrono::steady_clock::time_point start_time;

    TMovementData()
        : move_type(MOVE_TYPE_WALK)
        , state(MOVEMENT_STATE_IDLE)
        , move_speed(100.0f)
        , elapsed_time(0.0f)
        , total_distance(0.0f)
        , moved_distance(0.0f)
    {}
};

// Path waypoint
struct TWaypoint
{
    TPosition pos;
    bool reached;

    TWaypoint() : reached(false) {}
    TWaypoint(const TPosition& p) : pos(p), reached(false) {}
};

// Hareket yönetici sınıfı
class CMovement
{
public:
    CMovement();
    ~CMovement();

    // Hareket başlat
    bool StartMove(const TPosition& from, const TPosition& to, EMovementType type = MOVE_TYPE_RUN);

    // Path ile hareket (çoklu waypoint)
    bool StartPathMove(const TPosition& from, const std::vector<TPosition>& path, EMovementType type = MOVE_TYPE_RUN);

    // Hareketi durdur
    void StopMove();

    // Hareketi güncelle (her frame çağrılmalı)
    bool UpdateMove(float delta_time);

    // Anlık pozisyonu al
    TPosition GetCurrentPosition() const { return m_moveData.current_pos; }

    // Hareket halinde mi?
    bool IsMoving() const { return m_moveData.state == MOVEMENT_STATE_MOVING; }

    // Hedefe ulaşıldı mı?
    bool HasArrived() const { return m_moveData.state == MOVEMENT_STATE_ARRIVED; }

    // Hareket hızını ayarla
    void SetMoveSpeed(float speed);
    float GetMoveSpeed() const { return m_moveData.move_speed; }

    // Kalan mesafe
    float GetRemainingDistance() const;

    // İlerleme yüzdesi
    float GetProgress() const;

    // Yön hesapla
    static TDirection CalculateDirection(const TPosition& from, const TPosition& to);

    // Mesafe hesapla
    static float CalculateDistance(const TPosition& pos1, const TPosition& pos2);

private:
    TMovementData m_moveData;
    std::vector<TWaypoint> m_path;
    size_t m_currentWaypointIndex;

    bool m_bUsingPath;

    void UpdateSingleMove(float delta_time);
    void UpdatePathMove(float delta_time);
    bool GoToNextWaypoint();
};

#endif // __INC_GAME_MOVEMENT_H__
