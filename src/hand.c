#include "hand.h"

void hand_init(Hand *hand, int capacity)
{
    hand->count = 0;
    hand->capacity = capacity;
    for (int i = 0; i < HAND_SIZE; i++)
    {
        hand->occupied[i] = false;
        hand->rottenSlot[i] = false;
    }
}

void hand_discardAll(Hand *hand)
{
    hand->count = 0;
    for (int i = 0; i < hand->capacity; i++)
        hand->occupied[i] = false;
}

int hand_fillOneSlot(Hand *hand, Deck *deck, float lowBias)
{
    for (int i = 0; i < hand->capacity; i++)
    {
        if (hand->occupied[i]) continue;
        if (deck_isEmpty(deck)) return -1;
        hand->cards[i] = deck_drawCardWeighted(deck, lowBias);
        if (hand->rottenSlot[i]) card_markRotted(&hand->cards[i]);
        hand->occupied[i] = true;
        hand->count++;
        return i;
    }
    return -1;
}

bool hand_addRottenSlotAtIndex(Hand *hand, int index)
{
    int i = 0;
    for (int s = 0; s < hand->capacity; s++)
    {
        if (hand->rottenSlot[s]) continue;
        if (i == index)
        {
            hand->rottenSlot[s] = true;
            if (hand->occupied[s]) card_markRotted(&hand->cards[s]);
            return true;
        }
        i++;
    }
    return false;
}

Card hand_removeAt(Hand *hand, int index)
{
    Card card = hand->cards[index];
    hand->occupied[index] = false;
    hand->count--;
    return card;
}

void hand_ageRotValues(Hand *hand)
{
    for (int i = 0; i < hand->capacity; i++)
        if (hand->occupied[i])
            card_ageRot(&hand->cards[i]);
}
