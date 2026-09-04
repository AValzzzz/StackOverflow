#include "fonts.h"

#include <stddef.h>

#define BOLD_PIXELS_BASE_SIZE 64

static Font g_boldPixels;

static int buildCodepoints(int out[256])
{
    int n = 0;
    for (int c = 32; c <= 126; c++) out[n++] = c;
    for (int c = 0xA0; c <= 0xFF; c++) out[n++] = c;
    out[n++] = 0x152;
    out[n++] = 0x153;
    out[n++] = 0x178;
    return n;
}

void fonts_loadAll(void)
{
    int codepoints[256];
    int count = buildCodepoints(codepoints);
    g_boldPixels = LoadFontEx("assets/fonts/BoldPixels.ttf", BOLD_PIXELS_BASE_SIZE, codepoints, count);
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
