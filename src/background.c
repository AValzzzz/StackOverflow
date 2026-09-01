#include "background.h"

#define GRID_STEP 40

static Texture2D g_vignette;

void background_load(int screenWidth, int screenHeight)
{
    Image img = GenImageGradientRadial(screenWidth, screenHeight, 0.35f,
                                         (Color){ 0, 0, 0, 0 }, (Color){ 0, 0, 0, 170 });
    g_vignette = LoadTextureFromImage(img);
    UnloadImage(img);
}

void background_unload(void)
{
    UnloadTexture(g_vignette);
}

void background_draw(int screenWidth, int screenHeight, Color baseColor, Color gridLineColor)
{
    ClearBackground(baseColor);

    for (int x = GRID_STEP; x < screenWidth; x += GRID_STEP)
        DrawLine(x, 0, x, screenHeight, gridLineColor);
    for (int y = GRID_STEP; y < screenHeight; y += GRID_STEP)
        DrawLine(0, y, screenWidth, y, gridLineColor);

    DrawTexture(g_vignette, 0, 0, WHITE);
}
