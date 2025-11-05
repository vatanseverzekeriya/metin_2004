# Metin2 Mobil Dokunmatik Kontrol - Tam Entegrasyon Rehberi

Bu rehber, Metin2 oyununuzun mobil versiyonunda dokunmatik kontrollerin nasıl entegre edileceğini adım adım gösterir.

## 📑 İçindekiler

1. [Hızlı Başlangıç](#hızlı-başlangıç)
2. [Oyuncu Hareketi](#oyuncu-hareketi)
3. [Saldırı ve Beceri Sistemi](#saldırı-ve-beceri-sistemi)
4. [Menü Navigasyonu](#menü-navigasyonu)
5. [Sunucu Entegrasyonu](#sunucu-entegrasyonu)
6. [Platform Entegrasyonu](#platform-entegrasyonu)
7. [Performans Optimizasyonu](#performans-optimizasyonu)

---

## 🚀 Hızlı Başlangıç

### Minimum Kod - Oyunu Başlatma

```cpp
#include "mobile/TouchInput.h"
#include "mobile/GameControls.h"

void InitGame() {
    // Touch input başlat
    CTouchInput::Instance().Initialize(1920, 1080);

    // Game controls başlat
    CGameControls::Instance().Initialize(1920, 1080);

    // Hazır!
}

void GameLoop(float delta_time) {
    // Update
    CTouchInput::Instance().Update(delta_time * 1000);
    CGameControls::Instance().Update(delta_time * 1000);

    // Render
    CGameControls::Instance().Render();
}

// Platform'dan touch event geldiğinde
void OnPlatformTouchEvent(int id, float x, float y, int type) {
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

**Bu kadar!** Artık oyununuz mobil dokunmatik kontrollerle çalışıyor.

---

## 🎮 Oyuncu Hareketi

### Virtual Joystick ile Karakter Kontrolü

```cpp
void SetupMovement() {
    // Movement callback ayarla
    CGameControls::Instance().SetOnMoveCallback([](float dx, float dy) {
        // dx, dy = -1.0 ile 1.0 arası normalize edilmiş yön

        // Karakteri hareket ettir
        float speed = 300.0f; // piksel/saniye
        float new_x = player->GetX() + dx * speed * delta_time;
        float new_y = player->GetY() + dy * speed * delta_time;

        player->Move(new_x, new_y);

        // Sunucuya gönder
        SendMovePacket(new_x, new_y);
    });
}
```

### 8 Yönlü Hareket (Metin2 Tarzı)

```cpp
void SetupMetin2Movement() {
    auto* joystick = CGameControls::Instance().GetMovementJoystick();

    joystick->SetCallback([](const JoystickState& state) {
        if (!state.active) {
            player->StopMoving();
            return;
        }

        // 8 yön al (0-7)
        BYTE direction = state.Get8Direction();
        // 0=Sağ, 1=SağAlt, 2=Alt, 3=SolAlt, 4=Sol, 5=SolÜst, 6=Üst, 7=SağÜst

        player->SetDirection(direction);
        player->StartMoving();

        // Metin2 protokolü ile gönder
        TPacketCGMove packet;
        packet.header = HEADER_CG_MOVE;
        packet.direction = direction;
        packet.x = player->GetX();
        packet.y = player->GetY();
        SendPacket(&packet, sizeof(packet));
    });
}
```

### Joystick Özelleştirme

```cpp
void CustomizeJoystick() {
    auto* joystick = CGameControls::Instance().GetMovementJoystick();

    // Dead zone (merkezdeki hassasiyetsiz alan)
    joystick->SetDeadZone(0.15f); // %15

    // Floating mode (ilk dokunulan yerde belirir)
    joystick->SetFloating(true);

    // Auto recenter (bırakınca merkeze dön)
    joystick->SetAutoRecenter(true);

    // Görünüm
    joystick->SetBaseColor(0x80FFFFFF);   // Yarı saydam beyaz
    joystick->SetStickColor(0xFFFF0000);  // Kırmızı
    joystick->SetAlpha(0.7f);             // %70 opaklık
}
```

---

## ⚔️ Saldırı ve Beceri Sistemi

### Basic Saldırı

```cpp
void SetupAttack() {
    CGameControls::Instance().SetOnAttackCallback([]() {
        auto* target = player->GetTarget();
        if (!target) {
            ShowMessage("Hedef seçilmedi!");
            return;
        }

        // Mesafe kontrolü
        float distance = player->GetDistanceTo(target);
        if (distance > 200.0f) {
            ShowMessage("Hedef çok uzak!");
            return;
        }

        // Saldır
        player->Attack(target);

        // Sunucuya gönder
        TPacketCGAttack packet;
        packet.header = HEADER_CG_ATTACK;
        packet.target_id = target->GetID();
        SendPacket(&packet, sizeof(packet));
    });
}
```

### Auto-Attack

```cpp
void EnableAutoAttack(bool enable) {
    CGameControls::Instance().SetAutoAttack(enable);

    // Auto-attack interval ayarla (opsiyonel)
    // Default: 1000ms (1 saniye)
}
```

### Skill Kullanımı

```cpp
void SetupSkills() {
    // Skill kullanımı callback
    CGameControls::Instance().SetOnSkillCallback([](int skill_index) {
        // Skill bilgilerini al
        DWORD skill_id = player->GetSkillID(skill_index);
        auto* skill_data = GetSkillData(skill_id);

        // MP kontrolü
        if (player->GetMP() < skill_data->mp_cost) {
            ShowMessage("Yeterli MP yok!");
            return;
        }

        // Cooldown kontrolü
        auto* btn = CGameControls::Instance().GetSkillButton(skill_index);
        if (btn && btn->IsOnCooldown()) {
            ShowMessage("Beceri cooldown'da!");
            return;
        }

        // Target kontrolü (gerekirse)
        auto* target = player->GetTarget();
        if (skill_data->requires_target && !target) {
            ShowMessage("Hedef seçilmedi!");
            return;
        }

        // Skill kullan
        player->UseSkill(skill_id, target);

        // Cooldown başlat
        if (btn) {
            btn->SetCooldown(skill_data->cooldown);
        }

        // Sunucuya gönder
        TPacketCGSkill packet;
        packet.header = HEADER_CG_SKILL;
        packet.skill_id = skill_id;
        packet.target_id = target ? target->GetID() : 0;
        SendPacket(&packet, sizeof(packet));
    });

    // Quick slot'ları ayarla
    for (int i = 0; i < 6; i++) {
        DWORD skill_id = player->GetSkillID(i);
        DWORD icon_id = GetSkillIcon(skill_id);
        CGameControls::Instance().SetQuickSlot(i, skill_id, icon_id);
    }
}
```

### Skill Cooldown Güncelleme

```cpp
// Sunucudan cooldown güncellemesi geldiğinde
void OnSkillCooldownUpdate(int skill_index, float cooldown_sec) {
    auto* btn = CGameControls::Instance().GetSkillButton(skill_index);
    if (btn) {
        btn->SetCooldown(cooldown_sec);
    }
}
```

---

## 📦 Menü Navigasyonu

### Inventory Yönetimi

```cpp
void SetupInventory() {
    // Inventory boyutu ayarla
    CMenuNavigation::Instance().SetInventorySize(5, 8); // 5 satır, 8 sütun

    // Slot click callback
    CMenuNavigation::Instance().SetSlotClickCallback([](int slot_index) {
        auto* item = GetInventoryItem(slot_index);
        if (item) {
            ShowItemTooltip(item);
        }
    });

    // Drag-drop callback (item taşıma)
    CMenuNavigation::Instance().SetDragDropCallback([](int from_slot, int to_slot) {
        // Client-side swap
        SwapInventoryItems(from_slot, to_slot);

        // Sunucuya gönder
        TPacketCGItemMove packet;
        packet.header = HEADER_CG_ITEM_MOVE;
        packet.from_slot = from_slot;
        packet.to_slot = to_slot;
        SendPacket(&packet, sizeof(packet));
    });

    // Double-tap ile item kullanımı
    CMenuNavigation::Instance().SetItemUseCallback([](int slot_index, DWORD item_id) {
        // Item kullan
        UseItem(slot_index, item_id);

        // Sunucuya gönder
        TPacketCGItemUse packet;
        packet.header = HEADER_CG_ITEM_USE;
        packet.slot = slot_index;
        packet.item_id = item_id;
        SendPacket(&packet, sizeof(packet));
    });
}
```

### Inventory Açma/Kapama

```cpp
void ToggleInventory() {
    if (CMenuNavigation::Instance().IsMenuOpen()) {
        CMenuNavigation::Instance().CloseMenu();
        CGameControls::Instance().ShowHUD(true);  // HUD'u göster
    } else {
        CMenuNavigation::Instance().OpenMenu(MENU_INVENTORY);
        CGameControls::Instance().ShowHUD(false); // HUD'u gizle
    }
}

// Inventory button'a bağla
auto* invBtn = CGameControls::Instance().GetInventoryButton();
if (invBtn) {
    invBtn->SetClickCallback(ToggleInventory);
}
```

### Inventory Item Güncelleme

```cpp
// Sunucudan inventory update geldiğinde
void OnInventoryUpdate(int slot, DWORD item_id, DWORD count) {
    if (item_id == 0) {
        // Slot boş
        CMenuNavigation::Instance().ClearInventorySlot(slot);
    } else {
        // Item bilgilerini al
        auto* item_data = GetItemData(item_id);
        DWORD icon_id = item_data->icon_id;

        // Slot'u güncelle
        CMenuNavigation::Instance().SetInventoryItem(slot, item_id, icon_id, count);
    }
}
```

---

## 🌐 Sunucu Entegrasyonu

### Paket Gönderme

```cpp
// Movement paketi
void SendMovePacket(float x, float y, BYTE direction) {
    TPacketCGMove packet;
    packet.header = HEADER_CG_MOVE;
    packet.x = x;
    packet.y = y;
    packet.direction = direction;

    CNetworkManager::Instance().SendPacket(&packet, sizeof(packet));
}

// Attack paketi
void SendAttackPacket(DWORD target_id) {
    TPacketCGAttack packet;
    packet.header = HEADER_CG_ATTACK;
    packet.target_id = target_id;

    CNetworkManager::Instance().SendPacket(&packet, sizeof(packet));
}

// Skill paketi
void SendSkillPacket(DWORD skill_id, DWORD target_id) {
    TPacketCGSkill packet;
    packet.header = HEADER_CG_SKILL;
    packet.skill_id = skill_id;
    packet.target_id = target_id;

    CNetworkManager::Instance().SendPacket(&packet, sizeof(packet));
}
```

### Paket Alma

```cpp
void OnReceivePacket(BYTE header, void* data, size_t size) {
    switch (header) {
    case HEADER_GC_MOVE:
        {
            auto* packet = (TPacketGCMove*)data;
            OnCharacterMove(packet->id, packet->x, packet->y, packet->direction);
        }
        break;

    case HEADER_GC_ATTACK:
        {
            auto* packet = (TPacketGCAttack*)data;
            OnCharacterAttack(packet->attacker_id, packet->target_id, packet->damage);
        }
        break;

    case HEADER_GC_SKILL:
        {
            auto* packet = (TPacketGCSkill*)data;
            OnCharacterSkill(packet->caster_id, packet->skill_id, packet->target_id);
        }
        break;

    case HEADER_GC_HP_UPDATE:
        {
            auto* packet = (TPacketGCHPUpdate*)data;
            OnHPUpdate(packet->character_id, packet->hp, packet->max_hp);
        }
        break;

    case HEADER_GC_ITEM_UPDATE:
        {
            auto* packet = (TPacketGCItemUpdate*)data;
            OnInventoryUpdate(packet->slot, packet->item_id, packet->count);
        }
        break;
    }
}
```

---

## 📱 Platform Entegrasyonu

### Android (Kotlin + JNI)

#### TouchHandler.kt

```kotlin
package com.metin2.mobile

import android.view.MotionEvent
import android.view.View

class TouchHandler(private val nativePtr: Long) : View.OnTouchListener {

    override fun onTouch(v: View?, event: MotionEvent?): Boolean {
        event ?: return false

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val index = event.actionIndex
                val id = event.getPointerId(index)
                val x = event.getX(index)
                val y = event.getY(index)
                nativeOnTouchDown(nativePtr, id, x, y)
            }

            MotionEvent.ACTION_MOVE -> {
                for (i in 0 until event.pointerCount) {
                    val id = event.getPointerId(i)
                    val x = event.getX(i)
                    val y = event.getY(i)
                    nativeOnTouchMove(nativePtr, id, x, y)
                }
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                val index = event.actionIndex
                val id = event.getPointerId(index)
                val x = event.getX(index)
                val y = event.getY(index)
                nativeOnTouchUp(nativePtr, id, x, y)
            }

            MotionEvent.ACTION_CANCEL -> {
                for (i in 0 until event.pointerCount) {
                    val id = event.getPointerId(i)
                    nativeOnTouchCancel(nativePtr, id)
                }
            }
        }

        return true
    }

    private external fun nativeOnTouchDown(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchMove(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchUp(ptr: Long, id: Int, x: Float, y: Float)
    private external fun nativeOnTouchCancel(ptr: Long, id: Int)

    companion object {
        init {
            System.loadLibrary("metin2_mobile")
        }
    }
}
```

#### JNI Bridge (C++)

```cpp
// jni_bridge.cpp
#include <jni.h>
#include "mobile/TouchInput.h"
#include "mobile/GameControls.h"

extern "C" {

JNIEXPORT void JNICALL
Java_com_metin2_mobile_TouchHandler_nativeOnTouchDown(
    JNIEnv* env, jobject obj, jlong ptr, jint id, jfloat x, jfloat y)
{
    TouchPoint touch(id, x, y, TOUCH_DOWN);
    CTouchInput::Instance().OnTouchDown(id, x, y);
    CGameControls::Instance().OnTouchDown(touch);
}

JNIEXPORT void JNICALL
Java_com_metin2_mobile_TouchHandler_nativeOnTouchMove(
    JNIEnv* env, jobject obj, jlong ptr, jint id, jfloat x, jfloat y)
{
    TouchPoint touch(id, x, y, TOUCH_MOVE);
    CTouchInput::Instance().OnTouchMove(id, x, y);
    CGameControls::Instance().OnTouchMove(touch);
}

JNIEXPORT void JNICALL
Java_com_metin2_mobile_TouchHandler_nativeOnTouchUp(
    JNIEnv* env, jobject obj, jlong ptr, jint id, jfloat x, jfloat y)
{
    TouchPoint touch(id, x, y, TOUCH_UP);
    CTouchInput::Instance().OnTouchUp(id, x, y);
    CGameControls::Instance().OnTouchUp(touch);
}

JNIEXPORT void JNICALL
Java_com_metin2_mobile_TouchHandler_nativeOnTouchCancel(
    JNIEnv* env, jobject obj, jlong ptr, jint id)
{
    CTouchInput::Instance().OnTouchCancel(id);
}

} // extern "C"
```

### iOS (Swift + Objective-C++)

#### TouchHandler.swift

```swift
import UIKit

class GameView: UIView {
    var nativePtr: UnsafeMutableRawPointer?
    private var activeTouches: [UITouch: UInt32] = [:]

    override init(frame: CGRect) {
        super.init(frame: frame)
        isMultipleTouchEnabled = true
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        isMultipleTouchEnabled = true
    }

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            let id = generateTouchID(for: touch)
            activeTouches[touch] = id

            nativeOnTouchDown(nativePtr, id, Float(location.x), Float(location.y))
        }
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            guard let id = activeTouches[touch] else { continue }
            let location = touch.location(in: self)

            nativeOnTouchMove(nativePtr, id, Float(location.x), Float(location.y))
        }
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            guard let id = activeTouches[touch] else { continue }
            let location = touch.location(in: self)

            nativeOnTouchUp(nativePtr, id, Float(location.x), Float(location.y))
            activeTouches.removeValue(forKey: touch)
        }
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            guard let id = activeTouches[touch] else { continue }
            nativeOnTouchCancel(nativePtr, id)
            activeTouches.removeValue(forKey: touch)
        }
    }

    private func generateTouchID(for touch: UITouch) -> UInt32 {
        return UInt32(touch.hash & 0xFFFFFFFF)
    }
}
```

#### Bridge (Objective-C++)

```objc
// ios_bridge.mm
#import <Foundation/Foundation.h>
#include "mobile/TouchInput.h"
#include "mobile/GameControls.h"

extern "C" {

void nativeOnTouchDown(void* ptr, uint32_t id, float x, float y) {
    TouchPoint touch(id, x, y, TOUCH_DOWN);
    CTouchInput::Instance().OnTouchDown(id, x, y);
    CGameControls::Instance().OnTouchDown(touch);
}

void nativeOnTouchMove(void* ptr, uint32_t id, float x, float y) {
    TouchPoint touch(id, x, y, TOUCH_MOVE);
    CTouchInput::Instance().OnTouchMove(id, x, y);
    CGameControls::Instance().OnTouchMove(touch);
}

void nativeOnTouchUp(void* ptr, uint32_t id, float x, float y) {
    TouchPoint touch(id, x, y, TOUCH_UP);
    CTouchInput::Instance().OnTouchUp(id, x, y);
    CGameControls::Instance().OnTouchUp(touch);
}

void nativeOnTouchCancel(void* ptr, uint32_t id) {
    CTouchInput::Instance().OnTouchCancel(id);
}

} // extern "C"
```

---

## ⚡ Performans Optimizasyonu

### Touch Event Batching

```cpp
class TouchEventBatcher {
private:
    std::vector<TouchPoint> m_pendingEvents;
    std::mutex m_mutex;

public:
    void AddEvent(const TouchPoint& touch) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingEvents.push_back(touch);
    }

    void ProcessEvents() {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (const auto& touch : m_pendingEvents) {
            switch (touch.type) {
            case TOUCH_DOWN:
                CTouchInput::Instance().OnTouchDown(touch.id, touch.x, touch.y);
                break;
            case TOUCH_MOVE:
                CTouchInput::Instance().OnTouchMove(touch.id, touch.x, touch.y);
                break;
            case TOUCH_UP:
                CTouchInput::Instance().OnTouchUp(touch.id, touch.x, touch.y);
                break;
            }
        }

        m_pendingEvents.clear();
    }
};
```

### Low Power Mode

```cpp
void SetLowPowerMode(bool enable) {
    if (enable) {
        // Update interval'i azalt
        CGameControls::Instance().GetMovementJoystick()->SetUpdateInterval(100); // 10 FPS

        // Efektleri azalt
        CGameControls::Instance().GetMovementJoystick()->SetAlpha(0.3f);

        // Auto-attack zorla
        CGameControls::Instance().SetAutoAttack(true);

        // Skill animasyonlarını kısa tut
        // ...
    }
}
```

### Memory Pooling

```cpp
class UIButtonPool {
private:
    std::vector<std::unique_ptr<CUIButton>> m_pool;
    std::vector<CUIButton*> m_available;

public:
    CUIButton* Allocate() {
        if (m_available.empty()) {
            m_pool.push_back(std::make_unique<CUIButton>());
            return m_pool.back().get();
        }

        CUIButton* btn = m_available.back();
        m_available.pop_back();
        return btn;
    }

    void Free(CUIButton* btn) {
        m_available.push_back(btn);
    }
};
```

---

## 🎯 Best Practices

### 1. Touch Hiyerarşisi

```cpp
void OnTouchEvent(const TouchPoint& touch) {
    // Önce üstteki UI elementleri kontrol et
    if (CMenuNavigation::Instance().IsMenuOpen()) {
        CMenuNavigation::Instance().OnTouchDown(touch);
        return; // Menu açıksa game controls'e gönderme
    }

    // Sonra game controls
    CGameControls::Instance().OnTouchDown(touch);
}
```

### 2. Thread Safety

```cpp
// UI thread'den game thread'e event gönderme
void OnTouchFromUIThread(const TouchPoint& touch) {
    // Queue'ya ekle
    m_touchEventQueue.push(touch);
}

void ProcessTouchEventsInGameThread() {
    while (!m_touchEventQueue.empty()) {
        TouchPoint touch = m_touchEventQueue.front();
        m_touchEventQueue.pop();

        // Game thread'de işle
        CGameControls::Instance().OnTouchDown(touch);
    }
}
```

### 3. Haptic Feedback

```cpp
void EnableHapticFeedback() {
    // Attack button
    auto* attackBtn = CGameControls::Instance().GetAttackButton();
    attackBtn->SetClickCallback([]() {
        TriggerHapticFeedback(HAPTIC_MEDIUM);
        // ... attack logic
    });

    // Skill buttons
    for (int i = 0; i < 6; i++) {
        auto* skillBtn = CGameControls::Instance().GetSkillButton(i);
        skillBtn->SetClickCallback([i]() {
            TriggerHapticFeedback(HAPTIC_LIGHT);
            // ... skill logic
        });
    }
}
```

---

## 📚 Ek Kaynaklar

- [MOBILE_UI.md](MOBILE_UI.md) - Detaylı API dokümantasyonu
- [examples/mobile_ui/test_mobile_ui.cpp](examples/mobile_ui/test_mobile_ui.cpp) - Test suite
- [examples/mobile_ui/complete_integration_example.cpp](examples/mobile_ui/complete_integration_example.cpp) - Tam entegrasyon örneği

---

## 🐛 Troubleshooting

### Touch event'leri çalışmıyor
- Platform bridge'i doğru bağlandığından emin olun
- Multi-touch enable olduğunu kontrol edin
- Z-order / touch priority kontrolü yapın

### Joystick hassasiyeti çok yüksek
- Dead zone'u artırın: `joystick->SetDeadZone(0.2f)`
- Update interval'i azaltın

### Performans sorunları
- Touch event batching kullanın
- Render batching yapın
- Low power mode'u enable edin

---

**Hazır!** Metin2 mobil versiyonunuz artık tam dokunmatik kontrol desteğine sahip! 🎉
