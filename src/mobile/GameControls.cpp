#include "../../include/mobile/GameControls.h"
#include <iostream>

CGameControls::CGameControls()
    : m_fScreenWidth(1920.0f)
    , m_fScreenHeight(1080.0f)
    , m_bAutoAttack(false)
    , m_dwAutoAttackInterval(1000)
    , m_dwLastAutoAttack(0)
    , m_iCurrentLayout(0)
{
}

CGameControls::~CGameControls()
{
}

CGameControls& CGameControls::Instance()
{
    static CGameControls instance;
    return instance;
}

void CGameControls::Initialize(float screen_width, float screen_height)
{
    m_fScreenWidth = screen_width;
    m_fScreenHeight = screen_height;

    CreateDefaultLayout();

    std::cout << "GameControls initialized: " << screen_width << "x" << screen_height << std::endl;
}

void CGameControls::CreateDefaultLayout()
{
    // Movement joystick (sol alt köşe)
    m_pJoystick = std::make_unique<CVirtualJoystick>();
    m_pJoystick->Initialize(200.0f, m_fScreenHeight - 200.0f, 120.0f);
    m_pJoystick->SetFloating(false);
    m_pJoystick->SetCallback([this](const JoystickState& state) {
        if (m_onMove && state.active)
        {
            float dx, dy;
            state.GetDirection(dx, dy);
            m_onMove(dx, dy);
        }
    });

    // Attack button (sağ alt)
    m_pButtonAttack = std::make_unique<CUIButton>();
    m_pButtonAttack->Initialize(m_fScreenWidth - 200, m_fScreenHeight - 200, 120, 120);
    m_pButtonAttack->SetText("ATK");
    m_pButtonAttack->SetClickSound("attack.wav");
    m_pButtonAttack->SetClickCallback([this]() {
        if (m_onAttack)
            m_onAttack();
    });

    // Jump button
    m_pButtonJump = std::make_unique<CUIButton>();
    m_pButtonJump->Initialize(m_fScreenWidth - 350, m_fScreenHeight - 150, 100, 100);
    m_pButtonJump->SetText("JUMP");

    // Pickup button
    m_pButtonPickup = std::make_unique<CUIButton>();
    m_pButtonPickup->Initialize(m_fScreenWidth - 350, m_fScreenHeight - 300, 100, 100);
    m_pButtonPickup->SetText("PICK");

    // Skill buttons (quick bar - ekranın altında ortada)
    float skill_size = 80.0f;
    float skill_spacing = 90.0f;
    float start_x = (m_fScreenWidth - skill_spacing * 5) / 2.0f;
    float skill_y = m_fScreenHeight - 100;

    for (int i = 0; i < 6; i++)
    {
        auto btn = std::make_unique<CUIButton>();
        btn->Initialize(start_x + i * skill_spacing, skill_y, skill_size, skill_size);
        btn->SetText(std::to_string(i + 1));
        btn->SetType(BUTTON_COOLDOWN);

        int skill_index = i;
        btn->SetClickCallback([this, skill_index]() {
            if (m_onSkill)
                m_onSkill(skill_index);
            UseQuickSlot(skill_index);
        });

        m_vecSkillButtons.push_back(std::move(btn));
        m_vecSkillIDs.push_back(0);
    }

    // Menu buttons (üst sağ)
    float menu_size = 60.0f;
    float menu_y = 20.0f;

    m_pButtonMenu = std::make_unique<CUIButton>();
    m_pButtonMenu->Initialize(m_fScreenWidth - menu_size - 20, menu_y, menu_size, menu_size);
    m_pButtonMenu->SetText("☰");

    m_pButtonInventory = std::make_unique<CUIButton>();
    m_pButtonInventory->Initialize(m_fScreenWidth - menu_size * 2 - 30, menu_y, menu_size, menu_size);
    m_pButtonInventory->SetText("BAG");

    m_pButtonCharacter = std::make_unique<CUIButton>();
    m_pButtonCharacter->Initialize(m_fScreenWidth - menu_size * 3 - 40, menu_y, menu_size, menu_size);
    m_pButtonCharacter->SetText("CHAR");

    m_pButtonQuest = std::make_unique<CUIButton>();
    m_pButtonQuest->Initialize(m_fScreenWidth - menu_size * 4 - 50, menu_y, menu_size, menu_size);
    m_pButtonQuest->SetText("QUEST");

    m_pButtonGuild = std::make_unique<CUIButton>();
    m_pButtonGuild->Initialize(m_fScreenWidth - menu_size * 5 - 60, menu_y, menu_size, menu_size);
    m_pButtonGuild->SetText("GUILD");

    // Settings button (üst sol)
    m_pButtonSettings = std::make_unique<CUIButton>();
    m_pButtonSettings->Initialize(20, 20, menu_size, menu_size);
    m_pButtonSettings->SetText("⚙");

    std::cout << "Default layout created" << std::endl;
}

void CGameControls::CreateLeftHandedLayout()
{
    // Joystick sağda, attack butonları solda
    m_pJoystick->SetPosition(m_fScreenWidth - 200, m_fScreenHeight - 200);

    m_pButtonAttack->SetPosition(200, m_fScreenHeight - 200);
    m_pButtonJump->SetPosition(350, m_fScreenHeight - 150);
    m_pButtonPickup->SetPosition(350, m_fScreenHeight - 300);

    std::cout << "Left-handed layout created" << std::endl;
}

void CGameControls::Update(DWORD delta_time)
{
    // Joystick güncelle
    if (m_pJoystick)
        m_pJoystick->Update(delta_time);

    // Buttons güncelle
    if (m_pButtonAttack)
        m_pButtonAttack->Update(delta_time);
    if (m_pButtonJump)
        m_pButtonJump->Update(delta_time);
    if (m_pButtonPickup)
        m_pButtonPickup->Update(delta_time);

    // Skill buttons
    for (auto& btn : m_vecSkillButtons)
    {
        if (btn)
            btn->Update(delta_time);
    }

    // Menu buttons
    if (m_pButtonMenu) m_pButtonMenu->Update(delta_time);
    if (m_pButtonInventory) m_pButtonInventory->Update(delta_time);
    if (m_pButtonCharacter) m_pButtonCharacter->Update(delta_time);
    if (m_pButtonQuest) m_pButtonQuest->Update(delta_time);
    if (m_pButtonGuild) m_pButtonGuild->Update(delta_time);
    if (m_pButtonSettings) m_pButtonSettings->Update(delta_time);

    // Auto attack
    UpdateAutoAttack(delta_time);
}

void CGameControls::Render()
{
    // Joystick çiz
    if (m_pJoystick)
        m_pJoystick->Render();

    // Buttons çiz
    if (m_pButtonAttack) m_pButtonAttack->Render();
    if (m_pButtonJump) m_pButtonJump->Render();
    if (m_pButtonPickup) m_pButtonPickup->Render();

    // Skill buttons
    for (auto& btn : m_vecSkillButtons)
    {
        if (btn)
            btn->Render();
    }

    // Menu buttons
    if (m_pButtonMenu) m_pButtonMenu->Render();
    if (m_pButtonInventory) m_pButtonInventory->Render();
    if (m_pButtonCharacter) m_pButtonCharacter->Render();
    if (m_pButtonQuest) m_pButtonQuest->Render();
    if (m_pButtonGuild) m_pButtonGuild->Render();
    if (m_pButtonSettings) m_pButtonSettings->Render();
}

void CGameControls::OnTouchDown(const TouchPoint& touch)
{
    // Önce joystick kontrol et
    if (m_pJoystick)
        m_pJoystick->OnTouchDown(touch);

    // Buttonları kontrol et
    if (m_pButtonAttack && m_pButtonAttack->OnTouchDown(touch)) return;
    if (m_pButtonJump && m_pButtonJump->OnTouchDown(touch)) return;
    if (m_pButtonPickup && m_pButtonPickup->OnTouchDown(touch)) return;

    // Skill buttons
    for (auto& btn : m_vecSkillButtons)
    {
        if (btn && btn->OnTouchDown(touch))
            return;
    }

    // Menu buttons
    if (m_pButtonMenu && m_pButtonMenu->OnTouchDown(touch)) return;
    if (m_pButtonInventory && m_pButtonInventory->OnTouchDown(touch)) return;
    if (m_pButtonCharacter && m_pButtonCharacter->OnTouchDown(touch)) return;
    if (m_pButtonQuest && m_pButtonQuest->OnTouchDown(touch)) return;
    if (m_pButtonGuild && m_pButtonGuild->OnTouchDown(touch)) return;
    if (m_pButtonSettings && m_pButtonSettings->OnTouchDown(touch)) return;
}

void CGameControls::OnTouchMove(const TouchPoint& touch)
{
    if (m_pJoystick)
        m_pJoystick->OnTouchMove(touch);

    if (m_pButtonAttack) m_pButtonAttack->OnTouchMove(touch);
    if (m_pButtonJump) m_pButtonJump->OnTouchMove(touch);
    if (m_pButtonPickup) m_pButtonPickup->OnTouchMove(touch);

    for (auto& btn : m_vecSkillButtons)
    {
        if (btn)
            btn->OnTouchMove(touch);
    }
}

void CGameControls::OnTouchUp(const TouchPoint& touch)
{
    if (m_pJoystick)
        m_pJoystick->OnTouchUp(touch);

    if (m_pButtonAttack) m_pButtonAttack->OnTouchUp(touch);
    if (m_pButtonJump) m_pButtonJump->OnTouchUp(touch);
    if (m_pButtonPickup) m_pButtonPickup->OnTouchUp(touch);

    for (auto& btn : m_vecSkillButtons)
    {
        if (btn)
            btn->OnTouchUp(touch);
    }

    if (m_pButtonMenu) m_pButtonMenu->OnTouchUp(touch);
    if (m_pButtonInventory) m_pButtonInventory->OnTouchUp(touch);
    if (m_pButtonCharacter) m_pButtonCharacter->OnTouchUp(touch);
    if (m_pButtonQuest) m_pButtonQuest->OnTouchUp(touch);
    if (m_pButtonGuild) m_pButtonGuild->OnTouchUp(touch);
    if (m_pButtonSettings) m_pButtonSettings->OnTouchUp(touch);
}

CUIButton* CGameControls::GetSkillButton(int index)
{
    if (index >= 0 && index < (int)m_vecSkillButtons.size())
        return m_vecSkillButtons[index].get();
    return nullptr;
}

void CGameControls::SetQuickSlot(int index, DWORD skill_id, DWORD icon_id)
{
    if (index >= 0 && index < (int)m_vecSkillIDs.size())
    {
        m_vecSkillIDs[index] = skill_id;

        if (auto btn = GetSkillButton(index))
        {
            btn->SetIcon(icon_id);
        }
    }
}

void CGameControls::UseQuickSlot(int index)
{
    if (index >= 0 && index < (int)m_vecSkillIDs.size())
    {
        DWORD skill_id = m_vecSkillIDs[index];
        if (skill_id > 0)
        {
            std::cout << "[QuickSlot] Using skill " << skill_id << std::endl;

            // Cooldown başlat
            if (auto btn = GetSkillButton(index))
            {
                btn->SetCooldown(5.0f); // 5 saniye cooldown
            }
        }
    }
}

void CGameControls::SetLayout(int layout_id)
{
    m_iCurrentLayout = layout_id;

    switch (layout_id)
    {
    case 0:
        CreateDefaultLayout();
        break;
    case 1:
        CreateLeftHandedLayout();
        break;
    default:
        break;
    }

    std::cout << "[Layout] Changed to " << layout_id << std::endl;
}

void CGameControls::ShowHUD(bool show)
{
    if (m_pButtonMenu) m_pButtonMenu->SetVisible(show);
    if (m_pButtonInventory) m_pButtonInventory->SetVisible(show);
    if (m_pButtonCharacter) m_pButtonCharacter->SetVisible(show);
    if (m_pButtonQuest) m_pButtonQuest->SetVisible(show);
    if (m_pButtonGuild) m_pButtonGuild->SetVisible(show);
}

void CGameControls::ShowSkillBar(bool show)
{
    for (auto& btn : m_vecSkillButtons)
    {
        if (btn)
            btn->SetVisible(show);
    }
}

void CGameControls::ShowMenu(bool show)
{
    // Menu UI gösterme/gizleme
}

void CGameControls::SetAutoAttack(bool enabled)
{
    m_bAutoAttack = enabled;
    std::cout << "[AutoAttack] " << (enabled ? "Enabled" : "Disabled") << std::endl;
}

void CGameControls::UpdateAutoAttack(DWORD delta_time)
{
    if (!m_bAutoAttack)
        return;

    m_dwLastAutoAttack += delta_time;

    if (m_dwLastAutoAttack >= m_dwAutoAttackInterval)
    {
        m_dwLastAutoAttack = 0;

        // Auto attack tetikle
        if (m_onAttack)
            m_onAttack();
    }
}

void CGameControls::SaveLayout()
{
    // Layout'u dosyaya kaydet
    std::cout << "[Layout] Saved" << std::endl;
}

void CGameControls::LoadLayout()
{
    // Layout'u dosyadan yükle
    std::cout << "[Layout] Loaded" << std::endl;
}
