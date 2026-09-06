#ifndef TUTORIAL_H
#define TUTORIAL_H

#include <stdbool.h>

bool tutorial_hasCompleted(void);

void tutorial_markCompleted(void);

bool tutorial_hasSeenChessIntro(void);

void tutorial_markChessIntroSeen(void);

bool tutorial_hasSeenShopIntro(void);

void tutorial_markShopIntroSeen(void);

bool tutorial_hasSeenConditionIntro(void);

void tutorial_markConditionIntroSeen(void);

#endif
