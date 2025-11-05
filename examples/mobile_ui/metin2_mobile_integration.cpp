/**
 * Metin2 Mobile Touch Controls - Tam Entegrasyon Örneği
 *
 * Bu dosya Metin2 oyununa mobil dokunmatik kontrollerinin
 * nasıl entegre edileceğini gösterir.
 */

#include "../../include/mobile/TouchInput.h"
#include "../../include/mobile/VirtualJoystick.h"
#include "../../include/mobile/UIButton.h"
#include "../../include/mobile/GameControls.h"
#include <iostream>
#include <cmath>

// Metin2 Player class (örnek)
class CPlayer {
public:
    void Move(BYTE direction) {
        std::cout << "[Player] Moving in direction: " << (int)direction << std::endl;
    }

    void Attack(DWORD target_vid) {
        std::cout << "[Player] Attacking target: " << target_vid << std::endl;
    }

    void UseSkill(DWORD skill_vnum, DWORD target_vid = 0) {
        std::cout << "[Player] Using skill " << skill_vnum
                  << " on target " << target_vid << std::endl;
    }

    void Jump() {
        std::cout << "[Player] Jumping!" << std::endl;
    }

    void PickupItem() {
        std::cout << "[Player] Picking up item" << std::endl;
    }

    DWORD GetTarget() { return m_dwTargetVID; }
    void SetTarget(DWORD vid) { m_dwTargetVID = vid; }

    float GetX() { return m_fX; }
    float GetY() { return m_fY; }

    DWORD GetSkillVnum(int slot) {
        return slot < 6 ? m_Skills[slot] : 0;
    }

    void SetSkillSlot(int slot, DWORD vnum) {
        if (slot >= 0 && slot < 6)
            m_Skills[slot] = vnum;
    }

private:
    float m_fX = 0.0f;
    float m_fY = 0.0f;
    DWORD m_dwTargetVID = 0;
    DWORD m_Skills[6] = {1, 2, 3, 4, 5, 6}; // Skill vnumları
};

// Global player instance
CPlayer* g_pPlayer = nullptr;

/**
 * Network Packet Gönderme Fonksiyonları
 */
void SendMovePacket(float x, float y, BYTE direction) {
    std::cout << "[Network] MOVE packet: pos(" << x << "," << y
              << ") dir:" << (int)direction << std::endl;
}

void SendAttackPacket(DWORD target_vid) {
    std::cout << "[Network] ATTACK packet: target=" << target_vid << std::endl;
}

void SendSkillPacket(DWORD skill_vnum, DWORD target_vid) {
    std::cout << "[Network] SKILL packet: skill=" << skill_vnum
              << " target=" << target_vid << std::endl;
}

/**
 * UI Window Fonksiyonları
 */
void ShowInventoryWindow() {
    std::cout << "[UI] Opening Inventory Window" << std::endl;
}

void ShowCharacterWindow() {
    std::cout << "[UI] Opening Character Window" << std::endl;
}

void ShowQuestWindow() {
    std::cout << "[UI] Opening Quest Window" << std::endl;
}

void ShowGuildWindow() {
    std::cout << "[UI] Opening Guild Window" << std::endl;
}

void ShowSettingsWindow() {
    std::cout << "[UI] Opening Settings Window" << std::endl;
}

void ShowMessage(const std::string& msg) {
    std::cout << "[Message] " << msg << std::endl;
}

/**
 * Ana Oyun Sınıfı
 */
class CMobileGameClient {
public:
    CMobileGameClient()
        : m_fScreenWidth(1920.0f)
        , m_fScreenHeight(1080.0f)
        , m_bInitialized(false)
    {
        g_pPlayer = &m_Player;
    }

    ~CMobileGameClient() {
        g_pPlayer = nullptr;
    }

    /**
     * Mobil kontrolleri başlat
     */
    void InitializeMobileControls() {
        std::cout << "\n=== Metin2 Mobile Controls Initialization ===" << std::endl;

        // Touch input sistemi
        CTouchInput::Instance().Initialize(m_fScreenWidth, m_fScreenHeight);

        // Game controls sistemi
        CGameControls::Instance().Initialize(m_fScreenWidth, m_fScreenHeight);

        // Callback'leri kaydet
        SetupCallbacks();

        // Skill slotları ayarla
        SetupSkillSlots();

        // Menu butonlarını ayarla
        SetupMenuButtons();

        // Özel gesture'ları kaydet
        SetupGestures();

        m_bInitialized = true;

        std::cout << "=== Mobile Controls Ready! ===" << std::endl;
    }

    /**
     * Hareket callback'i
     */
    void SetupCallbacks() {
        // HAREKET CALLBACK
        CGameControls::Instance().SetOnMoveCallback([this](float dx, float dy) {
            if (!g_pPlayer) return;

            // 8 yönlü hareket için açı hesapla
            float angle = atan2f(dy, dx);

            // Açıyı 8 yöne dönüştür (0-7)
            // 0=Sağ, 1=SağAlt, 2=Alt, 3=SolAlt, 4=Sol, 5=SolÜst, 6=Üst, 7=SağÜst
            int degrees = (int)(angle * 180.0f / M_PI);
            if (degrees < 0) degrees += 360;
            BYTE direction = (BYTE)((degrees + 22) / 45) % 8;

            // Karakteri hareket ettir
            g_pPlayer->Move(direction);

            // Network'e gönder
            SendMovePacket(g_pPlayer->GetX(), g_pPlayer->GetY(), direction);
        });

        // SALDIRI CALLBACK
        CGameControls::Instance().SetOnAttackCallback([this]() {
            if (!g_pPlayer) return;

            DWORD target = g_pPlayer->GetTarget();
            if (target == 0) {
                ShowMessage("Hedef yok! Önce bir düşman seç.");
                return;
            }

            // Saldır
            g_pPlayer->Attack(target);
            SendAttackPacket(target);
        });

        // SKILL CALLBACK
        CGameControls::Instance().SetOnSkillCallback([this](int skill_index) {
            if (!g_pPlayer) return;

            DWORD skill_vnum = g_pPlayer->GetSkillVnum(skill_index);
            if (skill_vnum == 0) {
                ShowMessage("Bu slotta skill yok!");
                return;
            }

            // Skill cooldown kontrolü
            auto* skillBtn = CGameControls::Instance().GetSkillButton(skill_index);
            if (skillBtn && skillBtn->IsOnCooldown()) {
                ShowMessage("Skill cooldown'da!");
                return;
            }

            DWORD target = g_pPlayer->GetTarget();

            // Skill kullan
            g_pPlayer->UseSkill(skill_vnum, target);
            SendSkillPacket(skill_vnum, target);

            // Cooldown başlat (örnek: 5 saniye)
            if (skillBtn) {
                float cooldown = 5.0f; // Gerçekte skill'e göre değişir
                skillBtn->SetCooldown(cooldown);
            }
        });
    }

    /**
     * Skill slotlarını ayarla
     */
    void SetupSkillSlots() {
        std::cout << "Setting up skill slots..." << std::endl;

        // Örnek skill'ler (gerçekte player'dan alınır)
        struct SkillData {
            int slot;
            DWORD vnum;
            DWORD icon_id;
            const char* name;
        };

        SkillData skills[] = {
            {0, 1, 101, "Aura of the Sword"},     // Warrior skill
            {1, 2, 102, "Dash"},
            {2, 3, 103, "Sword Strike"},
            {3, 4, 104, "Triple Slash"},
            {4, 5, 105, "Tenacity"},
            {5, 6, 106, "Berserker"}
        };

        for (const auto& skill : skills) {
            g_pPlayer->SetSkillSlot(skill.slot, skill.vnum);
            CGameControls::Instance().SetQuickSlot(skill.slot, skill.vnum, skill.icon_id);
            std::cout << "  Slot " << skill.slot << ": " << skill.name << std::endl;
        }
    }

    /**
     * Menu butonlarını ayarla
     */
    void SetupMenuButtons() {
        std::cout << "Setting up menu buttons..." << std::endl;

        // Inventory button
        auto* inventoryBtn = CGameControls::Instance().GetInventoryButton();
        if (inventoryBtn) {
            inventoryBtn->SetClickCallback([]() {
                ShowInventoryWindow();
            });
        }

        // Character button
        auto* charBtn = CGameControls::Instance().GetSkillButton(0);
        if (charBtn) {
            // Bu normalde ayrı bir buton olmalı, örnek için skill button kullandık
        }

        // Settings button (Game controls'da zaten var)
        // CGameControls::Instance().GetSettingsButton()->SetClickCallback(...);

        std::cout << "  Menu buttons ready" << std::endl;
    }

    /**
     * Özel gesture'ları kaydet
     */
    void SetupGestures() {
        std::cout << "Setting up gestures..." << std::endl;

        CTouchInput::Instance().RegisterGestureCallback([this](const GestureInfo& gesture) {
            switch (gesture.type) {
                case GESTURE_DOUBLE_TAP:
                    // Çift dokunma: Hedef seç veya autopick
                    std::cout << "[Gesture] Double tap at ("
                              << gesture.current_x << "," << gesture.current_y << ")" << std::endl;
                    if (g_pPlayer) {
                        g_pPlayer->PickupItem();
                    }
                    break;

                case GESTURE_LONG_PRESS:
                    // Uzun basma: Item menüsü aç
                    std::cout << "[Gesture] Long press" << std::endl;
                    ShowMessage("Item menüsü açılıyor...");
                    break;

                case GESTURE_SWIPE:
                    // Swipe: Skill değiştir veya kamera çevir
                    if (gesture.swipe_dir == SWIPE_LEFT) {
                        std::cout << "[Gesture] Swipe LEFT - Previous skill page" << std::endl;
                    } else if (gesture.swipe_dir == SWIPE_RIGHT) {
                        std::cout << "[Gesture] Swipe RIGHT - Next skill page" << std::endl;
                    }
                    break;

                case GESTURE_PINCH:
                    // Pinch: Kamera zoom
                    std::cout << "[Gesture] Pinch - Zoom scale: " << gesture.scale << std::endl;
                    // SetCameraZoom(gesture.scale);
                    break;

                default:
                    break;
            }
        });

        std::cout << "  Gestures ready" << std::endl;
    }

    /**
     * Oyun döngüsü - Update
     */
    void Update(float deltaTime) {
        if (!m_bInitialized) return;

        DWORD delta_ms = (DWORD)(deltaTime * 1000.0f);

        // Touch input güncelle (gesture detection)
        CTouchInput::Instance().Update(delta_ms);

        // Game controls güncelle (cooldown, auto-attack, vb.)
        CGameControls::Instance().Update(delta_ms);
    }

    /**
     * Oyun döngüsü - Render
     */
    void Render() {
        if (!m_bInitialized) return;

        // Oyun dünyasını render et
        // RenderWorld();
        // RenderPlayers();
        // RenderMonsters();

        // UI render et (joystick, butonlar, cooldown overlays)
        CGameControls::Instance().Render();
    }

    /**
     * Platform'dan gelen touch event'leri
     * Android/iOS native code'dan çağrılır
     */
    void OnNativeTouchEvent(int id, float x, float y, int type) {
        if (!m_bInitialized) return;

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

            case TOUCH_CANCEL:
                CTouchInput::Instance().OnTouchCancel(id);
                break;
        }
    }

    /**
     * Layout değiştir (sağ/sol el)
     */
    void SwitchLayout(int layout_id) {
        CGameControls::Instance().SetLayout(layout_id);

        switch (layout_id) {
            case 0:
                ShowMessage("Layout: Sağ El (Default)");
                break;
            case 1:
                ShowMessage("Layout: Sol El");
                break;
            default:
                ShowMessage("Layout: Custom");
                break;
        }
    }

    /**
     * Auto-attack toggle
     */
    void ToggleAutoAttack() {
        bool current = CGameControls::Instance().IsAutoAttack();
        CGameControls::Instance().SetAutoAttack(!current);

        if (!current) {
            ShowMessage("Auto-Attack: AÇIK");
        } else {
            ShowMessage("Auto-Attack: KAPALI");
        }
    }

    /**
     * Ekran boyutu değiştiğinde (orientation change)
     */
    void OnScreenSizeChanged(int width, int height) {
        m_fScreenWidth = (float)width;
        m_fScreenHeight = (float)height;

        std::cout << "\n[Screen] Size changed: " << width << "x" << height << std::endl;

        // Yeniden başlat
        InitializeMobileControls();

        // Tablet için büyük butonlar
        if (width >= 2048) {
            auto* joystick = CGameControls::Instance().GetMovementJoystick();
            joystick->SetRadius(150.0f);

            auto* attackBtn = CGameControls::Instance().GetAttackButton();
            attackBtn->SetSize(150, 150);

            std::cout << "[UI] Tablet mode: Large buttons" << std::endl;
        }
        // Telefon için küçük butonlar
        else if (width < 1280) {
            auto* joystick = CGameControls::Instance().GetMovementJoystick();
            joystick->SetRadius(80.0f);

            auto* attackBtn = CGameControls::Instance().GetAttackButton();
            attackBtn->SetSize(80, 80);

            std::cout << "[UI] Phone mode: Small buttons" << std::endl;
        }
    }

    /**
     * Hedef seç
     */
    void SelectTarget(DWORD vid) {
        if (g_pPlayer) {
            g_pPlayer->SetTarget(vid);
            ShowMessage("Hedef seçildi!");
        }
    }

    /**
     * Batarya tasarrufu modu
     */
    void EnablePowerSaveMode(bool enable) {
        if (enable) {
            // Auto-attack aç (elle tıklama gerektirmez)
            CGameControls::Instance().SetAutoAttack(true);

            // Joystick'i daha az görünür yap
            auto* joystick = CGameControls::Instance().GetMovementJoystick();
            joystick->SetAlpha(0.3f);

            ShowMessage("Batarya Tasarrufu: AÇIK");
            std::cout << "[Power] Battery saver mode enabled" << std::endl;
        } else {
            auto* joystick = CGameControls::Instance().GetMovementJoystick();
            joystick->SetAlpha(0.7f);

            ShowMessage("Batarya Tasarrufu: KAPALI");
            std::cout << "[Power] Battery saver mode disabled" << std::endl;
        }
    }

private:
    CPlayer m_Player;
    float m_fScreenWidth;
    float m_fScreenHeight;
    bool m_bInitialized;
};

/**
 * JNI Export Fonksiyonları (Android için)
 */
extern "C" {
    CMobileGameClient* g_pGameClient = nullptr;

    // Game client oluştur
    void Java_com_metin2_MobileGame_nativeInit(int width, int height) {
        if (!g_pGameClient) {
            g_pGameClient = new CMobileGameClient();
        }
        g_pGameClient->OnScreenSizeChanged(width, height);
    }

    // Touch event'leri
    void Java_com_metin2_MobileGame_nativeOnTouchDown(int id, float x, float y) {
        if (g_pGameClient) {
            g_pGameClient->OnNativeTouchEvent(id, x, y, TOUCH_DOWN);
        }
    }

    void Java_com_metin2_MobileGame_nativeOnTouchMove(int id, float x, float y) {
        if (g_pGameClient) {
            g_pGameClient->OnNativeTouchEvent(id, x, y, TOUCH_MOVE);
        }
    }

    void Java_com_metin2_MobileGame_nativeOnTouchUp(int id, float x, float y) {
        if (g_pGameClient) {
            g_pGameClient->OnNativeTouchEvent(id, x, y, TOUCH_UP);
        }
    }

    // Update & Render
    void Java_com_metin2_MobileGame_nativeUpdate(float deltaTime) {
        if (g_pGameClient) {
            g_pGameClient->Update(deltaTime);
        }
    }

    void Java_com_metin2_MobileGame_nativeRender() {
        if (g_pGameClient) {
            g_pGameClient->Render();
        }
    }

    // Cleanup
    void Java_com_metin2_MobileGame_nativeDestroy() {
        if (g_pGameClient) {
            delete g_pGameClient;
            g_pGameClient = nullptr;
        }
    }
}

/**
 * Test Main (Desktop simulation)
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Metin2 Mobile Integration Example    " << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Game client oluştur
    CMobileGameClient gameClient;

    // Mobil kontrolleri başlat
    gameClient.InitializeMobileControls();

    std::cout << "\n--- Simulating Touch Events ---\n" << std::endl;

    // 1. Hareket simülasyonu
    std::cout << "1. Movement Simulation:" << std::endl;
    gameClient.OnNativeTouchEvent(1, 200, 880, TOUCH_DOWN);
    gameClient.OnNativeTouchEvent(1, 250, 830, TOUCH_MOVE);
    gameClient.OnNativeTouchEvent(1, 250, 830, TOUCH_UP);

    // 2. Attack simülasyonu
    std::cout << "\n2. Attack Simulation:" << std::endl;
    gameClient.SelectTarget(12345);  // Hedef seç
    gameClient.OnNativeTouchEvent(2, 1720, 880, TOUCH_DOWN);
    gameClient.OnNativeTouchEvent(2, 1720, 880, TOUCH_UP);

    // 3. Skill kullanımı
    std::cout << "\n3. Skill Usage Simulation:" << std::endl;
    gameClient.OnNativeTouchEvent(3, 900, 980, TOUCH_DOWN);
    gameClient.OnNativeTouchEvent(3, 900, 980, TOUCH_UP);

    // 4. Gesture simülasyonu
    std::cout << "\n4. Gesture Simulation:" << std::endl;
    CTouchInput::Instance().OnTouchDown(4, 500, 500);
    CTouchInput::Instance().OnTouchUp(4, 505, 505);  // Tap

    // 5. Layout değiştir
    std::cout << "\n5. Layout Switch:" << std::endl;
    gameClient.SwitchLayout(1);  // Sol el layout

    // 6. Auto-attack toggle
    std::cout << "\n6. Auto-Attack:" << std::endl;
    gameClient.ToggleAutoAttack();

    // 7. Update döngüsü simülasyonu
    std::cout << "\n7. Update Loop (5 frames):" << std::endl;
    for (int i = 0; i < 5; i++) {
        gameClient.Update(0.016f);  // 16ms = 60 FPS
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Complete!                        " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
