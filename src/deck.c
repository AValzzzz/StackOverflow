#include "deck.h"

#include <stdlib.h>

void deck_initStandard52(Deck *deck)
{
    int i = 0;
    for (Suit suit = 0; suit < SUIT_COUNT; suit++)
    {
        for (Rank rank = RANK_TWO; rank <= RANK_KING; rank++)
        {
            deck->cards[i++] = card_make(suit, rank);
        }
        deck->cards[i++] = card_make(suit, RANK_ACE);
    }
    deck->count = DECK_FULL_SIZE;
    deck->discardCount = 0;
}

void deck_shuffle(Deck *deck)
{
    for (int i = deck->count - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card tmp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = tmp;
    }
}

bool deck_isEmpty(const Deck *deck)
{
    return deck->count <= 0 && deck->discardCount <= 0;
}

Card deck_drawCard(Deck *deck)
{
    if (deck->count <= 0 && deck->discardCount > 0) deck_reshuffleDiscardIntoDeck(deck);
    if (deck->count <= 0) return card_make(SUIT_HEART, RANK_TWO);
    return deck->cards[--deck->count];
}

void deck_discard(Deck *deck, Card card)
{
    if (deck->discardCount >= DECK_MAX_SIZE) return;
    deck->discardPile[deck->discardCount++] = card;
}

void deck_reshuffleDiscardIntoDeck(Deck *deck)
{
    for (int i = 0; i < deck->discardCount && deck->count < DECK_MAX_SIZE; i++)
        deck->cards[deck->count++] = deck->discardPile[i];
    deck->discardCount = 0;
    deck_shuffle(deck);
}

void deck_injectCard(Deck *deck, Card card)
{
    if (deck->count >= DECK_MAX_SIZE) return;
    int j = rand() % (deck->count + 1);
    deck->cards[deck->count] = deck->cards[j];
    deck->cards[j] = card;
    deck->count++;
}

Card deck_drawLowestValueCard(Deck *deck)
{
    if (deck->count <= 0 && deck->discardCount > 0) deck_reshuffleDiscardIntoDeck(deck);
    if (deck->count <= 0) return card_make(SUIT_HEART, RANK_TWO);
    int bestIndex = 0;
    int bestValue = (deck->cards[0].rank == RANK_ACE) ? 1 : card_getEffectiveValue(&deck->cards[0]);
    for (int i = 1; i < deck->count; i++)
    {
        int value = (deck->cards[i].rank == RANK_ACE) ? 1 : card_getEffectiveValue(&deck->cards[i]);
        if (value < bestValue) { bestValue = value; bestIndex = i; }
    }
    Card result = deck->cards[bestIndex];
    if (result.rank == RANK_ACE) result.aceAsEleven = false;
    deck->cards[bestIndex] = deck->cards[deck->count - 1];
    deck->count--;
    return result;
}

Card deck_drawCardBelow(Deck *deck, int belowValue)
{
    if (deck->count <= 0 && deck->discardCount > 0) deck_reshuffleDiscardIntoDeck(deck);
    if (deck->count <= 0) return card_make(SUIT_HEART, RANK_TWO); 
    int candidates[DECK_MAX_SIZE];
    int count = 0;
    for (int i = 0; i < deck->count; i++)
    {
        int value = (deck->cards[i].rank == RANK_ACE) ? 1 : card_getEffectiveValue(&deck->cards[i]);
        if (value < belowValue) candidates[count++] = i;
    }
    if (count == 0) return deck_drawLowestValueCard(deck);

    int index = candidates[rand() % count];
    Card result = deck->cards[index];
    if (result.rank == RANK_ACE) result.aceAsEleven = false;
    deck->cards[index] = deck->cards[deck->count - 1];
    deck->count--;
    return result;
}

bool deck_hasCardBelow(const Deck *deck, int belowValue)
{
    for (int i = 0; i < deck->count; i++)
    {
        int value = (deck->cards[i].rank == RANK_ACE) ? 1 : card_getEffectiveValue(&deck->cards[i]);
        if (value < belowValue) return true;
    }
    for (int i = 0; i < deck->discardCount; i++)
    {
        int value = (deck->discardPile[i].rank == RANK_ACE) ? 1 : card_getEffectiveValue(&deck->discardPile[i]);
        if (value < belowValue) return true;
    }
    return false;
}

bool deck_purgeRank(Deck *deck, Rank rank)
{
    for (int i = 0; i < deck->count; i++)
    {
        if (deck->cards[i].rank == rank)
        {
            deck->cards[i] = deck->cards[deck->count - 1];
            deck->count--;
            return true;
        }
    }
    for (int i = 0; i < deck->discardCount; i++)
    {
        if (deck->discardPile[i].rank == rank)
        {
            deck->discardPile[i] = deck->discardPile[deck->discardCount - 1];
            deck->discardCount--;
            return true;
        }
    }
    return false;
}
