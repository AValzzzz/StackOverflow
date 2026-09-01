#ifndef HAND_H
#define HAND_H

#include <stdbool.h>

#include "card.h"
#include "deck.h"

#define HAND_SIZE 5
#define HAND_DEFAULT_CAPACITY 4

#define HAND_ROT_TURNS 2
#define HAND_ROT_GRACE_TURNS 8

typedef struct Hand {
    Card cards[HAND_SIZE];
    bool occupied[HAND_SIZE];
    int  turnsHeld[HAND_SIZE];
    bool rottenSlot[HAND_SIZE];
    int  capacity;
    int  count;
} Hand;

void hand_init(Hand *hand, int capacity);
void hand_discardAll(Hand *hand);

int hand_fillOneSlot(Hand *hand, Deck *deck);

Card hand_removeAt(Hand *hand, int index);

bool hand_ageHeldCards(Hand *hand, bool rotEnabled);

int  hand_countRottenCandidates(const Hand *hand);
bool hand_addRottenSlotAtIndex(Hand *hand, int index);

#endif
