#include "card.h"

#include <stdio.h>

#define CARD_ROT_CAP_TURNS 3
Card card_make(Suit suit, Rank rank)
{
    Card card;
    card.suit = suit;
    card.rank = rank;
    card.isLocked = false;
    card.isGlitched = false;
    card.isRotted = false;
    card.rotTurns = 0;
    card.isHidden = false;
    card.isEphemeral = false;
    card.isDiscounted = false;
    return card;
}

int card_getEffectiveValue(const Card *card)
{
    return (int)card->rank * card_rotMultiplier(card);
}

int card_getChipValue(const Card *card)
{
    if (card->rank == RANK_ACE) return 11;
    return (int)card->rank;
}

int card_rotMultiplier(const Card *card)
{
    if (!card->isRotted) return 1;
    int turns = card->rotTurns < 1 ? 1 : card->rotTurns;
    if (turns > CARD_ROT_CAP_TURNS) turns = CARD_ROT_CAP_TURNS;
    return 1 << turns;
}

void card_markRotted(Card *card)
{
    if (card->isRotted) return;
    card->isGlitched = false;
    card->isRotted = true;
    card->rotTurns = 1;
}

void card_ageRot(Card *card)
{
    if (!card->isRotted) return;
    if (card->rotTurns < CARD_ROT_CAP_TURNS) card->rotTurns++;
}

void card_markGlitched(Card *card)
{
    if (card->isGlitched) return;
    card->isRotted = false;
    card->rotTurns = 0;
    card->isGlitched = true;
}

void card_clearRot(Card *card)
{
    card->isRotted = false;
    card->rotTurns = 0;
    card->isGlitched = false;
}

const char *card_suitName(Suit suit)
{
    switch (suit)
    {
        case SUIT_HEART:   return "hearts";
        case SUIT_DIAMOND: return "diamonds";
        case SUIT_CLUB:    return "clubs";
        case SUIT_SPADE:   return "spades";
        default:           return "unknown";
    }
}

const char *card_rankLabel(Rank rank)
{
    switch (rank)
    {
        case RANK_ACE:   return "A";
        case RANK_TWO:   return "02";
        case RANK_THREE: return "03";
        case RANK_FOUR:  return "04";
        case RANK_FIVE:  return "05";
        case RANK_SIX:   return "06";
        case RANK_SEVEN: return "07";
        case RANK_EIGHT: return "08";
        case RANK_NINE:  return "09";
        case RANK_TEN:   return "10";
        case RANK_JACK:  return "J";
        case RANK_QUEEN: return "Q";
        case RANK_KING:  return "K";
        default:         return "?";
    }
}

void card_getTexturePath(const Card *card, char *outPath, size_t outSize)
{
    snprintf(outPath, outSize, "assets/cards/card_%s_%s.png",
              card_suitName(card->suit), card_rankLabel(card->rank));
}
