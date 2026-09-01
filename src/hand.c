#include "hand.h"

void hand_init(Hand *hand, int capacity)
{
    hand->count = 0;
    hand->capacity = capacity;
    for (int i = 0; i < HAND_SIZE; i++)
    {
        hand->occupied[i] = false;
        hand->turnsHeld[i] = 0;
        hand->rottenSlot[i] = false;
    }
}

void hand_discardAll(Hand *hand)
{
    hand->count = 0;
    for (int i = 0; i < hand->capacity; i++)
    {
        hand->occupied[i] = false;
        hand->turnsHeld[i] = 0;
    }
}

int hand_fillOneSlot(Hand *hand, Deck *deck)
{
    for (int i = 0; i < hand->capacity; i++)
    {
        if (hand->occupied[i]) continue;
        if (deck_isEmpty(deck)) return -1;
        hand->cards[i] = deck_drawCard(deck);
        if (hand->rottenSlot[i]) hand->cards[i].isRotted = true;
        hand->occupied[i] = true;
        hand->turnsHeld[i] = 0;
        hand->count++;
        return i;
    }
    return -1;
}

int hand_countRottenCandidates(const Hand *hand)
{
    int count = 0;
    for (int i = 0; i < hand->capacity; i++)
        if (!hand->rottenSlot[i]) count++;
    return count;
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
            if (hand->occupied[s]) hand->cards[s].isRotted = true;
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
    hand->turnsHeld[index] = 0;
    hand->count--;
    return card;
}

bool hand_ageHeldCards(Hand *hand, bool rotEnabled)
{
    if (!rotEnabled) return false;
    bool justRotted = false;
    for (int i = 0; i < hand->capacity; i++)
    {
        if (!hand->occupied[i]) continue;
        hand->turnsHeld[i]++;
        if (hand->turnsHeld[i] >= HAND_ROT_TURNS && !hand->cards[i].isRotted)
        {
            hand->cards[i].isRotted = true;
            justRotted = true;
        }
    }
    return justRotted;
}
