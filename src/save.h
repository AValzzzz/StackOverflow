#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>

#include "card.h"
#include "chess.h"
#include "inventory.h"

#define SAVE_MAX_CARDS 32

typedef struct GameSave {
    int roundNumber;
    int gold;
    int startingClass;

    Inventory inventory;

    Card boughtCards[SAVE_MAX_CARDS];
    int  boughtCardCount;
    Card removedCards[SAVE_MAX_CARDS];
    int  removedCardCount;

    int chessRoster[CHESS_PIECE_TYPE_COUNT];

    bool tutorialCompleted;
} GameSave;

bool save_exists(void);
bool save_write(const GameSave *save);
bool save_load(GameSave *outSave);
void save_delete(void);

#endif
