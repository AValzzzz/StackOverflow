#include "card_textures.h"

static Texture2D g_cardTextures[SUIT_COUNT][14];
static Texture2D g_backTexture;
static Texture2D g_emptyTexture;

static void loadWithPointFilter(Texture2D *tex, const char *path)
{
    *tex = LoadTexture(path);
    SetTextureFilter(*tex, TEXTURE_FILTER_POINT);
}

void cardtex_loadAll(void)
{
    for (Suit suit = 0; suit < SUIT_COUNT; suit++)
    {
        for (Rank rank = RANK_TWO; rank <= RANK_KING; rank++)
        {
            Card card = card_make(suit, rank);
            char path[128];
            card_getTexturePath(&card, path, sizeof(path));
            loadWithPointFilter(&g_cardTextures[suit][rank], path);
        }
        Card ace = card_make(suit, RANK_ACE);
        char path[128];
        card_getTexturePath(&ace, path, sizeof(path));
        loadWithPointFilter(&g_cardTextures[suit][RANK_ACE], path);
    }
    loadWithPointFilter(&g_backTexture, "assets/cards/card_back.png");
    loadWithPointFilter(&g_emptyTexture, "assets/cards/card_empty.png");
}

void cardtex_unloadAll(void)
{
    for (Suit suit = 0; suit < SUIT_COUNT; suit++)
        for (int rank = 1; rank <= 13; rank++)
            UnloadTexture(g_cardTextures[suit][rank]);
    UnloadTexture(g_backTexture);
    UnloadTexture(g_emptyTexture);
}

Texture2D cardtex_get(Suit suit, Rank rank) { return g_cardTextures[suit][rank]; }
Texture2D cardtex_getBack(void) { return g_backTexture; }
Texture2D cardtex_getEmpty(void) { return g_emptyTexture; }
