#include "../../include/mobile/VirtualJoystick.h"
#include <cmath>
#include <iostream>

CVirtualJoystick::CVirtualJoystick()
    : m_fCenterX(200.0f)
    , m_fCenterY(800.0f)
    , m_fRadius(100.0f)
    , m_fDeadZone(0.1f)
    , m_bFloating(false)
    , m_fOriginalCenterX(200.0f)
    , m_fOriginalCenterY(800.0f)
    , m_dwActiveTouchID(0xFFFFFFFF)
    , m_bAutoRecenter(true)
    , m_fRecenterSpeed(5.0f)
    , m_bVisible(true)
    , m_dwBaseColor(0x80FFFFFF)
    , m_dwStickColor(0xFFFFFFFF)
    , m_fAlpha(0.5f)
{
}

CVirtualJoystick::~CVirtualJoystick()
{
}

void CVirtualJoystick::Initialize(float center_x, float center_y, float radius)
{
    m_fCenterX = center_x;
    m_fCenterY = center_y;
    m_fRadius = radius;
    m_fOriginalCenterX = center_x;
    m_fOriginalCenterY = center_y;

    std::cout << "VirtualJoystick initialized at (" << center_x << "," << center_y
              << ") radius:" << radius << std::endl;
}

void CVirtualJoystick::Update(DWORD delta_time)
{
    // Auto recenter
    if (m_bAutoRecenter && !m_state.active)
    {
        float dt = delta_time / 1000.0f; // saniyeye çevir
        float speed = m_fRecenterSpeed * dt;

        // Yavaşça merkeze dön
        if (std::abs(m_state.x) > 0.01f || std::abs(m_state.y) > 0.01f)
        {
            m_state.x *= (1.0f - speed);
            m_state.y *= (1.0f - speed);
            m_state.magnitude *= (1.0f - speed);

            if (m_callback)
                m_callback(m_state);
        }
        else
        {
            m_state.x = 0.0f;
            m_state.y = 0.0f;
            m_state.magnitude = 0.0f;
        }
    }

    // Floating joystick merkezi reset
    if (m_bFloating && !m_state.active)
    {
        m_fCenterX = m_fOriginalCenterX;
        m_fCenterY = m_fOriginalCenterY;
    }
}

void CVirtualJoystick::Render()
{
    // Platform-specific rendering
    // Bu fonksiyon mobil platformda OpenGL ES veya Metal ile implement edilir
    // Şimdilik debug bilgisi yazdırıyoruz

    if (!m_bVisible)
        return;

    // Base circle çiz
    // DrawCircle(m_fCenterX, m_fCenterY, m_fRadius, m_dwBaseColor);

    // Stick çiz
    if (m_state.active)
    {
        float stick_x = m_fCenterX + m_state.x * m_fRadius;
        float stick_y = m_fCenterY + m_state.y * m_fRadius;
        // DrawCircle(stick_x, stick_y, m_fRadius * 0.4f, m_dwStickColor);
    }
}

void CVirtualJoystick::OnTouchDown(const TouchPoint& touch)
{
    // Joystick alanı içinde mi?
    float dx = touch.x - m_fCenterX;
    float dy = touch.y - m_fCenterY;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Floating joystick ise ilk dokunulan yer merkez olur
    if (m_bFloating && m_dwActiveTouchID == 0xFFFFFFFF)
    {
        m_fCenterX = touch.x;
        m_fCenterY = touch.y;
        distance = 0.0f;
    }

    // Joystick alanı içinde veya floating mode
    if (distance <= m_fRadius * 1.5f || m_bFloating)
    {
        m_dwActiveTouchID = touch.id;
        m_state.active = true;
        UpdateState(touch.x, touch.y);

        std::cout << "[Joystick] Activated" << std::endl;
    }
}

void CVirtualJoystick::OnTouchMove(const TouchPoint& touch)
{
    if (touch.id != m_dwActiveTouchID)
        return;

    UpdateState(touch.x, touch.y);
}

void CVirtualJoystick::OnTouchUp(const TouchPoint& touch)
{
    if (touch.id != m_dwActiveTouchID)
        return;

    m_dwActiveTouchID = 0xFFFFFFFF;
    ResetState();

    std::cout << "[Joystick] Deactivated" << std::endl;
}

void CVirtualJoystick::UpdateState(float touch_x, float touch_y)
{
    // Merkeze göre offset
    float dx = touch_x - m_fCenterX;
    float dy = touch_y - m_fCenterY;

    // Mesafe ve açı hesapla
    float distance = std::sqrt(dx * dx + dy * dy);
    float angle = std::atan2(dy, dx);

    // Magnitude (0-1 arası normalize et)
    float magnitude = distance / m_fRadius;

    // Limitle (1.0 maksimum)
    if (magnitude > 1.0f)
    {
        magnitude = 1.0f;
        dx = std::cos(angle) * m_fRadius;
        dy = std::sin(angle) * m_fRadius;
        distance = m_fRadius;
    }

    // Dead zone kontrolü
    if (magnitude < m_fDeadZone)
    {
        m_state.x = 0.0f;
        m_state.y = 0.0f;
        m_state.magnitude = 0.0f;
    }
    else
    {
        // Dead zone'dan sonraki değeri normalize et
        float adjusted_magnitude = (magnitude - m_fDeadZone) / (1.0f - m_fDeadZone);

        m_state.x = dx / m_fRadius;
        m_state.y = dy / m_fRadius;
        m_state.magnitude = adjusted_magnitude;
    }

    m_state.angle = angle;
    m_state.active = true;

    // Callback
    if (m_callback)
    {
        m_callback(m_state);
    }
}

void CVirtualJoystick::ResetState()
{
    m_state.x = 0.0f;
    m_state.y = 0.0f;
    m_state.angle = 0.0f;
    m_state.magnitude = 0.0f;
    m_state.active = false;

    // Callback
    if (m_callback)
    {
        m_callback(m_state);
    }
}

float CVirtualJoystick::Clamp(float value, float min, float max) const
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
