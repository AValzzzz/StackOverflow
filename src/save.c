#include "save.h"

#include <stdio.h>

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "tutorial.h"

#define SAVE_PATH     "save/game_save.dat"
#define SAVE_TMP_PATH "save/game_save.dat.tmp"
#define SAVE_MAGIC   0x534B5453u
#define SAVE_VERSION 4u

typedef struct SaveHeader {
    unsigned int magic;
    unsigned int version;
} SaveHeader;

bool save_exists(void)
{
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool save_write(const GameSave *save)
{
    GameSave toWrite = *save;
    toWrite.tutorialCompleted = tutorial_hasCompleted();

    FILE *f = fopen(SAVE_TMP_PATH, "wb");
    if (!f)
    {
        #ifdef _WIN32
            mkdir("save");
        #else
            mkdir("save", 0755);
        #endif
        f = fopen(SAVE_TMP_PATH, "wb");
        if (!f) return false;
    }

    SaveHeader header = { SAVE_MAGIC, SAVE_VERSION };
    bool ok = fwrite(&header, sizeof(header), 1, f) == 1 &&
              fwrite(&toWrite, sizeof(toWrite), 1, f) == 1 &&
              fflush(f) == 0;
    fclose(f);
    if (!ok)
    {
        remove(SAVE_TMP_PATH);
        return false;
    }

    #ifdef _WIN32
        ok = MoveFileExA(SAVE_TMP_PATH, SAVE_PATH, MOVEFILE_REPLACE_EXISTING) != 0;
    #else
        ok = rename(SAVE_TMP_PATH, SAVE_PATH) == 0;
    #endif
    if (!ok) remove(SAVE_TMP_PATH);
    return ok;
}

static bool cardValid(const Card *c)
{
    if (c->suit < 0 || c->suit >= SUIT_COUNT) return false;
    if (c->rank < RANK_ACE || c->rank > RANK_KING) return false;
    return true;
}

static bool itemSlotValid(int id)
{
    return id == NO_ITEM || (id >= 0 && id < ITEM_COUNT);
}

static bool moduleLevelValid(int level)
{
    return level >= 0 && level <= MODULE_MAX_LEVEL;
}

static bool inventoryValid(const Inventory *inv)
{
    if (inv->unlockedModuleSlots < MODULE_SLOTS_BASE || inv->unlockedModuleSlots > MODULE_SLOTS)
        return false;
    for (int i = 0; i < MODULE_SLOTS; i++)
        if (!itemSlotValid(inv->modules[i]) || !moduleLevelValid(inv->moduleLevels[i]))
            return false;
    for (int i = 0; i < SCRIPT_SLOTS; i++)
        if (!itemSlotValid(inv->scripts[i])) return false;
    if (!itemSlotValid(inv->classModule) || !moduleLevelValid(inv->classModuleLevel))
        return false;
    if (inv->tryCatchCharges < 0) return false;
    return true;
}

static bool saveValid(const GameSave *s, int startingClassCount)
{
    if (s->roundNumber < 1) return false;
    if (s->gold < 0) return false;
    if (s->ramUpgradesBought < 0) return false;
    if (s->startingClass < 0 || s->startingClass >= startingClassCount) return false;

    if (s->boughtCardCount < 0 || s->boughtCardCount > SAVE_MAX_CARDS) return false;
    if (s->removedCardCount < 0 || s->removedCardCount > SAVE_MAX_CARDS) return false;

    for (int i = 0; i < s->boughtCardCount; i++)
        if (!cardValid(&s->boughtCards[i])) return false;
    for (int i = 0; i < s->removedCardCount; i++)
        if (!cardValid(&s->removedCards[i])) return false;

    if (!inventoryValid(&s->inventory)) return false;

    for (int i = 0; i < CHESS_PIECE_TYPE_COUNT; i++)
        if (s->chessRoster[i] < 0) return false;
    if (s->chessMatchesPlayed < 0) return false;

    return true;
}

bool save_load(GameSave *outSave, int startingClassCount)
{
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f) return false;

    SaveHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 ||
        header.magic != SAVE_MAGIC || header.version != SAVE_VERSION)
    {
        fclose(f);
        return false;
    }

    GameSave loaded;
    bool ok = fread(&loaded, sizeof(loaded), 1, f) == 1;
    fclose(f);
    if (!ok) return false;

    if (!saveValid(&loaded, startingClassCount)) return false;

    if (loaded.tutorialCompleted) tutorial_markCompleted();

    *outSave = loaded;
    return true;
}

void save_delete(void)
{
    remove(SAVE_PATH);
}
