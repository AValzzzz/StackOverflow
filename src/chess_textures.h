#ifndef CHESS_TEXTURES_H
#define CHESS_TEXTURES_H

#include "raylib.h"

#include "chess.h"

void chesstex_loadAll(void);
void chesstex_unloadAll(void);

Texture2D chesstex_getSheet(ChessSide side);
Rectangle chesstex_getSourceRect(ChessPieceType type);

#endif
