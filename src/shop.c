#include "shop.h"

static const ShopItemInfo ITEMS[ITEM_COUNT] = {
    [ITEM_WILDCARD]        = { "Wildcard (Joker)",        "Place a chosen value on a free cell",        3, false },
    [ITEM_MEMORY_FLUSH]    = { "Memory Flush (Cisailles)","Destroy a cell's card for free",              2, false },
    [ITEM_BUFFER_RELOAD]   = { "Buffer Reload (Aimant)",  "Discard hand, draw 4 new cards",              3, false },
    [ITEM_COMPILER_PATCH]  = { "Compiler Patch",          "Inject a Glitched card into this round's deck", 4, false },
    [ITEM_NULL_POINTER]    = { "Null Pointer",            "Purge a chosen value from this round's deck", 3, false },
    [ITEM_REDUNDANT_COLOR] = { "Redundant Color",         "Clubs=Spades, Hearts=Diamonds for Same Suit", 7, true  },
    [ITEM_BANKER_CHIP]     = { "Banker Chip",             "Face cards count 0 toward the Stack Limit",   8, true  },
    [ITEM_TRY_CATCH]       = { "Try/Catch (Extincteur)",  "Absorbs one crash per round",                 6, true  },
    [ITEM_PREFETCH]           = { "Prefetch",             "Hand size 5 instead of 4",                    7, true  },
    [ITEM_GARBAGE_COLLECTOR]  = { "Garbage Collector",    "Every combo frees the lowest tile not in it", 6, true  },
    [ITEM_SEGFAULT_HANDLER]   = { "Segfault Handler",     "Aces never bust you, even over a combo",      7, true  },
    [ITEM_OVERCLOCK]          = { "Overclock",            "+20% combo points, -10% Stack Limit",         5, true  },
    [ITEM_STACK_TRACE]        = { "Stack Trace",          "Reveals the next 3 cards for the round",      2, false },
    [ITEM_ROLLBACK]           = { "Rollback",             "Undoes the last placement",                   3, false },
    [ITEM_MULTITHREAD]        = { "Multithread",          "Play 2 cards from your hand this turn",       4, false },
};

const ShopItemInfo *shop_getItemInfo(ShopItemId id)
{
    return &ITEMS[id];
}
