#include "raylib.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "STACK OVERFLOW");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground((Color){ 30, 30, 30, 255 });
            DrawText("STACK OVERFLOW", 20, 20, 30, (Color){ 78, 201, 176, 255 });
            DrawText("Core engine not implemented yet.", 20, 60, 20, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
