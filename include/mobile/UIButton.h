#ifndef __INC_MOBILE_UI_BUTTON_H__
#define __INC_MOBILE_UI_BUTTON_H__

#include "../common/types.h"
#include "TouchInput.h"
#include <string>
#include <functional>

// Button durumu
enum EButtonState
{
    BUTTON_NORMAL = 0,
    BUTTON_PRESSED = 1,
    BUTTON_DISABLED = 2,
    BUTTON_HIGHLIGHTED = 3
};

// Button tipi
enum EButtonType
{
    BUTTON_NORMAL_BTN = 0,
    BUTTON_TOGGLE = 1,      // On/Off toggle
    BUTTON_COOLDOWN = 2     // Cooldown gösterir (skill için)
};

// Button callback
typedef std::function<void()> ButtonCallback;
typedef std::function<void(bool)> ToggleCallback;

class CUIButton
{
public:
    CUIButton();
    ~CUIButton();

    // Başlatma
    void Initialize(float x, float y, float width, float height);
    void Update(DWORD delta_time);
    void Render();

    // Touch işleme
    bool OnTouchDown(const TouchPoint& touch);
    bool OnTouchMove(const TouchPoint& touch);
    bool OnTouchUp(const TouchPoint& touch);

    // Hit test
    bool Contains(float x, float y) const;

    // Pozisyon ve boyut
    void SetPosition(float x, float y) { m_fX = x; m_fY = y; }
    void SetSize(float width, float height) { m_fWidth = width; m_fHeight = height; }
    void SetBounds(float x, float y, float width, float height)
    {
        m_fX = x; m_fY = y; m_fWidth = width; m_fHeight = height;
    }

    float GetX() const { return m_fX; }
    float GetY() const { return m_fY; }
    float GetWidth() const { return m_fWidth; }
    float GetHeight() const { return m_fHeight; }

    // Durum
    void SetState(EButtonState state) { m_eState = state; }
    EButtonState GetState() const { return m_eState; }

    void SetEnabled(bool enabled) { m_eState = enabled ? BUTTON_NORMAL : BUTTON_DISABLED; }
    bool IsEnabled() const { return m_eState != BUTTON_DISABLED; }

    // Button tipi
    void SetType(EButtonType type) { m_eType = type; }
    EButtonType GetType() const { return m_eType; }

    // Toggle button
    void SetToggled(bool toggled) { m_bToggled = toggled; }
    bool IsToggled() const { return m_bToggled; }

    // Cooldown (skill button için)
    void SetCooldown(float cooldown_sec);
    void UpdateCooldown(DWORD delta_time);
    float GetCooldownPercent() const;
    bool IsOnCooldown() const { return m_fCooldownRemaining > 0.0f; }

    // Text
    void SetText(const std::string& text) { m_strText = text; }
    const std::string& GetText() const { return m_strText; }

    // Icon (texture ID veya path)
    void SetIcon(DWORD icon_id) { m_dwIconID = icon_id; }
    DWORD GetIconID() const { return m_dwIconID; }

    // Görünürlük
    void SetVisible(bool visible) { m_bVisible = visible; }
    bool IsVisible() const { return m_bVisible; }

    // Callback
    void SetClickCallback(ButtonCallback callback) { m_clickCallback = callback; }
    void SetToggleCallback(ToggleCallback callback) { m_toggleCallback = callback; }

    // Renk
    void SetColor(DWORD normal, DWORD pressed, DWORD disabled)
    {
        m_dwColorNormal = normal;
        m_dwColorPressed = pressed;
        m_dwColorDisabled = disabled;
    }

    // Ses
    void SetClickSound(const std::string& sound) { m_strClickSound = sound; }

    // Badge (bildirim sayısı göstermek için)
    void SetBadgeCount(DWORD count) { m_dwBadgeCount = count; }
    DWORD GetBadgeCount() const { return m_dwBadgeCount; }

private:
    void PlayClickSound();
    DWORD GetCurrentColor() const;

    // Pozisyon ve boyut
    float m_fX;
    float m_fY;
    float m_fWidth;
    float m_fHeight;

    // Durum
    EButtonState m_eState;
    EButtonType m_eType;
    bool m_bVisible;

    // Toggle state
    bool m_bToggled;

    // Cooldown
    float m_fCooldownTotal;
    float m_fCooldownRemaining;

    // Text ve icon
    std::string m_strText;
    DWORD m_dwIconID;

    // Görünüm
    DWORD m_dwColorNormal;
    DWORD m_dwColorPressed;
    DWORD m_dwColorDisabled;

    // Badge
    DWORD m_dwBadgeCount;

    // Ses
    std::string m_strClickSound;

    // Touch tracking
    DWORD m_dwActiveTouchID;
    bool m_bPressed;

    // Callbacks
    ButtonCallback m_clickCallback;
    ToggleCallback m_toggleCallback;
};

#endif // __INC_MOBILE_UI_BUTTON_H__
