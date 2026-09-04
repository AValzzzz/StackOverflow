#include "tutorial.h"

#include <stdio.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#define TUTORIAL_MARKER_PATH "save/tutorial_complete.flag"
#define CHESS_INTRO_MARKER_PATH "save/chess_tutorial_complete.flag"
#define SHOP_INTRO_MARKER_PATH "save/shop_tutorial_complete.flag"
#define CONDITION_INTRO_MARKER_PATH "save/condition_tutorial_complete.flag"

static bool markerExists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return false;
    fclose(f);
    return true;
}

static void createMarker(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f)
    {
        #ifdef _WIN32
            mkdir("save");
        #else
            mkdir("save", 0755);
        #endif
        f = fopen(path, "w");
        if (!f) return;
    }
    fputs("1", f);
    fclose(f);
}

bool tutorial_hasCompleted(void)
{
    return markerExists(TUTORIAL_MARKER_PATH);
}

void tutorial_markCompleted(void)
{
    createMarker(TUTORIAL_MARKER_PATH);
}

bool tutorial_hasSeenChessIntro(void)
{
    return markerExists(CHESS_INTRO_MARKER_PATH);
}

void tutorial_markChessIntroSeen(void)
{
    createMarker(CHESS_INTRO_MARKER_PATH);
}

bool tutorial_hasSeenShopIntro(void)
{
    return markerExists(SHOP_INTRO_MARKER_PATH);
}

void tutorial_markShopIntroSeen(void)
{
    createMarker(SHOP_INTRO_MARKER_PATH);
}

bool tutorial_hasSeenConditionIntro(void)
{
    return markerExists(CONDITION_INTRO_MARKER_PATH);
}

void tutorial_markConditionIntroSeen(void)
{
    createMarker(CONDITION_INTRO_MARKER_PATH);
}
