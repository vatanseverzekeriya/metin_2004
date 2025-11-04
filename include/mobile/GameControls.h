#ifndef __INC_MOBILE_GAME_CONTROLS_H__
#define __INC_MOBILE_GAME_CONTROLS_H__

#include "../common/types.h"
#include "TouchInput.h"
#include "VirtualJoystick.h"
#include "UIButton.h"
#include <vector>
#include <memory>

// Metin2 özel kontrol layoutu
class CGameControls
{
public:
    static CGameControls& Instance();

    // Başlatma
    void Initialize(float screen_width, float screen_height);
    void Update(DWORD delta_time);
    void Render();

    // Touch input integration
    void OnTouchDown(const TouchPoint& touch);
    void OnTouchMove(const TouchPoint& touch);
    void OnTouchUp(const TouchPoint& touch);

    // Joystick
    CVirtualJoystick* GetMovementJoystick() { return m_pJoystick.get(); }
    const JoystickState& GetMovementState() const { return m_pJoystick->GetState(); }

    // Buttons
    CUIButton* GetAttackButton() { return m_pButtonAttack.get(); }
    CUIButton* GetSkillButton(int index);
    CUIButton* GetMenuButton() { return m_pButtonMenu.get(); }
    CUIButton* GetInventoryButton() { return m_pButtonInventory.get(); }

    // Quick slots
    void SetQuickSlot(int index, DWORD skill_id, DWORD icon_id);
    void UseQuickSlot(int index);

    // Callbacks
    void SetOnMoveCallback(std::function<void(float x, float y)> callback) { m_onMove = callback; }
    void SetOnAttackCallback(std::function<void()> callback) { m_onAttack = callback; }
    void SetOnSkillCallback(std::function<void(int skill_index)> callback) { m_onSkill = callback; }

    // Layout control
    void SetLayout(int layout_id); // 0=default, 1=left-handed, 2=custom
    void SaveLayout();
    void LoadLayout();

    // Görünürlük
    void ShowHUD(bool show);
    void ShowSkillBar(bool show);
    void ShowMenu(bool show);

    // Auto-attack toggle
    void SetAutoAttack(bool enabled);
    bool IsAutoAttack() const { return m_bAutoAttack; }

private:
    CGameControls();
    ~CGameControls();

    CGameControls(const CGameControls&) = delete;
    CGameControls& operator=(const CGameControls&) = delete;

    void CreateDefaultLayout();
    void CreateLeftHandedLayout();
    void UpdateAutoAttack(DWORD delta_time);

    // Screen
    float m_fScreenWidth;
    float m_fScreenHeight;

    // Movement joystick
    std::unique_ptr<CVirtualJoystick> m_pJoystick;

    // Action buttons
    std::unique_ptr<CUIButton> m_pButtonAttack;
    std::unique_ptr<CUIButton> m_pButtonJump;
    std::unique_ptr<CUIButton> m_pButtonPickup;

    // Skill buttons (quick bar)
    std::vector<std::unique_ptr<CUIButton>> m_vecSkillButtons;
    std::vector<DWORD> m_vecSkillIDs;

    // Menu buttons
    std::unique_ptr<CUIButton> m_pButtonMenu;
    std::unique_ptr<CUIButton> m_pButtonInventory;
    std::unique_ptr<CUIButton> m_pButtonCharacter;
    std::unique_ptr<CUIButton> m_pButtonQuest;
    std::unique_ptr<CUIButton> m_pButtonGuild;

    // Settings button
    std::unique_ptr<CUIButton> m_pButtonSettings;

    // Auto attack
    bool m_bAutoAttack;
    DWORD m_dwAutoAttackInterval;
    DWORD m_dwLastAutoAttack;

    // Callbacks
    std::function<void(float x, float y)> m_onMove;
    std::function<void()> m_onAttack;
    std::function<void(int)> m_onSkill;

    // Layout
    int m_iCurrentLayout;
};

#endif // __INC_MOBILE_GAME_CONTROLS_H__
