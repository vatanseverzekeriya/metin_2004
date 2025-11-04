# Metin2 PvP - Mobil Dokunmatik UI Dokümantasyonu

## 📱 Genel Bakış

Metin2 PvP için özel olarak tasarlanmış, mobil dokunmatik ekranlar için optimize edilmiş C++ UI sistemi.

### Temel Özellikler

- ✅ Multi-touch desteği
- ✅ Virtual joystick (8 yön)
- ✅ Özelleştirilebilir button sistemi
- ✅ Gesture detection (tap, swipe, pinch, long-press)
- ✅ Skill cooldown gösterimi
- ✅ Auto-attack modu
- ✅ Layout customization (sağ/sol el)
- ✅ Android ve iOS entegrasyonu

## 🎮 UI Bileşenleri

### 1. Touch Input Sistemi

Multi-touch input ve gesture detection.

```cpp
#include "mobile/TouchInput.h"

// Başlatma
CTouchInput::Instance().Initialize(screen_width, screen_height);

// Touch callback kaydet
CTouchInput::Instance().RegisterTouchCallback([](const TouchPoint& touch) {
    std::cout << "Touch at (" << touch.x << "," << touch.y << ")" << std::endl;
});

// Gesture callback kaydet
CTouchInput::Instance().RegisterGestureCallback([](const GestureInfo& gesture) {
    if (gesture.type == GESTURE_SWIPE) {
        // Swipe işle
    }
});

// Touch event'leri (mobil platformdan)
CTouchInput::Instance().OnTouchDown(id, x, y);
CTouchInput::Instance().OnTouchMove(id, x, y);
CTouchInput::Instance().OnTouchUp(id, x, y);
```

**Desteklenen Gesture'lar:**
- **TAP**: Tek dokunma
- **DOUBLE_TAP**: Çift dokunma
- **LONG_PRESS**: Uzun basma (500ms+)
- **SWIPE**: Kaydırma (LEFT, RIGHT, UP, DOWN)
- **PINCH**: İki parmakla zoom
- **DRAG**: Sürükleme

### 2. Virtual Joystick

8 yönlü hareket joystick'i (Metin2 tarzı).

```cpp
#include "mobile/VirtualJoystick.h"

CVirtualJoystick joystick;
joystick.Initialize(200, 800, 100); // x, y, radius

// Callback
joystick.SetCallback([](const JoystickState& state) {
    if (state.active) {
        float dx, dy;
        state.GetDirection(dx, dy);

        // Karakteri hareket ettir
        MoveCharacter(dx, dy);

        // Veya 8 yön kullan
        BYTE direction = state.Get8Direction();
        // 0=Sağ, 1=SağAlt, 2=Alt, 3=SolAlt, 4=Sol, 5=SolÜst, 6=Üst, 7=SağÜst
    }
});

// Floating joystick (ilk dokunulan yerde belirir)
joystick.SetFloating(true);

// Dead zone ayarla
joystick.SetDeadZone(0.1f); // %10 dead zone

// Auto-recenter
joystick.SetAutoRecenter(true);
```

**Joystick Özellikleri:**
- **Fixed/Floating Mode**: Sabit veya dinamik pozisyon
- **Dead Zone**: Merkezdeki hassasiyet ayarı
- **Auto-Recenter**: Bırakınca merkeze dön
- **8 Directional**: Metin2 için optimize edilmiş

### 3. UI Button

Cooldown, toggle ve normal butonlar.

```cpp
#include "mobile/UIButton.h"

CUIButton attackBtn;
attackBtn.Initialize(1700, 900, 120, 120);
attackBtn.SetText("ATTACK");
attackBtn.SetIcon(icon_id);

// Click callback
attackBtn.SetClickCallback([]() {
    Attack();
});

// Cooldown button (skill için)
CUIButton skillBtn;
skillBtn.SetType(BUTTON_COOLDOWN);
skillBtn.SetCooldown(5.0f); // 5 saniye cooldown

// Toggle button (auto-attack için)
CUIButton autoBtn;
autoBtn.SetType(BUTTON_TOGGLE);
autoBtn.SetToggleCallback([](bool enabled) {
    SetAutoAttack(enabled);
});

// Disable button
attackBtn.SetEnabled(false);

// Badge (bildirim sayısı)
questBtn.SetBadgeCount(3); // "3" göster
```

**Button Tipleri:**
- **NORMAL**: Standard buton
- **TOGGLE**: On/Off toggle
- **COOLDOWN**: Cooldown gösterir (skill için)

### 4. Game Controls

Tam oyun kontrol sistemi (joystick + butonlar).

```cpp
#include "mobile/GameControls.h"

// Başlatma
CGameControls::Instance().Initialize(screen_width, screen_height);

// Movement callback
CGameControls::Instance().SetOnMoveCallback([](float x, float y) {
    // Karakteri hareket ettir
    character->Move(x, y);
});

// Attack callback
CGameControls::Instance().SetOnAttackCallback([]() {
    character->Attack();
});

// Skill callback
CGameControls::Instance().SetOnSkillCallback([](int skill_index) {
    character->UseSkill(skill_index);
});

// Quick slot ayarla
CGameControls::Instance().SetQuickSlot(0, skill_id, icon_id);

// Layout değiştir
CGameControls::Instance().SetLayout(1); // 0=default, 1=left-handed

// Auto-attack
CGameControls::Instance().SetAutoAttack(true);

// Update & Render
CGameControls::Instance().Update(delta_time);
CGameControls::Instance().Render();
```

## 📐 Layout Sistemi

### Default Layout (Sağ El)

```
┌──────────────────────────────────────┐
│  [⚙] [Q] [C] [I] [☰]        (Üst)  │
│                                      │
│                                      │
│                                      │
│                              [JUMP]  │
│  (Oyun Alanı)               [PICK]  │
│                                      │
│                              [ATK]   │
│  [JOY]                              │
│  ════════════════════════            │
│  [1] [2] [3] [4] [5] [6]  (Skills)  │
└──────────────────────────────────────┘

JOY = Virtual Joystick (Sol alt)
ATK = Attack Button (Sağ alt)
1-6 = Skill Quick Slots (Alt ortada)
```

### Left-Handed Layout

```
┌──────────────────────────────────────┐
│  [⚙] [Q] [C] [I] [☰]        (Üst)  │
│                                      │
│                                      │
│                                      │
│  [JUMP]                              │
│  [PICK]               (Oyun Alanı)  │
│                                      │
│  [ATK]                              │
│                               [JOY]  │
│  ════════════════════════            │
│  [1] [2] [3] [4] [5] [6]  (Skills)  │
└──────────────────────────────────────┘

JOY = Sağ alt
ATK = Sol alt
```

## 🔧 Platform Entegrasyonu

### Android (Kotlin/Java)

```kotlin
// TouchHandler.kt
class TouchHandler(private val nativePtr: Long) : View.OnTouchListener {

    override fun onTouch(v: View?, event: MotionEvent?): Boolean {
        val id = event.getPointerId(event.actionIndex)
        val x = event.getX(event.actionIndex)
        val y = event.getY(event.actionIndex)

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> nativeOnTouchDown(nativePtr, id, x, y)
            MotionEvent.ACTION_MOVE -> nativeOnTouchMove(nativePtr, id, x, y)
            MotionEvent.ACTION_UP -> nativeOnTouchUp(nativePtr, id, x, y)
        }

        return true
    }

    private external fun nativeOnTouchDown(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchMove(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchUp(ptr: Long, id: Int, x: Float, y: Float)
}
```

### iOS (Swift)

```swift
// TouchHandler.swift
class GameView: GLKView {

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            let id = UInt32(touch.hash & 0xFFFFFFFF)

            nativeOnTouchDown(nativePtr, id, Float(location.x), Float(location.y))
        }
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            let id = UInt32(touch.hash & 0xFFFFFFFF)

            nativeOnTouchMove(nativePtr, id, Float(location.x), Float(location.y))
        }
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            let id = UInt32(touch.hash & 0xFFFFFFFF)

            nativeOnTouchUp(nativePtr, id, Float(location.x), Float(location.y))
        }
    }
}
```

## 🎯 Kullanım Örnekleri

### Tam Oyun Döngüsü

```cpp
// Game initialization
void InitGame() {
    // Touch input
    CTouchInput::Instance().Initialize(1920, 1080);

    // Game controls
    CGameControls::Instance().Initialize(1920, 1080);

    // Callbacks
    CGameControls::Instance().SetOnMoveCallback([](float dx, float dy) {
        g_player->Move(dx, dy);

        // Network'e gönder
        TPacketCGMove move;
        move.x = g_player->GetX();
        move.y = g_player->GetY();
        SendPacket(HEADER_CG_MOVE, move);
    });

    CGameControls::Instance().SetOnAttackCallback([]() {
        auto* target = g_player->GetTarget();
        if (target) {
            TPacketCGAttack attack;
            attack.target_id = target->GetID();
            SendPacket(HEADER_CG_ATTACK, attack);
        }
    });

    // Skills
    for (int i = 0; i < 6; i++) {
        DWORD skill_id = g_player->GetSkillID(i);
        DWORD icon_id = GetSkillIcon(skill_id);
        CGameControls::Instance().SetQuickSlot(i, skill_id, icon_id);
    }
}

// Game loop
void GameLoop(float delta_time) {
    // Update touch input
    CTouchInput::Instance().Update(delta_time * 1000);

    // Update controls
    CGameControls::Instance().Update(delta_time * 1000);

    // Render game
    RenderGame();

    // Render UI
    CGameControls::Instance().Render();
}

// Touch event (from platform)
void OnPlatformTouch(int id, float x, float y, int type) {
    TouchPoint touch(id, x, y, (ETouchType)type);

    switch (type) {
    case TOUCH_DOWN:
        CTouchInput::Instance().OnTouchDown(id, x, y);
        CGameControls::Instance().OnTouchDown(touch);
        break;
    case TOUCH_MOVE:
        CTouchInput::Instance().OnTouchMove(id, x, y);
        CGameControls::Instance().OnTouchMove(touch);
        break;
    case TOUCH_UP:
        CTouchInput::Instance().OnTouchUp(id, x, y);
        CGameControls::Instance().OnTouchUp(touch);
        break;
    }
}
```

### Özel Button Oluşturma

```cpp
// Inventory button
CUIButton inventoryBtn;
inventoryBtn.Initialize(1800, 20, 60, 60);
inventoryBtn.SetIcon(ICON_INVENTORY);
inventoryBtn.SetClickSound("ui_click.wav");
inventoryBtn.SetClickCallback([]() {
    ToggleInventory();
});

// Quest button with badge
CUIButton questBtn;
questBtn.Initialize(1720, 20, 60, 60);
questBtn.SetIcon(ICON_QUEST);
questBtn.SetBadgeCount(3); // 3 aktif quest
questBtn.SetClickCallback([]() {
    ShowQuestWindow();
});

// Skill button with cooldown
CUIButton skillBtn;
skillBtn.Initialize(900, 1000, 80, 80);
skillBtn.SetIcon(ICON_SKILL_FIREBALL);
skillBtn.SetType(BUTTON_COOLDOWN);
skillBtn.SetClickCallback([&skillBtn]() {
    if (!skillBtn.IsOnCooldown()) {
        UseSkill(SKILL_FIREBALL);
        skillBtn.SetCooldown(5.0f); // 5 sec cooldown
    }
});
```

## ⚙️ Optimizasyon

### Performans İpuçları

1. **Touch Event Batching**
   - Touch move event'lerini her frame topla
   - Gereksiz güncellemeleri filtrele

2. **Render Batching**
   - Tüm UI elementlerini tek draw call'da çiz
   - Texture atlas kullan

3. **Update Frequency**
   - Joystick: 60 FPS
   - Buttons: Touch event'lerde
   - Cooldowns: 10 FPS yeterli

4. **Memory**
   - UI elementleri pool'da tut
   - Texture'ları önceden yükle

### Battery Optimization

```cpp
// Düşük batarya modunda
if (batteryLevel < 20) {
    // Update interval'i artır
    joystick->SetUpdateInterval(100); // 10 FPS

    // Gereksiz efektleri kapat
    joystick->SetAlpha(0.3f);

    // Auto-attack'i zorla
    CGameControls::Instance().SetAutoAttack(true);
}
```

## 📊 Test

```bash
# UI test çalıştır
./bin/test_mobile_ui

# Test menüsü:
# 1. Joystick test
# 2. Button test
# 3. Skill cooldown test
# 4. Gesture test
# 5. Full game session
```

## 🎨 Customization

### Joystick Görünümü

```cpp
joystick.SetBaseColor(0x80FFFFFF);    // Yarı saydam beyaz
joystick.SetStickColor(0xFFFF0000);   // Kırmızı stick
joystick.SetAlpha(0.7f);              // %70 opaklık
```

### Button Renkleri

```cpp
button.SetColor(
    0xFFFFFFFF,  // Normal (beyaz)
    0xFFCCCCCC,  // Pressed (gri)
    0xFF888888   // Disabled (koyu gri)
);
```

## 🔧 Build

CMakeLists.txt'ye eklenecek:

```cmake
set(MOBILE_SOURCES
    src/mobile/TouchInput.cpp
    src/mobile/VirtualJoystick.cpp
    src/mobile/UIButton.cpp
    src/mobile/GameControls.cpp
)

add_library(metin2_mobile SHARED ${MOBILE_SOURCES})
```

## 📱 Platform Gereksinimleri

- **Android**: API 21+ (Android 5.0+), OpenGL ES 3.0
- **iOS**: iOS 11.0+, Metal/OpenGL ES 3.0
- **Screen**: 1280x720 minimum (HD)

## 🎮 Sonuç

Metin2 PvP için tam özellikli, production-ready mobil UI sistemi!

**Özellikler:**
- ✅ Multi-touch
- ✅ Virtual joystick
- ✅ Cooldown system
- ✅ Gesture detection
- ✅ Android/iOS integration
- ✅ Customizable layouts
- ✅ Performance optimized

**Kullanıma hazır!** 🚀
