#include "hand.h"

void hand_init(Hand *hand, int capacity)
{
    hand->count = 0;
    hand->capacity = capacity;
    for (int i = 0; i < HAND_SIZE; i++)
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
        hand->occupied[i] = true;
        hand->turnsHeld[i] = 0;
        hand->count++;
        return i;
    }
    return -1;
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
