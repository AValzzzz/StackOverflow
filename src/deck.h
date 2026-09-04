#ifndef DECK_H
#define DECK_H

#include <stdbool.h>

#include "card.h"

#define DECK_FULL_SIZE 52
#define DECK_INJECT_HEADROOM 18
#define DECK_MAX_SIZE (DECK_FULL_SIZE + DECK_INJECT_HEADROOM)

typedef struct Deck {
    Card cards[DECK_MAX_SIZE];
    int  count;

    Card discardPile[DECK_MAX_SIZE];
    int  discardCount;
} Deck;

void deck_initStandard52(Deck *deck);
void deck_shuffle(Deck *deck);
bool deck_isEmpty(const Deck *deck);
Card deck_drawCard(Deck *deck);

Card deck_drawCardWeighted(Deck *deck, float lowBias);

void deck_discard(Deck *deck, Card card);

void deck_reshuffleDiscardIntoDeck(Deck *deck);

void deck_injectCard(Deck *deck, Card card);

bool deck_purgeRank(Deck *deck, Rank rank);

bool deck_removeOneMatching(Deck *deck, Suit suit, Rank rank);

Card deck_drawLowestValueCard(Deck *deck);

Card deck_drawCardBelow(Deck *deck, int belowValue);

bool deck_hasCardBelow(const Deck *deck, int belowValue);

#endif
