#ifndef ROUND_H
#define ROUND_H

#include "memory_grid.h"

#define WIN_ROUND_TARGET 10

typedef struct RoundConfig {
    int roundNumber;
    int objective;
    int stackLimit;
    int turnLimit;
    int goldReward;
    bool isBossRound;
    ComboType disabledCombo;
    bool unstableDeckActive;
    bool extendedLockActive;
    bool memoryCorruptionActive;
    bool interruptsActive;
    bool chessUnlocked;
    int  glitchEventChancePercent;
} RoundConfig;

RoundConfig round_getConfig(int roundNumber);

int round_goldBonus(int cardsLeftInDeck);

#endif
