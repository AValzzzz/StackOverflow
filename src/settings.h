#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

typedef struct Settings {
    int   language;
    float musicVolume;
    float sfxVolume;
    float masterVolume;
    float animSpeed;
    bool  hasChosenLanguage;
} Settings;

void settings_defaults(Settings *out);
bool settings_load(Settings *out);
void settings_write(const Settings *s);

#endif
