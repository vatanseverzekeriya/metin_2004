#ifndef __INC_MOBILE_MENU_NAVIGATION_H__
#define __INC_MOBILE_MENU_NAVIGATION_H__

#include "../common/types.h"
#include "TouchInput.h"
#include "UIButton.h"
#include <vector>
#include <memory>
#include <functional>

/**
 * Metin2 Mobil Menü Navigasyon Sistemi
 *
 * Inventory, Character, Quest, Shop gibi menülerde
 * dokunmatik kontrol sağlar.
 */

// Menü tipleri
enum EMenuType
{
    MENU_NONE = 0,
    MENU_INVENTORY = 1,
    MENU_CHARACTER = 2,
    MENU_QUEST = 3,
    MENU_SHOP = 4,
    MENU_GUILD = 5,
    MENU_CHAT = 6,
    MENU_SETTINGS = 7
};

// Inventory slot bilgisi
struct InventorySlot
{
    int slot_index;
    DWORD item_id;
    DWORD icon_id;
    DWORD count;
    float x, y, width, height;

    InventorySlot()
        : slot_index(0), item_id(0), icon_id(0), count(0)
        , x(0), y(0), width(0), height(0) {}
};

// Drag-drop state
struct DragDropState
{
    bool active;
    int source_slot;
    float start_x, start_y;
    float current_x, current_y;
    DWORD item_id;
    DWORD touch_id;

    DragDropState()
        : active(false), source_slot(-1)
        , start_x(0), start_y(0), current_x(0), current_y(0)
        , item_id(0), touch_id(0) {}
};

// Callbacks
typedef std::function<void(int slot_index)> SlotClickCallback;
typedef std::function<void(int from_slot, int to_slot)> SlotDragDropCallback;
typedef std::function<void(int slot_index, DWORD item_id)> ItemUseCallback;

class CMenuNavigation
{
public:
    static CMenuNavigation& Instance();

    // Başlatma
    void Initialize(float screen_width, float screen_height);
    void Update(DWORD delta_time);
    void Render();

    // Touch input
    void OnTouchDown(const TouchPoint& touch);
    void OnTouchMove(const TouchPoint& touch);
    void OnTouchUp(const TouchPoint& touch);

    // Menü kontrolü
    void OpenMenu(EMenuType menu_type);
    void CloseMenu();
    void CloseAllMenus();
    EMenuType GetActiveMenu() const { return m_eActiveMenu; }
    bool IsMenuOpen() const { return m_eActiveMenu != MENU_NONE; }

    // Inventory yönetimi
    void SetInventorySize(int rows, int cols);
    void SetInventoryItem(int slot_index, DWORD item_id, DWORD icon_id, DWORD count);
    void ClearInventorySlot(int slot_index);
    int GetSlotAtPosition(float x, float y) const;

    // Drag-drop
    void EnableDragDrop(bool enable) { m_bDragDropEnabled = enable; }
    bool IsDragging() const { return m_dragDropState.active; }
    const DragDropState& GetDragDropState() const { return m_dragDropState; }

    // Callbacks
    void SetSlotClickCallback(SlotClickCallback callback) { m_onSlotClick = callback; }
    void SetDragDropCallback(SlotDragDropCallback callback) { m_onDragDrop = callback; }
    void SetItemUseCallback(ItemUseCallback callback) { m_onItemUse = callback; }

    // Scroll support (uzun listeler için)
    void SetScrollOffset(float offset_y) { m_fScrollOffset = offset_y; }
    float GetScrollOffset() const { return m_fScrollOffset; }
    void Scroll(float delta_y);

    // Quick access buttons
    void AddQuickAccessButton(const std::string& label, std::function<void()> callback);

private:
    CMenuNavigation();
    ~CMenuNavigation();

    CMenuNavigation(const CMenuNavigation&) = delete;
    CMenuNavigation& operator=(const CMenuNavigation&) = delete;

    void CreateInventoryLayout();
    void UpdateDragDrop(const TouchPoint& touch);
    void ProcessSlotClick(int slot_index);
    void ProcessDragDrop(int from_slot, int to_slot);

    bool IsPointInSlot(float x, float y, const InventorySlot& slot) const;
    void DetectDoubleTap(int slot_index);

    // Screen info
    float m_fScreenWidth;
    float m_fScreenHeight;

    // Active menu
    EMenuType m_eActiveMenu;

    // Inventory
    std::vector<InventorySlot> m_vecInventorySlots;
    int m_iInventoryRows;
    int m_iInventoryCols;

    // Drag-drop
    bool m_bDragDropEnabled;
    DragDropState m_dragDropState;
    float m_fDragThreshold;  // Min mesafe (piksel)

    // Scroll
    float m_fScrollOffset;
    float m_fScrollMin;
    float m_fScrollMax;

    // Double tap detection (item kullanımı için)
    int m_iLastTappedSlot;
    DWORD m_dwLastTapTime;
    DWORD m_dwDoubleTapThreshold;

    // Callbacks
    SlotClickCallback m_onSlotClick;
    SlotDragDropCallback m_onDragDrop;
    ItemUseCallback m_onItemUse;

    // Quick access buttons
    std::vector<std::unique_ptr<CUIButton>> m_vecQuickButtons;
};

#endif // __INC_MOBILE_MENU_NAVIGATION_H__
