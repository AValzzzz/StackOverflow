#ifndef CARD_TEXTURES_H
#define CARD_TEXTURES_H

#include "raylib.h"
#include "card.h"

void cardtex_loadAll(void);
void cardtex_unloadAll(void);
Texture2D cardtex_get(Suit suit, Rank rank);
Texture2D cardtex_getBack(void);
Texture2D cardtex_getEmpty(void);

#endif
