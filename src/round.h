#ifndef ROUND_H
#define ROUND_H

#include "memory_grid.h"

typedef struct RoundConfig {
    int roundNumber;
    int objective;
    int stackLimit;
    int goldReward;
    bool isBossRound;
    ComboType disabledCombo;
    bool unstableDeckActive;
    bool extendedLockActive;
    bool memoryCorruptionActive;
} RoundConfig;

RoundConfig round_getConfig(int roundNumber);

int round_goldBonus(int cardsLeftInDeck);

#endif
