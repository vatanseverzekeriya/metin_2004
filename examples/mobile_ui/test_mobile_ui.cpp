#include "../../include/mobile/TouchInput.h"
#include "../../include/mobile/VirtualJoystick.h"
#include "../../include/mobile/UIButton.h"
#include "../../include/mobile/GameControls.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * Metin2 PvP Mobile UI Test
 * Dokunmatik kontrolleri test eder
 */

void PrintJoystickState(const JoystickState& state)
{
    if (state.active)
    {
        std::cout << "Joystick: x=" << state.x << " y=" << state.y
                  << " magnitude=" << state.magnitude
                  << " dir=" << (int)state.Get8Direction() << std::endl;
    }
}

void SimulateMovement()
{
    std::cout << "\n=== Simulating Movement ===" << std::endl;

    CVirtualJoystick joystick;
    joystick.Initialize(200, 800, 100);

    joystick.SetCallback([](const JoystickState& state) {
        PrintJoystickState(state);
    });

    // Touch down
    TouchPoint touch1(1, 200, 800, TOUCH_DOWN);
    joystick.OnTouchDown(touch1);

    // Sağa hareket
    for (int i = 0; i < 5; i++)
    {
        TouchPoint touchMove(1, 200 + i * 20, 800, TOUCH_MOVE);
        joystick.OnTouchMove(touchMove);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Touch up
    TouchPoint touchUp(1, 300, 800, TOUCH_UP);
    joystick.OnTouchUp(touchUp);

    std::cout << "Movement simulation complete\n" << std::endl;
}

void SimulateAttack()
{
    std::cout << "\n=== Simulating Attack ===" << std::endl;

    CUIButton attackBtn;
    attackBtn.Initialize(1700, 900, 120, 120);
    attackBtn.SetText("ATTACK");

    attackBtn.SetClickCallback([]() {
        std::cout << "ATTACK TRIGGERED!" << std::endl;
    });

    // Touch attack button
    TouchPoint touch(2, 1760, 960, TOUCH_DOWN);
    attackBtn.OnTouchDown(touch);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TouchPoint touchUp(2, 1760, 960, TOUCH_UP);
    attackBtn.OnTouchUp(touchUp);

    std::cout << "Attack simulation complete\n" << std::endl;
}

void SimulateSkillUsage()
{
    std::cout << "\n=== Simulating Skill Usage ===" << std::endl;

    CUIButton skillBtn;
    skillBtn.Initialize(900, 1000, 80, 80);
    skillBtn.SetText("SKILL 1");
    skillBtn.SetType(BUTTON_COOLDOWN);

    int skillUseCount = 0;
    skillBtn.SetClickCallback([&skillUseCount, &skillBtn]() {
        skillUseCount++;
        std::cout << "SKILL USED! (Count: " << skillUseCount << ")" << std::endl;

        // Cooldown başlat
        skillBtn.SetCooldown(5.0f);
    });

    // İlk kullanım
    TouchPoint touch1(3, 940, 1040, TOUCH_DOWN);
    skillBtn.OnTouchDown(touch1);

    TouchPoint touchUp1(3, 940, 1040, TOUCH_UP);
    skillBtn.OnTouchUp(touchUp1);

    std::cout << "Cooldown: " << (skillBtn.GetCooldownPercent() * 100) << "%" << std::endl;

    // Cooldown'daiken kullanmaya çalış
    std::cout << "Trying to use skill on cooldown..." << std::endl;
    TouchPoint touch2(4, 940, 1040, TOUCH_DOWN);
    skillBtn.OnTouchDown(touch2);

    TouchPoint touchUp2(4, 940, 1040, TOUCH_UP);
    skillBtn.OnTouchUp(touchUp2);

    // Cooldown simüle et
    for (int i = 0; i < 10; i++)
    {
        skillBtn.Update(500); // 500ms step
        if (skillBtn.IsOnCooldown())
        {
            std::cout << "Cooldown: " << (int)(skillBtn.GetCooldownPercent() * 100) << "%" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Skill simulation complete\n" << std::endl;
}

void SimulateGestures()
{
    std::cout << "\n=== Simulating Gestures ===" << std::endl;

    CTouchInput::Instance().Initialize(1920, 1080);

    // Gesture callback
    CTouchInput::Instance().RegisterGestureCallback([](const GestureInfo& gesture) {
        const char* types[] = {"TAP", "DOUBLE_TAP", "LONG_PRESS", "SWIPE", "PINCH", "DRAG"};
        std::cout << "[Gesture] " << types[gesture.type];

        if (gesture.type == GESTURE_SWIPE)
        {
            const char* dirs[] = {"NONE", "LEFT", "RIGHT", "UP", "DOWN"};
            std::cout << " Direction: " << dirs[gesture.swipe_dir];
        }

        std::cout << std::endl;
    });

    // Tap
    std::cout << "Simulating TAP..." << std::endl;
    CTouchInput::Instance().OnTouchDown(5, 500, 500);
    CTouchInput::Instance().OnTouchUp(5, 505, 505);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Swipe
    std::cout << "Simulating SWIPE..." << std::endl;
    CTouchInput::Instance().OnTouchDown(6, 300, 500);
    CTouchInput::Instance().OnTouchMove(6, 400, 500);
    CTouchInput::Instance().OnTouchMove(6, 500, 500);
    CTouchInput::Instance().OnTouchMove(6, 600, 500);
    CTouchInput::Instance().OnTouchUp(6, 700, 500);

    std::cout << "Gesture simulation complete\n" << std::endl;
}

void SimulateFullGameSession()
{
    std::cout << "\n=== Full Game Session Simulation ===" << std::endl;

    // Game controls başlat
    CGameControls::Instance().Initialize(1920, 1080);

    // Callbacks
    CGameControls::Instance().SetOnMoveCallback([](float x, float y) {
        std::cout << "[Move] dx=" << x << " dy=" << y << std::endl;
    });

    CGameControls::Instance().SetOnAttackCallback([]() {
        std::cout << "[Action] ATTACK!" << std::endl;
    });

    CGameControls::Instance().SetOnSkillCallback([](int skill_index) {
        std::cout << "[Action] SKILL " << skill_index << " used!" << std::endl;
    });

    // Quick slot ayarla
    CGameControls::Instance().SetQuickSlot(0, 1001, 10); // Skill ID 1001
    CGameControls::Instance().SetQuickSlot(1, 1002, 11); // Skill ID 1002

    std::cout << "\n--- Player Movement ---" << std::endl;

    // Hareket simülasyonu
    TouchPoint move1(10, 200, 800, TOUCH_DOWN);
    CGameControls::Instance().OnTouchDown(move1);

    TouchPoint move2(10, 250, 750, TOUCH_MOVE);
    CGameControls::Instance().OnTouchMove(move2);

    TouchPoint move3(10, 250, 750, TOUCH_UP);
    CGameControls::Instance().OnTouchUp(move3);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << "\n--- Attack Action ---" << std::endl;

    // Saldırı simülasyonu
    TouchPoint attack1(11, 1700, 900, TOUCH_DOWN);
    CGameControls::Instance().OnTouchDown(attack1);

    TouchPoint attack2(11, 1700, 900, TOUCH_UP);
    CGameControls::Instance().OnTouchUp(attack2);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << "\n--- Skill Usage ---" << std::endl;

    // Skill kullanımı
    TouchPoint skill1(12, 900, 1000, TOUCH_DOWN);
    CGameControls::Instance().OnTouchDown(skill1);

    TouchPoint skill2(12, 900, 1000, TOUCH_UP);
    CGameControls::Instance().OnTouchUp(skill2);

    std::cout << "\nFull game session simulation complete!" << std::endl;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  Metin2 PvP Mobile UI Test Suite      " << std::endl;
    std::cout << "========================================" << std::endl;

    // Test menüsü
    while (true)
    {
        std::cout << "\n========== TEST MENU ==========" << std::endl;
        std::cout << "1. Simulate Movement (Joystick)" << std::endl;
        std::cout << "2. Simulate Attack Button" << std::endl;
        std::cout << "3. Simulate Skill Usage (with Cooldown)" << std::endl;
        std::cout << "4. Simulate Gestures (Tap, Swipe, etc.)" << std::endl;
        std::cout << "5. Simulate Full Game Session" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "===============================" << std::endl;
        std::cout << "Choice: ";

        int choice;
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            SimulateMovement();
            break;
        case 2:
            SimulateAttack();
            break;
        case 3:
            SimulateSkillUsage();
            break;
        case 4:
            SimulateGestures();
            break;
        case 5:
            SimulateFullGameSession();
            break;
        case 0:
            std::cout << "\nGoodbye!" << std::endl;
            return 0;
        default:
            std::cout << "Invalid choice!" << std::endl;
        }
    }

    return 0;
}
