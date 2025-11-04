#include "../../include/game/Movement.h"
#include <cmath>
#include <iostream>

CMovement::CMovement()
    : m_currentWaypointIndex(0)
    , m_bUsingPath(false)
{
}

CMovement::~CMovement()
{
}

bool CMovement::StartMove(const TPosition& from, const TPosition& to, EMovementType type)
{
    // Hareket verilerini başlat
    m_moveData.start_pos = from;
    m_moveData.target_pos = to;
    m_moveData.current_pos = from;
    m_moveData.move_type = type;
    m_moveData.state = MOVEMENT_STATE_MOVING;
    m_moveData.start_time = std::chrono::steady_clock::now();
    m_moveData.elapsed_time = 0.0f;
    m_moveData.moved_distance = 0.0f;

    // Toplam mesafeyi hesapla
    m_moveData.total_distance = CalculateDistance(from, to);

    if (m_moveData.total_distance < 1.0f)
    {
        m_moveData.state = MOVEMENT_STATE_ARRIVED;
        return false;
    }

    // Hareket türüne göre hız ayarla
    switch (type)
    {
        case MOVE_TYPE_WALK:
            m_moveData.move_speed = 50.0f;  // Yavaş yürüme
            break;
        case MOVE_TYPE_RUN:
            m_moveData.move_speed = 150.0f; // Normal koşma
            break;
        case MOVE_TYPE_DASH:
            m_moveData.move_speed = 300.0f; // Hızlı dash
            break;
    }

    m_bUsingPath = false;
    m_path.clear();

    return true;
}

bool CMovement::StartPathMove(const TPosition& from, const std::vector<TPosition>& path, EMovementType type)
{
    if (path.empty())
        return false;

    m_path.clear();
    m_path.reserve(path.size());

    for (const auto& pos : path)
    {
        m_path.push_back(TWaypoint(pos));
    }

    m_currentWaypointIndex = 0;
    m_bUsingPath = true;
    m_moveData.move_type = type;

    // İlk waypoint'e hareket başlat
    return StartMove(from, m_path[0].pos, type);
}

void CMovement::StopMove()
{
    m_moveData.state = MOVEMENT_STATE_IDLE;
    m_bUsingPath = false;
}

bool CMovement::UpdateMove(float delta_time)
{
    if (m_moveData.state != MOVEMENT_STATE_MOVING)
        return false;

    if (m_bUsingPath)
        UpdatePathMove(delta_time);
    else
        UpdateSingleMove(delta_time);

    return m_moveData.state == MOVEMENT_STATE_MOVING;
}

void CMovement::UpdateSingleMove(float delta_time)
{
    m_moveData.elapsed_time += delta_time;

    // Bu frame'de ne kadar hareket edeceğiz
    float move_distance = m_moveData.move_speed * delta_time;

    // Kalan mesafe
    float remaining = m_moveData.total_distance - m_moveData.moved_distance;

    if (move_distance >= remaining)
    {
        // Hedefe ulaştık
        m_moveData.current_pos = m_moveData.target_pos;
        m_moveData.moved_distance = m_moveData.total_distance;
        m_moveData.state = MOVEMENT_STATE_ARRIVED;
        return;
    }

    // Yönü hesapla
    TDirection dir = CalculateDirection(m_moveData.start_pos, m_moveData.target_pos);

    // Yeni pozisyonu hesapla
    float progress = (m_moveData.moved_distance + move_distance) / m_moveData.total_distance;

    m_moveData.current_pos.x = m_moveData.start_pos.x + (LONG)(dir.dx * dir.distance * progress);
    m_moveData.current_pos.y = m_moveData.start_pos.y + (LONG)(dir.dy * dir.distance * progress);

    m_moveData.moved_distance += move_distance;
}

void CMovement::UpdatePathMove(float delta_time)
{
    UpdateSingleMove(delta_time);

    // Mevcut waypoint'e ulaştık mı?
    if (m_moveData.state == MOVEMENT_STATE_ARRIVED)
    {
        m_path[m_currentWaypointIndex].reached = true;

        // Bir sonraki waypoint'e geç
        if (!GoToNextWaypoint())
        {
            // Path tamamlandı
            m_bUsingPath = false;
            return;
        }
    }
}

bool CMovement::GoToNextWaypoint()
{
    m_currentWaypointIndex++;

    if (m_currentWaypointIndex >= m_path.size())
    {
        // Path tamamlandı
        return false;
    }

    // Sonraki waypoint'e hareket başlat
    TPosition from = m_moveData.current_pos;
    TPosition to = m_path[m_currentWaypointIndex].pos;

    return StartMove(from, to, m_moveData.move_type);
}

void CMovement::SetMoveSpeed(float speed)
{
    if (speed > 0.0f)
        m_moveData.move_speed = speed;
}

float CMovement::GetRemainingDistance() const
{
    if (m_moveData.state != MOVEMENT_STATE_MOVING)
        return 0.0f;

    return m_moveData.total_distance - m_moveData.moved_distance;
}

float CMovement::GetProgress() const
{
    if (m_moveData.total_distance < 1.0f)
        return 1.0f;

    return m_moveData.moved_distance / m_moveData.total_distance;
}

TDirection CMovement::CalculateDirection(const TPosition& from, const TPosition& to)
{
    TDirection dir;

    LONG dx = to.x - from.x;
    LONG dy = to.y - from.y;

    dir.distance = sqrt((float)(dx * dx + dy * dy));

    if (dir.distance > 0.0f)
    {
        dir.dx = dx / dir.distance;
        dir.dy = dy / dir.distance;
    }

    return dir;
}

float CMovement::CalculateDistance(const TPosition& pos1, const TPosition& pos2)
{
    LONG dx = pos2.x - pos1.x;
    LONG dy = pos2.y - pos1.y;
    return sqrt((float)(dx * dx + dy * dy));
}
