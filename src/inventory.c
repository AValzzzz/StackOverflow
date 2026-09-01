#include "inventory.h"

void inventory_init(Inventory *inv)
{
    for (int i = 0; i < MODULE_SLOTS; i++) inv->modules[i] = NO_ITEM;
    for (int i = 0; i < SCRIPT_SLOTS; i++) inv->scripts[i] = NO_ITEM;
    inv->tryCatchChargeAvailable = false;
}

bool inventory_hasModule(const Inventory *inv, ShopItemId id)
{
    for (int i = 0; i < MODULE_SLOTS; i++)
        if (inv->modules[i] == (int)id) return true;
    return false;
}

static int firstFree(const int *slots, int count)
{
    for (int i = 0; i < count; i++)
        if (slots[i] == NO_ITEM) return i;
    return -1;
}

bool inventory_buyModule(Inventory *inv, ShopItemId id)
{
    if (inventory_hasModule(inv, id)) return false;
    int slot = firstFree(inv->modules, MODULE_SLOTS);
    if (slot < 0) return false;
    inv->modules[slot] = (int)id;
    return true;
}

bool inventory_buyScript(Inventory *inv, ShopItemId id)
{
    int slot = firstFree(inv->scripts, SCRIPT_SLOTS);
    if (slot < 0) return false;
    inv->scripts[slot] = (int)id;
    return true;
}

void inventory_consumeScript(Inventory *inv, int slotIndex)
{
    inv->scripts[slotIndex] = NO_ITEM;
}

void inventory_removeModule(Inventory *inv, int slotIndex)
{
    inv->modules[slotIndex] = NO_ITEM;
}

bool inventory_moduleSlotsFull(const Inventory *inv)
{
    return firstFree(inv->modules, MODULE_SLOTS) < 0;
}

bool inventory_scriptSlotsFull(const Inventory *inv)
{
    return firstFree(inv->scripts, SCRIPT_SLOTS) < 0;
}

void inventory_onRoundStart(Inventory *inv)
{
    inv->tryCatchChargeAvailable = inventory_hasModule(inv, ITEM_TRY_CATCH);
}
