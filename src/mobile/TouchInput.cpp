#include "../../include/mobile/TouchInput.h"
#include <cmath>
#include <algorithm>
#include <iostream>

CTouchInput::CTouchInput()
    : m_fTapThreshold(20.0f)        // 20 piksel
    , m_dwLongPressTime(500)        // 500ms
    , m_fSwipeThreshold(100.0f)     // 100 piksel
    , m_dwDoubleTapTime(300)        // 300ms
    , m_fScreenWidth(1920.0f)
    , m_fScreenHeight(1080.0f)
    , m_dwLastTapTime(0)
    , m_fLastTapX(0.0f)
    , m_fLastTapY(0.0f)
{
}

CTouchInput::~CTouchInput()
{
}

CTouchInput& CTouchInput::Instance()
{
    static CTouchInput instance;
    return instance;
}

void CTouchInput::Initialize(float screen_width, float screen_height)
{
    m_fScreenWidth = screen_width;
    m_fScreenHeight = screen_height;

    std::cout << "TouchInput initialized: " << screen_width << "x" << screen_height << std::endl;
}

void CTouchInput::Update(DWORD delta_time)
{
    // Gesture detection
    ProcessGestures();

    // Long press detection
    DetectLongPress();
}

void CTouchInput::OnTouchDown(DWORD id, float x, float y)
{
    TouchPoint touch(id, x, y, TOUCH_DOWN);
    touch.timestamp = 0; // Gerçek implementasyonda sistem zamanı

    m_mapActiveTouches[id] = touch;
    m_mapTouchStartPoints[id] = touch;
    m_mapTouchStartTimes[id] = 0; // Gerçek implementasyonda sistem zamanı

    // Callbacks çağır
    for (auto& callback : m_vecTouchCallbacks)
    {
        callback(touch);
    }

    std::cout << "[Touch] DOWN id:" << id << " pos:(" << x << "," << y << ")" << std::endl;
}

void CTouchInput::OnTouchMove(DWORD id, float x, float y)
{
    auto it = m_mapActiveTouches.find(id);
    if (it == m_mapActiveTouches.end())
        return;

    TouchPoint& touch = it->second;
    touch.x = x;
    touch.y = y;
    touch.type = TOUCH_MOVE;

    // Callbacks çağır
    for (auto& callback : m_vecTouchCallbacks)
    {
        callback(touch);
    }
}

void CTouchInput::OnTouchUp(DWORD id, float x, float y)
{
    auto it = m_mapActiveTouches.find(id);
    if (it == m_mapActiveTouches.end())
        return;

    TouchPoint touch(id, x, y, TOUCH_UP);

    // Gesture detection
    auto start_it = m_mapTouchStartPoints.find(id);
    if (start_it != m_mapTouchStartPoints.end())
    {
        DetectTap(touch);
        DetectSwipe(start_it->second, touch);
    }

    // Callbacks çağır
    for (auto& callback : m_vecTouchCallbacks)
    {
        callback(touch);
    }

    // Temizle
    m_mapActiveTouches.erase(id);
    m_mapTouchStartPoints.erase(id);
    m_mapTouchStartTimes.erase(id);

    std::cout << "[Touch] UP id:" << id << " pos:(" << x << "," << y << ")" << std::endl;
}

void CTouchInput::OnTouchCancel(DWORD id)
{
    m_mapActiveTouches.erase(id);
    m_mapTouchStartPoints.erase(id);
    m_mapTouchStartTimes.erase(id);

    std::cout << "[Touch] CANCEL id:" << id << std::endl;
}

void CTouchInput::RegisterTouchCallback(TouchCallback callback)
{
    m_vecTouchCallbacks.push_back(callback);
}

void CTouchInput::RegisterGestureCallback(GestureCallback callback)
{
    m_vecGestureCallbacks.push_back(callback);
}

bool CTouchInput::IsTouching(DWORD id) const
{
    return m_mapActiveTouches.find(id) != m_mapActiveTouches.end();
}

const TouchPoint* CTouchInput::GetTouch(DWORD id) const
{
    auto it = m_mapActiveTouches.find(id);
    return (it != m_mapActiveTouches.end()) ? &it->second : nullptr;
}

DWORD CTouchInput::GetActiveTouchCount() const
{
    return m_mapActiveTouches.size();
}

std::vector<TouchPoint> CTouchInput::GetActiveTouches() const
{
    std::vector<TouchPoint> touches;
    for (const auto& pair : m_mapActiveTouches)
    {
        touches.push_back(pair.second);
    }
    return touches;
}

void CTouchInput::ProcessGestures()
{
    // Pinch gesture (2 parmak)
    if (m_mapActiveTouches.size() == 2)
    {
        DetectPinch();
    }
}

void CTouchInput::DetectTap(const TouchPoint& touch)
{
    auto start_it = m_mapTouchStartPoints.find(touch.id);
    if (start_it == m_mapTouchStartPoints.end())
        return;

    const TouchPoint& start = start_it->second;
    float distance = GetDistance(start.x, start.y, touch.x, touch.y);

    // Hareket çok az ise tap
    if (distance < m_fTapThreshold)
    {
        GestureInfo gesture;
        gesture.type = GESTURE_TAP;
        gesture.start_x = start.x;
        gesture.start_y = start.y;
        gesture.current_x = touch.x;
        gesture.current_y = touch.y;
        gesture.touch_count = 1;

        // Double tap kontrolü
        DWORD current_time = 0; // Gerçek implementasyonda sistem zamanı
        if (current_time - m_dwLastTapTime < m_dwDoubleTapTime)
        {
            float tap_distance = GetDistance(m_fLastTapX, m_fLastTapY, touch.x, touch.y);
            if (tap_distance < m_fTapThreshold * 2)
            {
                gesture.type = GESTURE_DOUBLE_TAP;
                m_dwLastTapTime = 0; // Reset
            }
        }
        else
        {
            m_dwLastTapTime = current_time;
            m_fLastTapX = touch.x;
            m_fLastTapY = touch.y;
        }

        // Callback çağır
        for (auto& callback : m_vecGestureCallbacks)
        {
            callback(gesture);
        }

        std::cout << "[Gesture] TAP at (" << touch.x << "," << touch.y << ")" << std::endl;
    }
}

void CTouchInput::DetectLongPress()
{
    DWORD current_time = 0; // Gerçek implementasyonda sistem zamanı

    for (const auto& pair : m_mapActiveTouches)
    {
        DWORD id = pair.first;
        const TouchPoint& touch = pair.second;

        auto time_it = m_mapTouchStartTimes.find(id);
        if (time_it == m_mapTouchStartTimes.end())
            continue;

        DWORD duration = current_time - time_it->second;

        // Long press kontrolü
        if (duration >= m_dwLongPressTime)
        {
            auto start_it = m_mapTouchStartPoints.find(id);
            if (start_it == m_mapTouchStartPoints.end())
                continue;

            const TouchPoint& start = start_it->second;
            float distance = GetDistance(start.x, start.y, touch.x, touch.y);

            // Pek hareket etmemiş ise
            if (distance < m_fTapThreshold)
            {
                GestureInfo gesture;
                gesture.type = GESTURE_LONG_PRESS;
                gesture.start_x = start.x;
                gesture.start_y = start.y;
                gesture.current_x = touch.x;
                gesture.current_y = touch.y;
                gesture.duration = duration;
                gesture.touch_count = 1;

                // Callback çağır
                for (auto& callback : m_vecGestureCallbacks)
                {
                    callback(gesture);
                }

                std::cout << "[Gesture] LONG_PRESS at (" << touch.x << "," << touch.y << ")" << std::endl;

                // Bir kere tetiklenmesi için temizle
                m_mapTouchStartTimes[id] = current_time + 10000;
            }
        }
    }
}

void CTouchInput::DetectSwipe(const TouchPoint& start, const TouchPoint& end)
{
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = GetDistance(start.x, start.y, end.x, end.y);

    // Yeterince uzun hareket ise swipe
    if (distance >= m_fSwipeThreshold)
    {
        GestureInfo gesture;
        gesture.type = GESTURE_SWIPE;
        gesture.start_x = start.x;
        gesture.start_y = start.y;
        gesture.current_x = end.x;
        gesture.current_y = end.y;
        gesture.delta_x = dx;
        gesture.delta_y = dy;
        gesture.swipe_dir = GetSwipeDirection(dx, dy);
        gesture.touch_count = 1;

        // Callback çağır
        for (auto& callback : m_vecGestureCallbacks)
        {
            callback(gesture);
        }

        const char* dir_names[] = {"NONE", "LEFT", "RIGHT", "UP", "DOWN"};
        std::cout << "[Gesture] SWIPE " << dir_names[gesture.swipe_dir]
                  << " distance:" << distance << std::endl;
    }
}

void CTouchInput::DetectPinch()
{
    if (m_mapActiveTouches.size() != 2)
        return;

    auto it = m_mapActiveTouches.begin();
    const TouchPoint& touch1 = it->second;
    ++it;
    const TouchPoint& touch2 = it->second;

    float current_distance = GetDistance(touch1.x, touch1.y, touch2.x, touch2.y);

    // Start points
    auto start1_it = m_mapTouchStartPoints.find(touch1.id);
    auto start2_it = m_mapTouchStartPoints.find(touch2.id);

    if (start1_it != m_mapTouchStartPoints.end() &&
        start2_it != m_mapTouchStartPoints.end())
    {
        const TouchPoint& start1 = start1_it->second;
        const TouchPoint& start2 = start2_it->second;

        float start_distance = GetDistance(start1.x, start1.y, start2.x, start2.y);

        if (start_distance > 0)
        {
            float scale = current_distance / start_distance;

            GestureInfo gesture;
            gesture.type = GESTURE_PINCH;
            gesture.start_x = (start1.x + start2.x) / 2.0f;
            gesture.start_y = (start1.y + start2.y) / 2.0f;
            gesture.current_x = (touch1.x + touch2.x) / 2.0f;
            gesture.current_y = (touch1.y + touch2.y) / 2.0f;
            gesture.distance = current_distance;
            gesture.scale = scale;
            gesture.touch_count = 2;

            // Callback çağır (sürekli)
            for (auto& callback : m_vecGestureCallbacks)
            {
                callback(gesture);
            }
        }
    }
}

ESwipeDirection CTouchInput::GetSwipeDirection(float dx, float dy) const
{
    // Dominant yön
    if (std::abs(dx) > std::abs(dy))
    {
        return (dx > 0) ? SWIPE_RIGHT : SWIPE_LEFT;
    }
    else
    {
        return (dy > 0) ? SWIPE_DOWN : SWIPE_UP;
    }
}

float CTouchInput::GetDistance(float x1, float y1, float x2, float y2) const
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}
