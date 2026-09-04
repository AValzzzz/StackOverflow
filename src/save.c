#include "save.h"

#include <stdio.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "tutorial.h"

#define SAVE_PATH "save/game_save.dat"
#define SAVE_MAGIC   0x534B5453u
#define SAVE_VERSION 2u

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

    FILE *f = fopen(SAVE_PATH, "wb");
    if (!f)
    {
        #ifdef _WIN32
            mkdir("save");
        #else
            mkdir("save", 0755);
        #endif
        f = fopen(SAVE_PATH, "wb");
        if (!f) return false;
    }

    SaveHeader header = { SAVE_MAGIC, SAVE_VERSION };
    bool ok = fwrite(&header, sizeof(header), 1, f) == 1 &&
              fwrite(&toWrite, sizeof(toWrite), 1, f) == 1;
    fclose(f);
    return ok;
}

bool save_load(GameSave *outSave)
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

    if (loaded.boughtCardCount < 0 || loaded.boughtCardCount > SAVE_MAX_CARDS) return false;
    if (loaded.removedCardCount < 0 || loaded.removedCardCount > SAVE_MAX_CARDS) return false;

    if (loaded.tutorialCompleted) tutorial_markCompleted();

    *outSave = loaded;
    return true;
}

void save_delete(void)
{
    remove(SAVE_PATH);
}
