#include "inventory.h"

void inventory_init(Inventory *inv)
{
    for (int i = 0; i < MODULE_SLOTS; i++)
    {
        inv->modules[i] = NO_ITEM;
        inv->moduleLevels[i] = 0;
    }
    for (int i = 0; i < SCRIPT_SLOTS; i++) inv->scripts[i] = NO_ITEM;
    inv->classModule = NO_ITEM;
    inv->classModuleLevel = 0;
    inv->tryCatchCharges = 0;
}

bool inventory_hasModule(const Inventory *inv, ShopItemId id)
{
    if (inv->classModule == (int)id) return true;
    for (int i = 0; i < MODULE_SLOTS; i++)
        if (inv->modules[i] == (int)id) return true;
    return false;
}

int inventory_getModuleLevel(const Inventory *inv, ShopItemId id)
{
    if (inv->classModule == (int)id) return inv->classModuleLevel;
    for (int i = 0; i < MODULE_SLOTS; i++)
        if (inv->modules[i] == (int)id) return inv->moduleLevels[i];
    return 0;
}

void inventory_grantClassModule(Inventory *inv, ShopItemId id)
{
    inv->classModule = (int)id;
    inv->classModuleLevel = 1;
}

static int firstFree(const int *slots, int count)
{
    for (int i = 0; i < count; i++)
        if (slots[i] == NO_ITEM) return i;
    return -1;
}

bool inventory_buyModule(Inventory *inv, ShopItemId id)
{
    if (inv->classModule == (int)id)
    {
        if (inv->classModuleLevel >= MODULE_MAX_LEVEL) return false;
        inv->classModuleLevel++;
        return true;
    }
    for (int i = 0; i < MODULE_SLOTS; i++)
    {
        if (inv->modules[i] != (int)id) continue;
        if (inv->moduleLevels[i] >= MODULE_MAX_LEVEL) return false;
        inv->moduleLevels[i]++;
        return true;
    }
    int slot = firstFree(inv->modules, MODULE_SLOTS);
    if (slot < 0) return false;
    inv->modules[slot] = (int)id;
    inv->moduleLevels[slot] = 1;
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
    inv->moduleLevels[slotIndex] = 0;
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
    inv->tryCatchCharges = inventory_getModuleLevel(inv, ITEM_TRY_CATCH);
}
