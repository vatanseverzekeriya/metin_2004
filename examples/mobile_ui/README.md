# 📱 Metin2 Mobile Touch Controls - Örnek Kullanım

Bu dizin, Metin2 oyunu için mobil dokunmatik kontrol sisteminin nasıl kullanılacağını gösteren örnekler içerir.

## 📁 Dosyalar

### Ana Örnekler
- **`metin2_mobile_integration.cpp`** - Tam entegrasyon örneği (EN DETAYLI)
- **`test_mobile_ui.cpp`** - Temel test programı

### Build Dosyaları
- **`CMakeLists.txt`** - CMake build script

## 🚀 Hızlı Başlangıç

### 1. Build (Derleme)

```bash
# Build dizini oluştur
cd examples/mobile_ui
mkdir build && cd build

# CMake ile configure et
cmake ..

# Derle
make

# Çalıştır
../../bin/metin2_mobile_integration
# veya
../../bin/test_mobile_ui
```

### 2. Temel Kullanım

```cpp
#include "mobile/GameControls.h"

// 1. Başlat
CGameControls::Instance().Initialize(1920, 1080);

// 2. Callback kaydet
CGameControls::Instance().SetOnMoveCallback([](float x, float y) {
    // Hareket kodu
});

CGameControls::Instance().SetOnAttackCallback([]() {
    // Saldırı kodu
});

// 3. Game loop
void Update(float dt) {
    CGameControls::Instance().Update(dt * 1000);
}

void Render() {
    CGameControls::Instance().Render();
}

// 4. Touch events (platform'dan)
void OnTouch(int id, float x, float y, int type) {
    TouchPoint touch(id, x, y, (ETouchType)type);

    if (type == TOUCH_DOWN)
        CGameControls::Instance().OnTouchDown(touch);
    else if (type == TOUCH_MOVE)
        CGameControls::Instance().OnTouchMove(touch);
    else if (type == TOUCH_UP)
        CGameControls::Instance().OnTouchUp(touch);
}
```

## 🎮 Özellikler

### ✅ Oyuncu Hareket
- **8 Yönlü Joystick** (Metin2 tarzı)
- Floating/Fixed mode
- Dead zone desteği
- Auto-recenter

```cpp
auto* joystick = CGameControls::Instance().GetMovementJoystick();
joystick->SetFloating(true);
joystick->SetDeadZone(0.15f);
```

### ✅ Saldırı Sistemi
- Attack button
- Auto-attack mode
- Target selection

```cpp
CGameControls::Instance().SetAutoAttack(true);
```

### ✅ Skill Kullanımı
- 6 Quick slot
- Cooldown gösterimi
- Skill icon desteği

```cpp
CGameControls::Instance().SetQuickSlot(0, skill_vnum, icon_id);
```

### ✅ Menü Navigasyonu
- Inventory
- Character
- Quest
- Guild
- Settings

```cpp
auto* inventoryBtn = CGameControls::Instance().GetInventoryButton();
inventoryBtn->SetClickCallback([]() {
    ShowInventory();
});
```

### ✅ Gesture Desteği
- Tap / Double Tap
- Long Press
- Swipe (4 yön)
- Pinch (zoom)

```cpp
CTouchInput::Instance().RegisterGestureCallback([](const GestureInfo& gesture) {
    if (gesture.type == GESTURE_PINCH) {
        // Zoom
    }
});
```

## 📐 Layout Sistemi

### Default Layout (Sağ El)
```
┌──────────────────────────────────────┐
│  [⚙] [Q] [C] [I] [☰]        (Üst)  │
│                                      │
│                                      │
│  [JOY]                      [JUMP]  │
│  ════════════════════════   [PICK]  │
│  [1] [2] [3] [4] [5] [6]    [ATK]   │
└──────────────────────────────────────┘
```

### Left-Handed Layout
```
┌──────────────────────────────────────┐
│  [⚙] [Q] [C] [I] [☰]        (Üst)  │
│                                      │
│                                      │
│  [JUMP]                      [JOY]  │
│  [PICK]   ════════════════════════   │
│  [ATK]    [1] [2] [3] [4] [5] [6]   │
└──────────────────────────────────────┘
```

```cpp
// Layout değiştir
CGameControls::Instance().SetLayout(1); // 0=default, 1=left-handed
```

## 🔧 Platform Entegrasyonu

### Android (JNI)

```cpp
extern "C" {
    void Java_com_metin2_MobileGame_nativeOnTouchDown(
        JNIEnv* env, jobject obj, jint id, jfloat x, jfloat y)
    {
        g_pGameClient->OnNativeTouchEvent(id, x, y, TOUCH_DOWN);
    }
}
```

### iOS (Objective-C++)

```objc
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        CGPoint location = [touch locationInView:self.view];
        g_pGameClient->OnNativeTouchEvent(
            (int)touch.hash, location.x, location.y, TOUCH_DOWN);
    }
}
```

## ⚡ Performans Optimizasyonu

### Batarya Tasarrufu
```cpp
void EnablePowerSaveMode() {
    // Auto-attack aç
    CGameControls::Instance().SetAutoAttack(true);

    // Joystick alpha düşür
    auto* joy = CGameControls::Instance().GetMovementJoystick();
    joy->SetAlpha(0.3f);

    // Update frekansını azalt (30 FPS)
}
```

### Ekran Boyutu Adaptasyonu
```cpp
void AdaptToScreen(int width, int height) {
    // Tablet için büyük butonlar
    if (width >= 2048) {
        joystick->SetRadius(150);
        attackBtn->SetSize(150, 150);
    }
    // Telefon için küçük butonlar
    else if (width < 1280) {
        joystick->SetRadius(80);
        attackBtn->SetSize(80, 80);
    }
}
```

## 📊 Test Etme

### Simülasyon Testi
```bash
# Tam entegrasyon testini çalıştır
./bin/metin2_mobile_integration

# Basit UI testini çalıştır
./bin/test_mobile_ui
```

### Çıktı Örneği
```
=== Metin2 Mobile Controls Initialization ===
TouchInput initialized: 1920x1080
VirtualJoystick initialized at (200,880) radius:120
Default layout created
Setting up skill slots...
  Slot 0: Aura of the Sword
  Slot 1: Dash
  ...

--- Simulating Touch Events ---
1. Movement Simulation:
[Joystick] Activated
[Player] Moving in direction: 6
[Network] MOVE packet: pos(0,0) dir:6

2. Attack Simulation:
[Button] Pressed: ATK
[Player] Attacking target: 12345
[Network] ATTACK packet: target=12345

✅ Test Complete!
```

## 📱 Mobil Cihazlarda Test

### Android
```bash
# APK build
cd android
./gradlew assembleDebug

# Install & Run
adb install app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.metin2.mobile/.MainActivity
```

### iOS
```bash
# Xcode project aç
open ios/Metin2Mobile.xcodeproj

# Build & Run (Xcode'da)
# Product -> Run (Cmd+R)
```

## 🎨 Özelleştirme

### Buton Renkleri
```cpp
attackBtn->SetColor(
    0xFFFF0000,  // Normal (kırmızı)
    0xFFCC0000,  // Pressed (koyu kırmızı)
    0xFF888888   // Disabled (gri)
);
```

### Joystick Görünümü
```cpp
joystick->SetBaseColor(0x80FFFFFF);   // Yarı saydam beyaz
joystick->SetStickColor(0xFFFF6B00);  // Turuncu stick
joystick->SetAlpha(0.7f);             // %70 opaklık
```

## 📚 Daha Fazla Bilgi

- **Ana Dokümantasyon:** `/MOBILE_UI.md`
- **Header Dosyaları:** `/include/mobile/*.h`
- **Kaynak Kodlar:** `/src/mobile/*.cpp`

## 🐛 Sorun Giderme

### Problem: Joystick çalışmıyor
```cpp
// Floating mode'u dene
joystick->SetFloating(true);

// Dead zone'u azalt
joystick->SetDeadZone(0.0f);
```

### Problem: Butonlar basılmıyor
```cpp
// Hit test debug
bool hit = button->Contains(touch.x, touch.y);
std::cout << "Hit: " << hit << std::endl;
```

### Problem: Cooldown çalışmıyor
```cpp
// Update çağrıldığından emin ol
button->Update(delta_time_ms);
```

## 📝 Lisans

Bu kod Metin2 PvP projesi içindir.

---

**🎮 İyi Oyunlar!**
