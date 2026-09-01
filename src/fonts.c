#include "fonts.h"

#include <stddef.h>

#define BOLD_PIXELS_BASE_SIZE 64

static Font g_boldPixels;

void fonts_loadAll(void)
{
    g_boldPixels = LoadFontEx("assets/fonts/BoldPixels.ttf", BOLD_PIXELS_BASE_SIZE, NULL, 0);
    SetTextureFilter(g_boldPixels.texture, TEXTURE_FILTER_POINT);
}

void fonts_unloadAll(void)
{
    UnloadFont(g_boldPixels);
}

Font fonts_get(void)
{
    return g_boldPixels;
}
