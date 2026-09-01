#include "tutorial.h"

#include <stdio.h>

#ifdef _WIN32
    #include <direct.h>   
#else
    #include <sys/stat.h>  
    #include <sys/types.h>
#endif

#define TUTORIAL_MARKER_PATH "save/tutorial_complete.flag"

bool tutorial_hasCompleted(void)
{
    FILE *f = fopen(TUTORIAL_MARKER_PATH, "r");
    if (!f) return false;
    fclose(f);
    return true;
}

void tutorial_markCompleted(void)
{
    FILE *f = fopen(TUTORIAL_MARKER_PATH, "w");
    if (!f)
    {
        #ifdef _WIN32
            mkdir("save");
        #else
            mkdir("save", 0755);
        #endif
        f = fopen(TUTORIAL_MARKER_PATH, "w");
        if (!f) return;
    }
    fputs("1", f);
    fclose(f);
}
