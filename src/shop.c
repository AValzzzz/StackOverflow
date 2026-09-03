#include "shop.h"

static const ShopItemInfo ITEMS[ITEM_COUNT] = {
    [ITEM_WILDCARD]        = { "Wildcard (Joker)",        "Place a chosen value on a free cell",        3, false },
    [ITEM_MEMORY_FLUSH]    = { "Memory Flush (Cisailles)","Destroy a cell's card for free",              2, false },
    [ITEM_BUFFER_RELOAD]   = { "Buffer Reload (Aimant)",  "Discard hand, draw 4 new cards",              3, false },
    [ITEM_COMPILER_PATCH]  = { "Compiler Patch",          "Inject a Glitched card into this round's deck", 4, false },
    [ITEM_NULL_POINTER]    = { "Null Pointer",            "Purge a chosen value from this round's deck", 3, false },
    [ITEM_REDUNDANT_WARM]  = { "Redundant Color: Warm",   "[Architect] Hearts=Diamonds for Same Suit (penalty eases per level)", 6, true  },
    [ITEM_REDUNDANT_COOL]  = { "Redundant Color: Cool",   "[Architect] Clubs=Spades for Same Suit (penalty eases per level)", 6, true  },
    [ITEM_REDUNDANT_COLOR] = { "Redundant Color: Full",   "[Architect] Both Warm and Cool at once (penalty eases per level)", 11, true },
    [ITEM_BANKER_CHIP]     = { "Banker Chip",             "[Banker] Jacks count 0 toward the Stack Limit (2: also Queens, 3: also Kings)", 8, true  },
    [ITEM_TRY_CATCH]       = { "Try/Catch (Extincteur)",  "Absorbs 1/2/3 crashes per round",             6, true  },
    [ITEM_PREFETCH]           = { "Prefetch",             "Hand size 5/6/7",                             7, true  },
    [ITEM_GARBAGE_COLLECTOR]  = { "Garbage Collector",    "Every combo permanently raises your multiplier +0.2/0.4/0.6", 6, true  },
    [ITEM_OVERCLOCK]          = { "Overclock",            "+50/100/150 chips on Three of a Kind, Stack Limit penalty eases per level", 5, true  },
    [ITEM_ROLLBACK]           = { "Rollback",             "Undoes the last placement",                   3, false },
    [ITEM_MULTITHREAD]        = { "Multithread",          "Play 2 cards from your hand this turn",       4, false },
    [ITEM_CACHE_BOOST]        = { "Cache Boost",          "[Architect] Center cell combo bonus rises from 1.5x to 2.0x/2.25x/2.5x", 6, true },
    [ITEM_LOOP_UNROLL]        = { "Loop Unroll",          "High-card combo bonus scales harder per level", 6, true },
    [ITEM_DEFRAG]             = { "Defrag",               "Clears all rot from your hand and grid",      3, false },
    [ITEM_JIT_COMPILER]       = { "JIT Compiler",         "[Compiler] +2/4/6 multiplier per glitched card on the grid", 7, true },
    [ITEM_CLUB_CACHE]         = { "Club Cache",           "[Architect] A Club in a scored combo: x2.0/2.5/3.0 multiplier", 6, true },
    [ITEM_EXPLOIT]            = { "Exploit",              "[Compiler] A Glitched card in a scored combo: x3/4/5 multiplier", 8, true },
    [ITEM_CORE_DUMP]          = { "Core Dump",             "[Compiler] Glitched/Rotted cards: -2/4/6 flat off their Stack Score", 6, true },
    [ITEM_RACE_CONDITION]     = { "Race Condition",        "[Compiler] Drawn cards have a 10%/20%/30% chance to spawn Glitched", 5, true },
    [ITEM_STACK_CANARY]       = { "Stack Canary",          "[Compiler] Dampens Rot's multiplier growth (divides by 2/3/4)", 5, true },
    [ITEM_AMORTIZATION]       = { "Amortization",          "[Banker] Cards rank<=5/7/9 count half toward Stack Score", 6, true },
    [ITEM_DEBT_CEILING]       = { "Debt Ceiling",          "[Banker] Stack Limit +5%/10%/15%",            5, true },
    [ITEM_COMPOUND_INTEREST]  = { "Compound Interest",     "[Banker] Round-clear gold reward +10%/20%/30%", 5, true },
    [ITEM_EMERGENCY_FUND]     = { "Emergency Fund",        "[Banker] +$3/$6/$9 gold at the start of every round", 4, true },
    [ITEM_DIAGONAL_CACHE]     = { "Diagonal Cache",        "[Architect] Diagonal-mode combos: +25%/50%/75% extra multiplier", 6, true },
    [ITEM_DEALLOCATOR]        = { "Deallocator",           "A random suit (rolled on purchase) counts for less toward the Stack Score (3/4, then half, then quarter value)", 6, true },
    [ITEM_COMPRESSION_ALGORITHM] = { "Compression Algorithm", "Every combo cleared also cuts Stack Score by 5%/10%/15% per line (capped, can't zero it out)", 7, true },
    [ITEM_STACK_REWIND]       = { "Stack Rewind",          "Instantly halve your current Stack Score",    5, false },
    [ITEM_FORCE_PUSH]         = { "Force Push",            "Destroy the 2 heaviest cards on the grid for free", 5, false },
};

const ShopItemInfo *shop_getItemInfo(ShopItemId id)
{
    return &ITEMS[id];
}
