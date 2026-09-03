#include "chess_textures.h"

#define CHESS_SPRITE_SIZE 16

static Texture2D g_whitePieces;
static Texture2D g_blackPieces;

void chesstex_loadAll(void)
{
    g_whitePieces = LoadTexture("assets/pieces/WhitePieces.png");
    g_blackPieces = LoadTexture("assets/pieces/BlackPieces.png");
    SetTextureFilter(g_whitePieces, TEXTURE_FILTER_POINT); 
    SetTextureFilter(g_blackPieces, TEXTURE_FILTER_POINT);
}

void chesstex_unloadAll(void)
{
    UnloadTexture(g_whitePieces);
    UnloadTexture(g_blackPieces);
}

Texture2D chesstex_getSheet(ChessSide side)
{
    return side == CHESS_SIDE_PLAYER ? g_whitePieces : g_blackPieces;
}

Rectangle chesstex_getSourceRect(ChessPieceType type)
{
    static const int COLUMN_FOR_TYPE[CHESS_PIECE_TYPE_COUNT] = {
        [CHESS_PAWN]   = 0,
        [CHESS_KNIGHT] = 1,
        [CHESS_ROOK]   = 2,
        [CHESS_BISHOP] = 3,
        [CHESS_QUEEN]  = 4,
        [CHESS_KING]   = 5,
    };
    int col = COLUMN_FOR_TYPE[type];
    return (Rectangle){ (float)(col * CHESS_SPRITE_SIZE), 0.0f, CHESS_SPRITE_SIZE, CHESS_SPRITE_SIZE };
}
