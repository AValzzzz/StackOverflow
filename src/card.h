#ifndef CARD_H
#define CARD_H

#include <stdbool.h>
#include <stddef.h>

typedef enum Suit {
    SUIT_HEART,
    SUIT_DIAMOND,
    SUIT_CLUB,
    SUIT_SPADE,
    SUIT_COUNT
} Suit;

typedef enum Rank {
    RANK_TWO   = 2,
    RANK_THREE,
    RANK_FOUR,
    RANK_FIVE,
    RANK_SIX,
    RANK_SEVEN,
    RANK_EIGHT,
    RANK_NINE,
    RANK_TEN,
    RANK_JACK  = 11,
    RANK_QUEEN = 12,
    RANK_KING  = 13,
    RANK_ACE   = 1
} Rank;

typedef struct Card {
    Suit suit;
    Rank rank;
    bool isLocked;    
    bool aceAsEleven; 
    bool isSpecial;   
    bool isRotted;    
    bool isHidden;    
    bool isEphemeral; 
} Card;

Card card_make(Suit suit, Rank rank);
int  card_getEffectiveValue(const Card *card);

const char *card_suitName(Suit suit);  
const char *card_rankLabel(Rank rank);  

void card_getTexturePath(const Card *card, char *outPath, size_t outSize);

#endif
