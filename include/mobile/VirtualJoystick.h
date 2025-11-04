#ifndef __INC_MOBILE_VIRTUAL_JOYSTICK_H__
#define __INC_MOBILE_VIRTUAL_JOYSTICK_H__

#include "../common/types.h"
#include "TouchInput.h"
#include <functional>

// Joystick pozisyon bilgisi
struct JoystickState
{
    float x;            // -1.0 (sol) ile 1.0 (sağ)
    float y;            // -1.0 (yukarı) ile 1.0 (aşağı)
    float angle;        // Açı (radyan)
    float magnitude;    // Büyüklük (0.0 - 1.0)
    bool active;        // Aktif mi?

    JoystickState()
        : x(0.0f), y(0.0f), angle(0.0f)
        , magnitude(0.0f), active(false) {}

    // Yön vektörü
    void GetDirection(float& out_x, float& out_y) const
    {
        out_x = x * magnitude;
        out_y = y * magnitude;
    }

    // 8 yön (Metin2 tarzı)
    BYTE Get8Direction() const
    {
        if (!active || magnitude < 0.1f)
            return 255; // Yok

        float angle_deg = angle * 180.0f / 3.14159f;
        if (angle_deg < 0) angle_deg += 360.0f;

        // 8 yön: 0=Sağ, 1=SağAlt, 2=Alt, 3=SolAlt, 4=Sol, 5=SolÜst, 6=Üst, 7=SağÜst
        int dir = (int)((angle_deg + 22.5f) / 45.0f) % 8;
        return (BYTE)dir;
    }
};

// Joystick callback
typedef std::function<void(const JoystickState&)> JoystickCallback;

class CVirtualJoystick
{
public:
    CVirtualJoystick();
    ~CVirtualJoystick();

    // Başlatma
    void Initialize(float center_x, float center_y, float radius);
    void Update(DWORD delta_time);
    void Render(); // Çizim (platforma özel)

    // Joystick kontrolü
    void OnTouchDown(const TouchPoint& touch);
    void OnTouchMove(const TouchPoint& touch);
    void OnTouchUp(const TouchPoint& touch);

    // Durum
    const JoystickState& GetState() const { return m_state; }
    bool IsActive() const { return m_state.active; }

    // Callback
    void SetCallback(JoystickCallback callback) { m_callback = callback; }

    // Ayarlar
    void SetPosition(float x, float y) { m_fCenterX = x; m_fCenterY = y; }
    void SetRadius(float radius) { m_fRadius = radius; }
    void SetDeadZone(float dead_zone) { m_fDeadZone = dead_zone; }
    void SetAutoRecenter(bool enable) { m_bAutoRecenter = enable; }
    void SetFloating(bool enable) { m_bFloating = enable; }

    // Bilgiler
    float GetCenterX() const { return m_fCenterX; }
    float GetCenterY() const { return m_fCenterY; }
    float GetRadius() const { return m_fRadius; }

    // Görünürlük
    void SetVisible(bool visible) { m_bVisible = visible; }
    bool IsVisible() const { return m_bVisible; }

    // Renk/görünüm (render için)
    void SetBaseColor(DWORD color) { m_dwBaseColor = color; }
    void SetStickColor(DWORD color) { m_dwStickColor = color; }
    void SetAlpha(float alpha) { m_fAlpha = alpha; }

private:
    void UpdateState(float touch_x, float touch_y);
    void ResetState();
    float Clamp(float value, float min, float max) const;

    // Pozisyon ve boyut
    float m_fCenterX;
    float m_fCenterY;
    float m_fRadius;
    float m_fDeadZone;          // Dead zone yarıçapı (0.0 - 1.0)

    // Floating joystick (ilk dokunduğun yerde belirir)
    bool m_bFloating;
    float m_fOriginalCenterX;
    float m_fOriginalCenterY;

    // State
    JoystickState m_state;
    DWORD m_dwActiveTouchID;
    bool m_bAutoRecenter;       // Bırakınca merkeze dön
    float m_fRecenterSpeed;     // Merkeze dönme hızı

    // Callback
    JoystickCallback m_callback;

    // Görünüm
    bool m_bVisible;
    DWORD m_dwBaseColor;
    DWORD m_dwStickColor;
    float m_fAlpha;
};

#endif // __INC_MOBILE_VIRTUAL_JOYSTICK_H__
