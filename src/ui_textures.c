#include "ui_textures.h"

static Texture2D g_padlock;

void uitex_loadAll(void)
{
    g_padlock = LoadTexture("assets/ui/padlock.png");
    SetTextureFilter(g_padlock, TEXTURE_FILTER_BILINEAR);
}

void uitex_unloadAll(void)
{
    UnloadTexture(g_padlock);
}

Texture2D uitex_getPadlock(void)
{
    return g_padlock;
}
