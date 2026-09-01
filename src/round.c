#include "round.h"

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
        cfg.objective  = 4000 + 2400 * extraRounds;
        cfg.stackLimit = 60 - (int)(extraRounds * 1.5f);
        if (cfg.stackLimit < 40) cfg.stackLimit = 40;
        cfg.goldReward = 10 + 2 * extraRounds;
    }

    cfg.turnLimit = 30 - (roundNumber - 1);
    if (cfg.turnLimit < 16) cfg.turnLimit = 16;

    cfg.isBossRound = (roundNumber > 0) && (roundNumber % 5 == 0);

    cfg.disabledCombo = COMBO_NONE;
    if (!cfg.isBossRound && roundNumber % 4 == 0)
        cfg.disabledCombo = (roundNumber % 8 == 0) ? COMBO_BRELAN : COMBO_SAME_SUIT;

    cfg.unstableDeckActive     = roundNumber >= 5;
    cfg.extendedLockActive     = roundNumber >= 8;
    cfg.memoryCorruptionActive = roundNumber >= 12;
    cfg.rottenSlotsActive      = roundNumber >= 5;

    return cfg;
}

int round_goldBonus(int cardsLeftInDeck)
{
    return cardsLeftInDeck / 5;
}
