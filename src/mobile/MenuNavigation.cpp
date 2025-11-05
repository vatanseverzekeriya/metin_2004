#include "../../include/mobile/MenuNavigation.h"
#include <cmath>
#include <iostream>
#include <algorithm>

CMenuNavigation::CMenuNavigation()
    : m_fScreenWidth(1920.0f)
    , m_fScreenHeight(1080.0f)
    , m_eActiveMenu(MENU_NONE)
    , m_iInventoryRows(5)
    , m_iInventoryCols(8)
    , m_bDragDropEnabled(true)
    , m_fDragThreshold(20.0f)
    , m_fScrollOffset(0.0f)
    , m_fScrollMin(0.0f)
    , m_fScrollMax(0.0f)
    , m_iLastTappedSlot(-1)
    , m_dwLastTapTime(0)
    , m_dwDoubleTapThreshold(300)
{
}

CMenuNavigation::~CMenuNavigation()
{
}

CMenuNavigation& CMenuNavigation::Instance()
{
    static CMenuNavigation instance;
    return instance;
}

void CMenuNavigation::Initialize(float screen_width, float screen_height)
{
    m_fScreenWidth = screen_width;
    m_fScreenHeight = screen_height;

    CreateInventoryLayout();

    std::cout << "MenuNavigation initialized: " << screen_width << "x" << screen_height << std::endl;
}

void CMenuNavigation::Update(DWORD delta_time)
{
    // Update quick access buttons
    for (auto& btn : m_vecQuickButtons)
    {
        if (btn)
            btn->Update(delta_time);
    }
}

void CMenuNavigation::Render()
{
    if (m_eActiveMenu == MENU_NONE)
        return;

    // Menü arka planı çiz
    // DrawMenuBackground(...)

    // Inventory slot'ları çiz
    if (m_eActiveMenu == MENU_INVENTORY)
    {
        for (const auto& slot : m_vecInventorySlots)
        {
            // Slot background
            // DrawRect(slot.x, slot.y - m_fScrollOffset, slot.width, slot.height, ...);

            // Item icon
            if (slot.item_id > 0)
            {
                // DrawTexture(slot.icon_id, slot.x, slot.y - m_fScrollOffset, ...);

                // Item count
                if (slot.count > 1)
                {
                    // DrawText(std::to_string(slot.count), slot.x, slot.y - m_fScrollOffset, ...);
                }
            }
        }
    }

    // Drag-drop preview
    if (m_dragDropState.active)
    {
        // Sürüklenen item'i mouse pozisyonunda göster
        const auto& slot = m_vecInventorySlots[m_dragDropState.source_slot];
        float preview_x = m_dragDropState.current_x - slot.width / 2;
        float preview_y = m_dragDropState.current_y - slot.height / 2;

        // DrawTexture(slot.icon_id, preview_x, preview_y, ...);
    }

    // Quick buttons
    for (auto& btn : m_vecQuickButtons)
    {
        if (btn)
            btn->Render();
    }
}

void CMenuNavigation::OnTouchDown(const TouchPoint& touch)
{
    if (m_eActiveMenu == MENU_NONE)
        return;

    // Quick buttons kontrol et
    for (auto& btn : m_vecQuickButtons)
    {
        if (btn && btn->OnTouchDown(touch))
            return;
    }

    // Inventory slot kontrolü
    if (m_eActiveMenu == MENU_INVENTORY)
    {
        int slot_index = GetSlotAtPosition(touch.x, touch.y);
        if (slot_index >= 0 && slot_index < (int)m_vecInventorySlots.size())
        {
            const auto& slot = m_vecInventorySlots[slot_index];
            if (slot.item_id > 0 && m_bDragDropEnabled)
            {
                // Drag başlatılabilir
                m_dragDropState.start_x = touch.x;
                m_dragDropState.start_y = touch.y;
                m_dragDropState.current_x = touch.x;
                m_dragDropState.current_y = touch.y;
                m_dragDropState.source_slot = slot_index;
                m_dragDropState.item_id = slot.item_id;
                m_dragDropState.touch_id = touch.id;
                // active = false (henüz drag başlamadı, threshold kontrolü yapılacak)

                std::cout << "[Menu] Touch down on slot " << slot_index << std::endl;
            }
        }
    }
}

void CMenuNavigation::OnTouchMove(const TouchPoint& touch)
{
    if (m_eActiveMenu == MENU_NONE)
        return;

    // Drag-drop kontrolü
    if (m_dragDropState.source_slot >= 0 && touch.id == m_dragDropState.touch_id)
    {
        m_dragDropState.current_x = touch.x;
        m_dragDropState.current_y = touch.y;

        // Threshold geçildiyse drag başlat
        if (!m_dragDropState.active)
        {
            float dx = touch.x - m_dragDropState.start_x;
            float dy = touch.y - m_dragDropState.start_y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance > m_fDragThreshold)
            {
                m_dragDropState.active = true;
                std::cout << "[Menu] Drag started from slot " << m_dragDropState.source_slot << std::endl;
            }
        }
    }
}

void CMenuNavigation::OnTouchUp(const TouchPoint& touch)
{
    if (m_eActiveMenu == MENU_NONE)
        return;

    // Quick buttons
    for (auto& btn : m_vecQuickButtons)
    {
        if (btn)
            btn->OnTouchUp(touch);
    }

    // Drag-drop işlemi
    if (m_dragDropState.active && touch.id == m_dragDropState.touch_id)
    {
        // Drop target slot bul
        int target_slot = GetSlotAtPosition(touch.x, touch.y);

        if (target_slot >= 0 && target_slot != m_dragDropState.source_slot)
        {
            // Drag-drop başarılı
            ProcessDragDrop(m_dragDropState.source_slot, target_slot);
        }

        // Drag state reset
        m_dragDropState.active = false;
        m_dragDropState.source_slot = -1;
        m_dragDropState.touch_id = 0;

        std::cout << "[Menu] Drag ended" << std::endl;
    }
    else if (m_dragDropState.source_slot >= 0 && touch.id == m_dragDropState.touch_id)
    {
        // Drag başlamadı, normal click
        int slot_index = GetSlotAtPosition(touch.x, touch.y);
        if (slot_index == m_dragDropState.source_slot)
        {
            ProcessSlotClick(slot_index);
            DetectDoubleTap(slot_index);
        }

        m_dragDropState.source_slot = -1;
        m_dragDropState.touch_id = 0;
    }
}

void CMenuNavigation::OpenMenu(EMenuType menu_type)
{
    m_eActiveMenu = menu_type;

    const char* menu_names[] = {"NONE", "INVENTORY", "CHARACTER", "QUEST", "SHOP", "GUILD", "CHAT", "SETTINGS"};
    std::cout << "[Menu] Opened: " << menu_names[menu_type] << std::endl;
}

void CMenuNavigation::CloseMenu()
{
    m_eActiveMenu = MENU_NONE;
    std::cout << "[Menu] Closed" << std::endl;
}

void CMenuNavigation::CloseAllMenus()
{
    m_eActiveMenu = MENU_NONE;
    m_fScrollOffset = 0.0f;
    std::cout << "[Menu] All menus closed" << std::endl;
}

void CMenuNavigation::SetInventorySize(int rows, int cols)
{
    m_iInventoryRows = rows;
    m_iInventoryCols = cols;
    CreateInventoryLayout();
}

void CMenuNavigation::CreateInventoryLayout()
{
    m_vecInventorySlots.clear();

    // Inventory genelde ekranın ortasında olur
    float inventory_width = m_fScreenWidth * 0.8f;
    float inventory_height = m_fScreenHeight * 0.7f;

    float start_x = (m_fScreenWidth - inventory_width) / 2.0f;
    float start_y = (m_fScreenHeight - inventory_height) / 2.0f + 80.0f; // Title bar için offset

    float slot_size = std::min(
        (inventory_width - 20) / m_iInventoryCols,
        (inventory_height - 20) / m_iInventoryRows
    );

    float spacing = 5.0f;

    for (int row = 0; row < m_iInventoryRows; row++)
    {
        for (int col = 0; col < m_iInventoryCols; col++)
        {
            InventorySlot slot;
            slot.slot_index = row * m_iInventoryCols + col;
            slot.x = start_x + col * (slot_size + spacing);
            slot.y = start_y + row * (slot_size + spacing);
            slot.width = slot_size;
            slot.height = slot_size;

            m_vecInventorySlots.push_back(slot);
        }
    }

    std::cout << "[Menu] Inventory layout created: " << m_iInventoryRows << "x" << m_iInventoryCols
              << " (" << m_vecInventorySlots.size() << " slots)" << std::endl;
}

void CMenuNavigation::SetInventoryItem(int slot_index, DWORD item_id, DWORD icon_id, DWORD count)
{
    if (slot_index >= 0 && slot_index < (int)m_vecInventorySlots.size())
    {
        auto& slot = m_vecInventorySlots[slot_index];
        slot.item_id = item_id;
        slot.icon_id = icon_id;
        slot.count = count;
    }
}

void CMenuNavigation::ClearInventorySlot(int slot_index)
{
    if (slot_index >= 0 && slot_index < (int)m_vecInventorySlots.size())
    {
        auto& slot = m_vecInventorySlots[slot_index];
        slot.item_id = 0;
        slot.icon_id = 0;
        slot.count = 0;
    }
}

int CMenuNavigation::GetSlotAtPosition(float x, float y) const
{
    float adjusted_y = y + m_fScrollOffset;

    for (const auto& slot : m_vecInventorySlots)
    {
        if (IsPointInSlot(x, adjusted_y, slot))
        {
            return slot.slot_index;
        }
    }

    return -1;
}

bool CMenuNavigation::IsPointInSlot(float x, float y, const InventorySlot& slot) const
{
    return (x >= slot.x && x <= slot.x + slot.width &&
            y >= slot.y && y <= slot.y + slot.height);
}

void CMenuNavigation::ProcessSlotClick(int slot_index)
{
    std::cout << "[Menu] Slot clicked: " << slot_index << std::endl;

    if (m_onSlotClick)
        m_onSlotClick(slot_index);
}

void CMenuNavigation::ProcessDragDrop(int from_slot, int to_slot)
{
    std::cout << "[Menu] Drag-drop: " << from_slot << " -> " << to_slot << std::endl;

    if (m_onDragDrop)
        m_onDragDrop(from_slot, to_slot);

    // Item'leri swap et
    if (from_slot >= 0 && from_slot < (int)m_vecInventorySlots.size() &&
        to_slot >= 0 && to_slot < (int)m_vecInventorySlots.size())
    {
        auto& slot_from = m_vecInventorySlots[from_slot];
        auto& slot_to = m_vecInventorySlots[to_slot];

        std::swap(slot_from.item_id, slot_to.item_id);
        std::swap(slot_from.icon_id, slot_to.icon_id);
        std::swap(slot_from.count, slot_to.count);
    }
}

void CMenuNavigation::DetectDoubleTap(int slot_index)
{
    DWORD current_time = 0; // Gerçek implementasyonda sistem zamanı

    if (m_iLastTappedSlot == slot_index &&
        current_time - m_dwLastTapTime < m_dwDoubleTapThreshold)
    {
        // Double tap detected - item kullan
        if (slot_index >= 0 && slot_index < (int)m_vecInventorySlots.size())
        {
            const auto& slot = m_vecInventorySlots[slot_index];
            if (slot.item_id > 0)
            {
                std::cout << "[Menu] Double tap - using item " << slot.item_id << std::endl;

                if (m_onItemUse)
                    m_onItemUse(slot_index, slot.item_id);
            }
        }

        m_iLastTappedSlot = -1;
        m_dwLastTapTime = 0;
    }
    else
    {
        m_iLastTappedSlot = slot_index;
        m_dwLastTapTime = current_time;
    }
}

void CMenuNavigation::Scroll(float delta_y)
{
    m_fScrollOffset += delta_y;

    // Clamp
    if (m_fScrollOffset < m_fScrollMin)
        m_fScrollOffset = m_fScrollMin;
    if (m_fScrollOffset > m_fScrollMax)
        m_fScrollOffset = m_fScrollMax;
}

void CMenuNavigation::AddQuickAccessButton(const std::string& label, std::function<void()> callback)
{
    auto btn = std::make_unique<CUIButton>();

    float btn_width = 100.0f;
    float btn_height = 50.0f;
    float btn_x = 50.0f;
    float btn_y = 100.0f + m_vecQuickButtons.size() * (btn_height + 10.0f);

    btn->Initialize(btn_x, btn_y, btn_width, btn_height);
    btn->SetText(label);
    btn->SetClickCallback(callback);

    m_vecQuickButtons.push_back(std::move(btn));

    std::cout << "[Menu] Quick button added: " << label << std::endl;
}
