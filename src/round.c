#include "round.h"

#include <math.h>

#include "raylib.h"

#define OBJECTIVE_GROWTH_PRE_WIN  1.33
#define OBJECTIVE_GROWTH_POST_WIN 1.5

RoundConfig round_getConfig(int roundNumber)
{
    RoundConfig cfg;
    cfg.roundNumber = roundNumber;

    if (roundNumber == 1)      { cfg.objective = 800;  cfg.stackLimit = 80; cfg.goldReward = 6; }
    else if (roundNumber == 2) { cfg.objective = 2000; cfg.stackLimit = 70; cfg.goldReward = 8; }
    else if (roundNumber == 3) { cfg.objective = 4000; cfg.stackLimit = 60; cfg.goldReward = 10; }
    else
    {
        int extraRounds = roundNumber - 3;
        if (roundNumber <= WIN_ROUND_TARGET)
        {

            cfg.objective = (int)(4000.0 * pow(OBJECTIVE_GROWTH_PRE_WIN, extraRounds));
        }
        else
        {
            int winObjective = (int)(4000.0 * pow(OBJECTIVE_GROWTH_PRE_WIN, WIN_ROUND_TARGET - 3));
            int postWinRounds = roundNumber - WIN_ROUND_TARGET;
            cfg.objective = (int)((double)winObjective * pow(OBJECTIVE_GROWTH_POST_WIN, postWinRounds));
        }
        cfg.stackLimit = 60 - (int)(extraRounds * 2.0f);
        if (cfg.stackLimit < 36) cfg.stackLimit = 36;
        cfg.goldReward = 10 + (int)(extraRounds * 1.5f);
    }

    cfg.turnLimit = 30 - (roundNumber - 1);
    if (cfg.turnLimit < 16) cfg.turnLimit = 16;

    cfg.isBossRound = (roundNumber > 0) && (roundNumber % 5 == 0);

    cfg.disabledCombo = COMBO_NONE;
    if (!cfg.isBossRound && roundNumber % 4 == 0)
        cfg.disabledCombo = COMBO_SAME_SUIT;

    cfg.unstableDeckActive     = roundNumber >= 4  && GetRandomValue(1, 100) <= 85;
    cfg.extendedLockActive     = roundNumber >= 7  && GetRandomValue(1, 100) <= 80;
    cfg.memoryCorruptionActive = roundNumber >= 10 && GetRandomValue(1, 100) <= 75;
    cfg.interruptsActive       = roundNumber >= 3;
    cfg.chessUnlocked          = roundNumber >= 5;

    cfg.glitchEventChancePercent = 0;
    if (roundNumber >= 3)
    {
        int chance = 3 + (roundNumber - 3) / 2;
        cfg.glitchEventChancePercent = chance > 20 ? 20 : chance;
    }

    return cfg;
}

int round_goldBonus(int cardsLeftInDeck)
{
    return cardsLeftInDeck / 5;
}
