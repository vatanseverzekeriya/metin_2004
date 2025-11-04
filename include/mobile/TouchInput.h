#ifndef __INC_MOBILE_TOUCH_INPUT_H__
#define __INC_MOBILE_TOUCH_INPUT_H__

#include "../common/types.h"
#include <vector>
#include <map>
#include <functional>

// Touch tipi
enum ETouchType
{
    TOUCH_DOWN = 0,     // Dokunma başladı
    TOUCH_MOVE = 1,     // Dokunma hareket ediyor
    TOUCH_UP = 2,       // Dokunma bitti
    TOUCH_CANCEL = 3    // Dokunma iptal edildi
};

// Touch point - Bir parmak dokunması
struct TouchPoint
{
    DWORD id;           // Parmak ID (multi-touch için)
    float x;            // X koordinatı (0.0 - screen_width)
    float y;            // Y koordinatı (0.0 - screen_height)
    float pressure;     // Basınç (0.0 - 1.0)
    DWORD timestamp;    // Zaman damgası
    ETouchType type;    // Touch tipi

    TouchPoint()
        : id(0), x(0.0f), y(0.0f), pressure(0.0f)
        , timestamp(0), type(TOUCH_DOWN) {}

    TouchPoint(DWORD _id, float _x, float _y, ETouchType _type)
        : id(_id), x(_x), y(_y), pressure(1.0f)
        , timestamp(0), type(_type) {}
};

// Gesture tipleri
enum EGestureType
{
    GESTURE_TAP = 0,        // Tek dokunma
    GESTURE_DOUBLE_TAP = 1, // Çift dokunma
    GESTURE_LONG_PRESS = 2, // Uzun basma
    GESTURE_SWIPE = 3,      // Kaydırma
    GESTURE_PINCH = 4,      // Yakınlaştırma/uzaklaştırma
    GESTURE_DRAG = 5        // Sürükleme
};

// Swipe yönü
enum ESwipeDirection
{
    SWIPE_NONE = 0,
    SWIPE_LEFT = 1,
    SWIPE_RIGHT = 2,
    SWIPE_UP = 3,
    SWIPE_DOWN = 4
};

// Gesture bilgisi
struct GestureInfo
{
    EGestureType type;
    float start_x;
    float start_y;
    float current_x;
    float current_y;
    float delta_x;
    float delta_y;
    float distance;         // İki parmak arası mesafe (pinch için)
    float scale;           // Ölçek faktörü (pinch için)
    ESwipeDirection swipe_dir;
    DWORD duration;        // Gesture süresi (ms)
    DWORD touch_count;     // Kaç parmak

    GestureInfo()
        : type(GESTURE_TAP), start_x(0), start_y(0)
        , current_x(0), current_y(0), delta_x(0), delta_y(0)
        , distance(0), scale(1.0f), swipe_dir(SWIPE_NONE)
        , duration(0), touch_count(0) {}
};

// Touch callback tipleri
typedef std::function<void(const TouchPoint&)> TouchCallback;
typedef std::function<void(const GestureInfo&)> GestureCallback;

class CTouchInput
{
public:
    static CTouchInput& Instance();

    // Başlatma
    void Initialize(float screen_width, float screen_height);
    void Update(DWORD delta_time);

    // Touch event'leri
    void OnTouchDown(DWORD id, float x, float y);
    void OnTouchMove(DWORD id, float x, float y);
    void OnTouchUp(DWORD id, float x, float y);
    void OnTouchCancel(DWORD id);

    // Callback kaydetme
    void RegisterTouchCallback(TouchCallback callback);
    void RegisterGestureCallback(GestureCallback callback);

    // Touch sorguları
    bool IsTouching(DWORD id) const;
    const TouchPoint* GetTouch(DWORD id) const;
    DWORD GetActiveTouchCount() const;
    std::vector<TouchPoint> GetActiveTouches() const;

    // Gesture ayarları
    void SetTapThreshold(float threshold) { m_fTapThreshold = threshold; }
    void SetLongPressTime(DWORD time_ms) { m_dwLongPressTime = time_ms; }
    void SetSwipeThreshold(float threshold) { m_fSwipeThreshold = threshold; }

    // Ekran bilgileri
    float GetScreenWidth() const { return m_fScreenWidth; }
    float GetScreenHeight() const { return m_fScreenHeight; }

    // Koordinat dönüşümleri
    float NormalizeX(float x) const { return x / m_fScreenWidth; }
    float NormalizeY(float y) const { return y / m_fScreenHeight; }
    float DenormalizeX(float norm_x) const { return norm_x * m_fScreenWidth; }
    float DenormalizeY(float norm_y) const { return norm_y * m_fScreenHeight; }

private:
    CTouchInput();
    ~CTouchInput();

    CTouchInput(const CTouchInput&) = delete;
    CTouchInput& operator=(const CTouchInput&) = delete;

    void ProcessGestures();
    void DetectTap(const TouchPoint& touch);
    void DetectLongPress();
    void DetectSwipe(const TouchPoint& start, const TouchPoint& end);
    void DetectPinch();

    ESwipeDirection GetSwipeDirection(float dx, float dy) const;
    float GetDistance(float x1, float y1, float x2, float y2) const;

    // Touch tracking
    std::map<DWORD, TouchPoint> m_mapActiveTouches;
    std::map<DWORD, TouchPoint> m_mapTouchStartPoints;
    std::map<DWORD, DWORD> m_mapTouchStartTimes;

    // Callbacks
    std::vector<TouchCallback> m_vecTouchCallbacks;
    std::vector<GestureCallback> m_vecGestureCallbacks;

    // Gesture thresholds
    float m_fTapThreshold;          // Max hareket mesafesi (tap için)
    DWORD m_dwLongPressTime;        // Long press süresi (ms)
    float m_fSwipeThreshold;        // Min swipe mesafesi
    DWORD m_dwDoubleTapTime;        // Double tap max süresi (ms)

    // Ekran bilgileri
    float m_fScreenWidth;
    float m_fScreenHeight;

    // Son gesture
    DWORD m_dwLastTapTime;
    float m_fLastTapX;
    float m_fLastTapY;
};

#endif // __INC_MOBILE_TOUCH_INPUT_H__
