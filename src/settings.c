#include "settings.h"

#include <stdio.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#define SETTINGS_PATH "save/settings.dat"
#define SETTINGS_MAGIC   0x53455454u
#define SETTINGS_VERSION 1u

typedef struct SettingsHeader {
    unsigned int magic;
    unsigned int version;
} SettingsHeader;

void settings_defaults(Settings *out)
{
    out->language = 0;
    out->musicVolume = 0.6f;
    out->sfxVolume = 0.8f;
    out->masterVolume = 1.0f;
    out->animSpeed = 1.0f;
    out->hasChosenLanguage = false;
}

bool settings_load(Settings *out)
{
    FILE *f = fopen(SETTINGS_PATH, "rb");
    if (!f) return false;

    SettingsHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 ||
        header.magic != SETTINGS_MAGIC || header.version != SETTINGS_VERSION)
    {
        fclose(f);
        return false;
    }

    Settings loaded;
    bool ok = fread(&loaded, sizeof(loaded), 1, f) == 1;
    fclose(f);
    if (!ok) return false;

    *out = loaded;
    return true;
}

void settings_write(const Settings *s)
{
    FILE *f = fopen(SETTINGS_PATH, "wb");
    if (!f)
    {
        #ifdef _WIN32
            mkdir("save");
        #else
            mkdir("save", 0755);
        #endif
        f = fopen(SETTINGS_PATH, "wb");
        if (!f) return;
    }

    SettingsHeader header = { SETTINGS_MAGIC, SETTINGS_VERSION };
    fwrite(&header, sizeof(header), 1, f);
    fwrite(s, sizeof(*s), 1, f);
    fclose(f);
}
