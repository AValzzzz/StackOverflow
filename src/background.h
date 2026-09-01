#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "raylib.h"

void background_load(int screenWidth, int screenHeight);
void background_unload(void);

void background_draw(int screenWidth, int screenHeight, Color baseColor, Color gridLineColor);

#endif
