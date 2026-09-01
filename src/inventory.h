#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdbool.h>

#include "shop.h"

#define MODULE_SLOTS 2
#define SCRIPT_SLOTS 2
#define NO_ITEM (-1)

typedef struct Inventory {
    int  modules[MODULE_SLOTS];
    int  scripts[SCRIPT_SLOTS];
    bool tryCatchChargeAvailable; 
} Inventory;

void inventory_init(Inventory *inv);
bool inventory_hasModule(const Inventory *inv, ShopItemId id);
bool inventory_buyModule(Inventory *inv, ShopItemId id); 
bool inventory_buyScript(Inventory *inv, ShopItemId id); 
void inventory_consumeScript(Inventory *inv, int slotIndex);
void inventory_removeModule(Inventory *inv, int slotIndex);
bool inventory_moduleSlotsFull(const Inventory *inv);
bool inventory_scriptSlotsFull(const Inventory *inv);
void inventory_onRoundStart(Inventory *inv);

#endif
