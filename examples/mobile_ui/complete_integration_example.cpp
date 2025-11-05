/**
 * Metin2 PvP - Tam Mobil Entegrasyon Örneği
 *
 * Bu dosya, Metin2 oyununun mobil versiyonu için
 * tüm dokunmatik kontrollerin nasıl entegre edileceğini gösterir.
 *
 * Özellikler:
 * - Oyuncu hareketi (virtual joystick)
 * - Saldırı ve beceri kullanımı
 * - Menü navigasyonu (inventory, character, etc.)
 * - Sunucu ile iletişim
 * - Target seçimi
 * - Auto-attack
 */

#include "../../include/mobile/TouchInput.h"
#include "../../include/mobile/VirtualJoystick.h"
#include "../../include/mobile/UIButton.h"
#include "../../include/mobile/GameControls.h"
#include "../../include/mobile/MenuNavigation.h"
#include "../../include/game/Character.h"
#include "../../include/network/NetworkManager.h"
#include <iostream>
#include <memory>

// Global nesneler (gerçek uygulamada singleton veya dependency injection kullanın)
std::unique_ptr<CCharacter> g_pLocalPlayer;
std::unique_ptr<CCharacter> g_pTargetCharacter;

/**
 * 1. OYUN BAŞLATMA
 */
void InitializeMobileGame()
{
    std::cout << "=== Metin2 Mobile Game Initialization ===" << std::endl;

    // Touch input sistemi
    CTouchInput::Instance().Initialize(1920, 1080);

    // Oyun kontrolleri
    CGameControls::Instance().Initialize(1920, 1080);

    // Menü navigasyonu
    CMenuNavigation::Instance().Initialize(1920, 1080);
    CMenuNavigation::Instance().SetInventorySize(5, 8); // 5x8 inventory

    // Oyuncu karakteri oluştur
    g_pLocalPlayer = std::make_unique<CCharacter>(1, "TestPlayer");
    g_pLocalPlayer->SetPosition(1000, 1000);
    g_pLocalPlayer->SetLevel(50);
    g_pLocalPlayer->SetHP(1000, 1000);

    std::cout << "Mobile game initialized successfully!" << std::endl;
}

/**
 * 2. OYUNCU HAREKETİ - Joystick ile karakter kontrolü
 */
void SetupMovementControls()
{
    std::cout << "\n=== Setting up Movement Controls ===" << std::endl;

    // Movement callback
    CGameControls::Instance().SetOnMoveCallback([](float dx, float dy) {
        if (!g_pLocalPlayer)
            return;

        // Karakterin mevcut pozisyonunu al
        float x = g_pLocalPlayer->GetX();
        float y = g_pLocalPlayer->GetY();

        // Hareket hızı
        float speed = 300.0f; // piksel/saniye

        // Yeni pozisyon hesapla (delta time ile çarp)
        float delta_time = 0.016f; // ~60 FPS
        x += dx * speed * delta_time;
        y += dy * speed * delta_time;

        // Pozisyonu güncelle
        g_pLocalPlayer->SetPosition(x, y);

        std::cout << "[Movement] Player moved to (" << x << ", " << y << ")" << std::endl;

        // Sunucuya gönder
        // TPacketCGMove packet;
        // packet.x = x;
        // packet.y = y;
        // CNetworkManager::Instance().SendPacket(HEADER_CG_MOVE, &packet, sizeof(packet));
    });

    // Joystick özelleştirme
    auto* joystick = CGameControls::Instance().GetMovementJoystick();
    if (joystick)
    {
        joystick->SetDeadZone(0.15f);  // %15 dead zone
        joystick->SetAutoRecenter(true);
        joystick->SetFloating(false);  // Sabit pozisyon

        std::cout << "[Setup] Joystick configured: dead_zone=0.15, floating=false" << std::endl;
    }
}

/**
 * 3. SALDIRI SİSTEMİ - Normal saldırı ve auto-attack
 */
void SetupAttackControls()
{
    std::cout << "\n=== Setting up Attack Controls ===" << std::endl;

    CGameControls::Instance().SetOnAttackCallback([]() {
        if (!g_pLocalPlayer)
            return;

        // Target var mı kontrol et
        if (!g_pTargetCharacter)
        {
            std::cout << "[Attack] No target selected!" << std::endl;
            return;
        }

        // Mesafe kontrolü
        float distance = g_pLocalPlayer->GetDistanceTo(g_pTargetCharacter.get());
        float attack_range = 200.0f; // Metin2'de yakın dövüş menzili

        if (distance > attack_range)
        {
            std::cout << "[Attack] Target too far! Distance: " << distance << std::endl;
            return;
        }

        // Saldırı yap
        std::cout << "[Attack] Attacking target ID: " << g_pTargetCharacter->GetID() << std::endl;
        g_pLocalPlayer->Attack(g_pTargetCharacter.get());

        // Sunucuya saldırı paketi gönder
        // TPacketCGAttack packet;
        // packet.target_id = g_pTargetCharacter->GetID();
        // CNetworkManager::Instance().SendPacket(HEADER_CG_ATTACK, &packet, sizeof(packet));
    });

    // Attack button özelleştirme
    auto* attackBtn = CGameControls::Instance().GetAttackButton();
    if (attackBtn)
    {
        attackBtn->SetClickSound("attack_click.wav");
        attackBtn->SetColor(0xFFFF0000, 0xFFCC0000, 0xFF880000); // Kırmızı tonları

        std::cout << "[Setup] Attack button configured" << std::endl;
    }

    // Auto-attack (PvP için kullanışlı)
    CGameControls::Instance().SetAutoAttack(true);
    std::cout << "[Setup] Auto-attack enabled" << std::endl;
}

/**
 * 4. BECERİ SİSTEMİ - Skill kullanımı ve cooldown yönetimi
 */
void SetupSkillControls()
{
    std::cout << "\n=== Setting up Skill Controls ===" << std::endl;

    CGameControls::Instance().SetOnSkillCallback([](int skill_index) {
        if (!g_pLocalPlayer)
            return;

        // Skill bilgilerini al (bu örnekte hardcoded)
        struct SkillInfo {
            DWORD skill_id;
            const char* name;
            float cooldown;
            int mp_cost;
        };

        SkillInfo skills[] = {
            {1001, "Fireball", 5.0f, 50},
            {1002, "Ice Bolt", 3.0f, 30},
            {1003, "Lightning", 8.0f, 80},
            {1004, "Heal", 10.0f, 100},
            {1005, "Teleport", 15.0f, 60},
            {1006, "Shield", 12.0f, 40}
        };

        if (skill_index < 0 || skill_index >= 6)
            return;

        const SkillInfo& skill = skills[skill_index];

        // MP kontrolü
        if (g_pLocalPlayer->GetMP() < skill.mp_cost)
        {
            std::cout << "[Skill] Not enough MP! Required: " << skill.mp_cost << std::endl;
            return;
        }

        // Target gerekli mi? (heal dışında target gerekli)
        if (skill_index != 3 && !g_pTargetCharacter)
        {
            std::cout << "[Skill] No target selected!" << std::endl;
            return;
        }

        // Skill kullan
        std::cout << "[Skill] Using " << skill.name << " (ID: " << skill.skill_id << ")" << std::endl;
        g_pLocalPlayer->SetMP(g_pLocalPlayer->GetMP() - skill.mp_cost);

        // Cooldown başlat
        auto* skillBtn = CGameControls::Instance().GetSkillButton(skill_index);
        if (skillBtn)
        {
            skillBtn->SetCooldown(skill.cooldown);
        }

        // Sunucuya skill paketi gönder
        // TPacketCGSkill packet;
        // packet.skill_id = skill.skill_id;
        // packet.target_id = g_pTargetCharacter ? g_pTargetCharacter->GetID() : 0;
        // CNetworkManager::Instance().SendPacket(HEADER_CG_SKILL, &packet, sizeof(packet));
    });

    // Skill slot'larını ayarla
    CGameControls::Instance().SetQuickSlot(0, 1001, 101); // Fireball
    CGameControls::Instance().SetQuickSlot(1, 1002, 102); // Ice Bolt
    CGameControls::Instance().SetQuickSlot(2, 1003, 103); // Lightning
    CGameControls::Instance().SetQuickSlot(3, 1004, 104); // Heal
    CGameControls::Instance().SetQuickSlot(4, 1005, 105); // Teleport
    CGameControls::Instance().SetQuickSlot(5, 1006, 106); // Shield

    std::cout << "[Setup] 6 skills configured in quick slots" << std::endl;
}

/**
 * 5. MENÜ NAVİGASYONU - Inventory, Character, Quest
 */
void SetupMenuNavigation()
{
    std::cout << "\n=== Setting up Menu Navigation ===" << std::endl;

    // Inventory butonu
    auto* invBtn = CGameControls::Instance().GetInventoryButton();
    if (invBtn)
    {
        invBtn->SetClickCallback([]() {
            std::cout << "[Menu] Opening inventory" << std::endl;
            CMenuNavigation::Instance().OpenMenu(MENU_INVENTORY);
            CGameControls::Instance().ShowHUD(false); // HUD'u gizle
        });
    }

    // Character butonu (örnek - yeni buton ekleyebilirsiniz)
    // ...

    // Inventory item click callback
    CMenuNavigation::Instance().SetSlotClickCallback([](int slot_index) {
        std::cout << "[Inventory] Slot " << slot_index << " clicked" << std::endl;
    });

    // Drag-drop callback (item taşıma)
    CMenuNavigation::Instance().SetDragDropCallback([](int from_slot, int to_slot) {
        std::cout << "[Inventory] Item moved from slot " << from_slot << " to " << to_slot << std::endl;

        // Sunucuya item move paketi gönder
        // TPacketCGItemMove packet;
        // packet.from_slot = from_slot;
        // packet.to_slot = to_slot;
        // CNetworkManager::Instance().SendPacket(HEADER_CG_ITEM_MOVE, &packet, sizeof(packet));
    });

    // Double-tap ile item kullanımı
    CMenuNavigation::Instance().SetItemUseCallback([](int slot_index, DWORD item_id) {
        std::cout << "[Inventory] Using item " << item_id << " from slot " << slot_index << std::endl;

        // Sunucuya item use paketi gönder
        // TPacketCGItemUse packet;
        // packet.slot = slot_index;
        // packet.item_id = item_id;
        // CNetworkManager::Instance().SendPacket(HEADER_CG_ITEM_USE, &packet, sizeof(packet));
    });

    // Test inventory item'leri ekle
    CMenuNavigation::Instance().SetInventoryItem(0, 10001, 201, 1);   // Sword
    CMenuNavigation::Instance().SetInventoryItem(1, 10002, 202, 1);   // Shield
    CMenuNavigation::Instance().SetInventoryItem(2, 10003, 203, 99);  // HP Potion x99
    CMenuNavigation::Instance().SetInventoryItem(3, 10004, 204, 50);  // MP Potion x50

    std::cout << "[Setup] Menu navigation configured with 4 test items" << std::endl;
}

/**
 * 6. TARGET SELECTION - Düşman seçimi (gesture ile)
 */
void SetupTargetSelection()
{
    std::cout << "\n=== Setting up Target Selection ===" << std::endl;

    // Tap gesture ile target seçimi
    CTouchInput::Instance().RegisterGestureCallback([](const GestureInfo& gesture) {
        if (gesture.type != GESTURE_TAP)
            return;

        // Menü açıksa tap'i işleme
        if (CMenuNavigation::Instance().IsMenuOpen())
            return;

        // UI butonu üzerinde mi kontrol et (burada basit bir implementasyon)
        // Gerçek uygulamada UI hit test yapılmalı

        float tap_x = gesture.current_x;
        float tap_y = gesture.current_y;

        std::cout << "[Target] Tap at (" << tap_x << ", " << tap_y << ")" << std::endl;

        // Oyun dünyasındaki karakterleri kontrol et
        // Örnek: Yakındaki düşmanları bul
        // Bu gerçek uygulamada spatial partitioning ile optimize edilmeli

        // Test için basit bir target oluştur
        if (!g_pTargetCharacter)
        {
            g_pTargetCharacter = std::make_unique<CCharacter>(2, "Enemy");
            g_pTargetCharacter->SetPosition(1200, 1200);
            g_pTargetCharacter->SetLevel(45);
            g_pTargetCharacter->SetHP(800, 800);

            std::cout << "[Target] Target selected: " << g_pTargetCharacter->GetName() << std::endl;
        }
        else
        {
            // Target'i kaldır
            g_pTargetCharacter.reset();
            std::cout << "[Target] Target deselected" << std::endl;
        }
    });

    std::cout << "[Setup] Target selection configured (tap to select)" << std::endl;
}

/**
 * 7. GESTURE CONTROLS - İleri seviye gesture'lar
 */
void SetupAdvancedGestures()
{
    std::cout << "\n=== Setting up Advanced Gestures ===" << std::endl;

    CTouchInput::Instance().RegisterGestureCallback([](const GestureInfo& gesture) {
        switch (gesture.type)
        {
        case GESTURE_DOUBLE_TAP:
            // Double tap ile pickup (item toplama)
            std::cout << "[Gesture] Double tap - Pick up item" << std::endl;
            // PickUpItemAtPosition(gesture.current_x, gesture.current_y);
            break;

        case GESTURE_LONG_PRESS:
            // Long press ile menü aç
            std::cout << "[Gesture] Long press - Context menu" << std::endl;
            // ShowContextMenu(gesture.current_x, gesture.current_y);
            break;

        case GESTURE_SWIPE:
            // Swipe ile kamera rotasyonu veya quick dodge
            if (gesture.swipe_dir == SWIPE_LEFT || gesture.swipe_dir == SWIPE_RIGHT)
            {
                std::cout << "[Gesture] Swipe - Camera rotation" << std::endl;
                // RotateCamera(gesture.swipe_dir == SWIPE_LEFT ? -1 : 1);
            }
            else if (gesture.swipe_dir == SWIPE_DOWN)
            {
                std::cout << "[Gesture] Swipe down - Close menu" << std::endl;
                CMenuNavigation::Instance().CloseAllMenus();
                CGameControls::Instance().ShowHUD(true);
            }
            break;

        case GESTURE_PINCH:
            // Pinch ile zoom
            std::cout << "[Gesture] Pinch - Zoom (scale: " << gesture.scale << ")" << std::endl;
            // SetCameraZoom(gesture.scale);
            break;

        default:
            break;
        }
    });

    std::cout << "[Setup] Advanced gestures configured" << std::endl;
}

/**
 * 8. LAYOUT ÖZELLEŞTİRME - Sol el / sağ el
 */
void DemoLayoutCustomization()
{
    std::cout << "\n=== Layout Customization Demo ===" << std::endl;

    // Default layout (sağ el)
    std::cout << "[Layout] Default layout (right-handed)" << std::endl;
    CGameControls::Instance().SetLayout(0);

    // Sol el layout'u
    std::cout << "[Layout] Switching to left-handed layout" << std::endl;
    CGameControls::Instance().SetLayout(1);

    // Layout'u kaydet (dosyaya)
    CGameControls::Instance().SaveLayout();
}

/**
 * 9. NETWORK ENTEGRASYONU - Sunucu ile paket alışverişi
 */
void SetupNetworkIntegration()
{
    std::cout << "\n=== Setting up Network Integration ===" << std::endl;

    // Not: Bu örnekte network paketleri gösterilmiştir
    // Gerçek implementasyon için NetworkManager kullanın

    // Örnek paket yapıları (protocol.h'den)
    /*
    struct TPacketCGMove {
        BYTE header;
        float x;
        float y;
        BYTE direction;
    };

    struct TPacketCGAttack {
        BYTE header;
        DWORD target_id;
    };

    struct TPacketCGSkill {
        BYTE header;
        DWORD skill_id;
        DWORD target_id;
    };

    struct TPacketCGItemMove {
        BYTE header;
        BYTE from_slot;
        BYTE to_slot;
    };
    */

    std::cout << "[Network] Packet handlers registered" << std::endl;
    std::cout << "[Network] Ready to send/receive packets" << std::endl;
}

/**
 * 10. GAME LOOP - Ana oyun döngüsü
 */
void GameLoop()
{
    std::cout << "\n=== Starting Game Loop ===" << std::endl;

    const int FPS = 60;
    const float FRAME_TIME = 1000.0f / FPS; // ms

    for (int frame = 0; frame < 10; frame++) // Demo için 10 frame
    {
        // Touch input güncelle
        CTouchInput::Instance().Update(FRAME_TIME);

        // Game controls güncelle
        CGameControls::Instance().Update(FRAME_TIME);

        // Menu navigation güncelle
        CMenuNavigation::Instance().Update(FRAME_TIME);

        // Oyun mantığını güncelle
        if (g_pLocalPlayer)
        {
            // Karakter state update
            // g_pLocalPlayer->Update(FRAME_TIME);
        }

        // Render
        // RenderGame();
        CGameControls::Instance().Render();
        CMenuNavigation::Instance().Render();

        // Simüle frame delay
        // std::this_thread::sleep_for(std::chrono::milliseconds((int)FRAME_TIME));
    }

    std::cout << "[GameLoop] 10 frames processed" << std::endl;
}

/**
 * MAIN - Tüm sistemi test et
 */
int main()
{
    std::cout << "==========================================" << std::endl;
    std::cout << "  Metin2 Mobile - Complete Integration   " << std::endl;
    std::cout << "==========================================" << std::endl;

    // 1. Oyunu başlat
    InitializeMobileGame();

    // 2. Kontrolleri ayarla
    SetupMovementControls();
    SetupAttackControls();
    SetupSkillControls();
    SetupMenuNavigation();
    SetupTargetSelection();
    SetupAdvancedGestures();

    // 3. Network entegrasyonu
    SetupNetworkIntegration();

    // 4. Layout customization demo
    DemoLayoutCustomization();

    // 5. Game loop başlat
    GameLoop();

    std::cout << "\n==========================================" << std::endl;
    std::cout << "  Integration Complete!                   " << std::endl;
    std::cout << "==========================================" << std::endl;

    return 0;
}
