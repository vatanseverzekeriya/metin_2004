#include "../../include/mobile/UIButton.h"
#include <iostream>

CUIButton::CUIButton()
    : m_fX(0.0f), m_fY(0.0f), m_fWidth(100.0f), m_fHeight(100.0f)
    , m_eState(BUTTON_NORMAL), m_eType(BUTTON_NORMAL_BTN), m_bVisible(true)
    , m_bToggled(false), m_fCooldownTotal(0.0f), m_fCooldownRemaining(0.0f)
    , m_dwIconID(0), m_dwColorNormal(0xFFFFFFFF), m_dwColorPressed(0xFFCCCCCC)
    , m_dwColorDisabled(0xFF888888), m_dwBadgeCount(0)
    , m_dwActiveTouchID(0xFFFFFFFF), m_bPressed(false)
{
}

CUIButton::~CUIButton()
{
}

void CUIButton::Initialize(float x, float y, float width, float height)
{
    m_fX = x;
    m_fY = y;
    m_fWidth = width;
    m_fHeight = height;
}

void CUIButton::Update(DWORD delta_time)
{
    UpdateCooldown(delta_time);
}

void CUIButton::Render()
{
    if (!m_bVisible)
        return;

    // Platform-specific rendering
    DWORD color = GetCurrentColor();

    // DrawRect(m_fX, m_fY, m_fWidth, m_fHeight, color);
    // if (m_dwIconID > 0) DrawTexture(m_dwIconID, m_fX, m_fY, m_fWidth, m_fHeight);
    // if (!m_strText.empty()) DrawText(m_strText, m_fX, m_fY);

    // Cooldown overlay
    if (IsOnCooldown())
    {
        float percent = GetCooldownPercent();
        // DrawCooldownOverlay(m_fX, m_fY, m_fWidth, m_fHeight, percent);
    }

    // Badge
    if (m_dwBadgeCount > 0)
    {
        // DrawBadge(m_fX + m_fWidth * 0.7f, m_fY, m_dwBadgeCount);
    }
}

bool CUIButton::OnTouchDown(const TouchPoint& touch)
{
    if (!m_bVisible || !IsEnabled())
        return false;

    if (Contains(touch.x, touch.y))
    {
        m_dwActiveTouchID = touch.id;
        m_bPressed = true;
        m_eState = BUTTON_PRESSED;

        std::cout << "[Button] Pressed: " << m_strText << std::endl;
        return true;
    }

    return false;
}

bool CUIButton::OnTouchMove(const TouchPoint& touch)
{
    if (touch.id != m_dwActiveTouchID)
        return false;

    // Touch button dışına çıktı mı?
    if (!Contains(touch.x, touch.y))
    {
        if (m_bPressed)
        {
            m_bPressed = false;
            m_eState = BUTTON_NORMAL;
        }
    }
    else
    {
        if (!m_bPressed)
        {
            m_bPressed = true;
            m_eState = BUTTON_PRESSED;
        }
    }

    return true;
}

bool CUIButton::OnTouchUp(const TouchPoint& touch)
{
    if (touch.id != m_dwActiveTouchID)
        return false;

    m_dwActiveTouchID = 0xFFFFFFFF;

    if (m_bPressed && Contains(touch.x, touch.y))
    {
        // Click!
        m_eState = BUTTON_NORMAL;
        m_bPressed = false;

        PlayClickSound();

        // Toggle button
        if (m_eType == BUTTON_TOGGLE)
        {
            m_bToggled = !m_bToggled;
            if (m_toggleCallback)
                m_toggleCallback(m_bToggled);
        }

        // Normal callback
        if (m_clickCallback)
            m_clickCallback();

        std::cout << "[Button] Clicked: " << m_strText << std::endl;
        return true;
    }

    m_bPressed = false;
    m_eState = BUTTON_NORMAL;
    return false;
}

bool CUIButton::Contains(float x, float y) const
{
    return (x >= m_fX && x <= m_fX + m_fWidth &&
            y >= m_fY && y <= m_fY + m_fHeight);
}

void CUIButton::SetCooldown(float cooldown_sec)
{
    m_fCooldownTotal = cooldown_sec;
    m_fCooldownRemaining = cooldown_sec;
}

void CUIButton::UpdateCooldown(DWORD delta_time)
{
    if (m_fCooldownRemaining > 0.0f)
    {
        m_fCooldownRemaining -= delta_time / 1000.0f;
        if (m_fCooldownRemaining < 0.0f)
            m_fCooldownRemaining = 0.0f;
    }
}

float CUIButton::GetCooldownPercent() const
{
    if (m_fCooldownTotal <= 0.0f)
        return 0.0f;
    return m_fCooldownRemaining / m_fCooldownTotal;
}

void CUIButton::PlayClickSound()
{
    if (!m_strClickSound.empty())
    {
        // PlaySound(m_strClickSound);
        std::cout << "[Sound] " << m_strClickSound << std::endl;
    }
}

DWORD CUIButton::GetCurrentColor() const
{
    switch (m_eState)
    {
    case BUTTON_PRESSED:
    case BUTTON_HIGHLIGHTED:
        return m_dwColorPressed;
    case BUTTON_DISABLED:
        return m_dwColorDisabled;
    default:
        return m_dwColorNormal;
    }
}
