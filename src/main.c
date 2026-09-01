#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#include "audio.h"
#include "background.h"
#include "card.h"
#include "card_textures.h"
#include "deck.h"
#include "fonts.h"
#include "hand.h"
#include "inventory.h"
#include "memory_grid.h"
#include "round.h"
#include "shop.h"
#include "tutorial.h"
#include "ui_textures.h"

static Font g_gameFont;
#define DrawText(text, x, y, size, color) \
    DrawTextEx(g_gameFont, (text), (Vector2){ (float)(x), (float)(y) }, (float)(size), 1.0f, (color))
#define MeasureText(text, size) \
    ((int)MeasureTextEx(g_gameFont, (text), (float)(size), 1.0f).x)

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

static float g_renderScale = 1.0f;
static Vector2 g_renderOffset = { 0.0f, 0.0f };

static Vector2 getVirtualMousePosition(void)
{
    return (Vector2){
        ((float)GetMouseX() - g_renderOffset.x) / g_renderScale,
        ((float)GetMouseY() - g_renderOffset.y) / g_renderScale
    };
}
#define GetMousePosition() getVirtualMousePosition()

static void updateRenderTransform(void)
{
    float winW = (float)GetScreenWidth(), winH = (float)GetScreenHeight();
    float scaleX = winW / SCREEN_WIDTH, scaleY = winH / SCREEN_HEIGHT;
    g_renderScale = scaleX < scaleY ? scaleX : scaleY;
    g_renderOffset.x = (winW - SCREEN_WIDTH * g_renderScale) / 2.0f;
    g_renderOffset.y = (winH - SCREEN_HEIGHT * g_renderScale) / 2.0f;
}

#define CARD_DISP_W 90
#define CARD_DISP_H 129
#define GRID_GAP    16
#define HAND_GAP    16
#define LOCK_BADGE  26

#define SLOT_W 130
#define SLOT_H 100
#define SLOT_GAP 20

#define RANK_PICK_W 58
#define RANK_PICK_H 83
#define RANK_PICK_GAP 6

#define MENU_BUTTON_W 260
#define MENU_BUTTON_H 56
#define MENU_BUTTON_GAP 18

#define SHOP_COLS 4
#define SHOP_BOX_W 290
#define SHOP_BOX_H 180
#define SHOP_BOX_GAP 20
#define SHOP_ORIGIN_Y 140

#define SHOP_OFFER_COUNT 4
#define SHOP_REROLL_BASE_COST 2

#define SHOP_CARD_OFFER_COUNT 3
#define SHOP_CARD_PRICE_CEIL 15
#define MAX_BOUGHT_CARDS 20

#define EPHEMERAL_INTERVAL_TURNS 4
#define EPHEMERAL_PLAY_WINDOW 2

#define EVENT_TRIGGER_CHANCE_PERCENT 35
#define EVENT_GOLD_REWARD 2

#define UNSTABLE_DECK_TRIGGER_PERCENT 20

#define BOSS_CLEAR_BONUS_GOLD 10

#define FULL_STACK_MIN_ROUND 15
#define FULL_STACK_BONUS 500

#define MODULE_CHOICE_EVERY_N_ROUNDS 3

#define DRAG_CLICK_THRESHOLD 8.0f

#define PILE_CARD_W 50
#define PILE_CARD_H 72
#define PILE_STACK_OFFSET 3.0f

#define MAX_FLYING_CARDS 36
#define FLY_DURATION 0.32f
#define FLY_IN_DELAY 0.15f

#define SCORE_POPUP_DURATION 1.1f

#define COMBO_FLASH_DURATION 0.5f

#define SHAKE_DURATION_CRASH 0.45f
#define SHAKE_MAGNITUDE_CRASH 16.0f
#define SHAKE_DURATION_COMBO 0.22f

#define MAX_COMBO_CASCADE_WAVES 20

#define CASCADE_WAVE_DELAY 0.55f

#define CASCADE_REVEAL_DELAY 0.85f

#define DECK_POPUP_COLS 13
#define DECK_POPUP_CARD_W 54
#define DECK_POPUP_CARD_H 78
#define DECK_POPUP_CARD_GAP 6
#define DECK_POPUP_ORIGIN_Y 110

static const Color COLOR_BG        = { 30, 30, 30, 255 };
static const Color COLOR_ACCENT    = { 78, 201, 176, 255 };
static const Color COLOR_SELECTED  = { 220, 90, 60, 255 };
static const Color COLOR_FREE_CELL = { 60, 60, 60, 255 };
static const Color COLOR_DANGER    = { 255, 0, 60, 255 };
static const Color COLOR_PROMPT    = { 240, 200, 60, 255 };
static const Color COLOR_GOLD      = { 240, 200, 60, 255 };
static const Color COLOR_SLOT_BG   = { 45, 45, 45, 255 };
static const Color COLOR_PANEL     = { 20, 20, 22, 235 };
static const Color COLOR_SPECIAL_TINT = { 150, 255, 190, 255 }; 
static const Color COLOR_ROTTED_TINT  = { 255, 130, 80, 255 };  
static const Color COLOR_L1_CACHE     = { 255, 215, 90, 255 };  
static const Color COLOR_COMBO_GREEN  = { 90, 220, 110, 255 }; 
static const Color COLOR_COMBO_ORANGE = { 230, 150, 60, 255 }; 
static const Color COLOR_GAUGE_BG     = { 55, 55, 55, 255 };
static const Color COLOR_BOSS         = { 200, 80, 220, 255 }; 
static const Color COLOR_MULT         = { 255, 90, 60, 255 }; 
static const Color COLOR_CHIPS        = { 255, 240, 210, 255 }; 

static const Color COLOR_FLASH_SAME_SUIT      = { 90, 180, 255, 255 };  
static const Color COLOR_FLASH_STRAIGHT       = { 90, 220, 110, 255 };  
static const Color COLOR_FLASH_BRELAN         = { 200, 120, 255, 255 }; 
static const Color COLOR_FLASH_STRAIGHT_FLUSH = { 255, 215, 90, 255 };  

typedef enum GamePhase {
    PHASE_MAIN_MENU,    
    PHASE_CLASS_SELECT, 
    PHASE_PLAYING,
    PHASE_SHOP,
    PHASE_EVENT,        
    PHASE_MODULE_CHOICE,
    PHASE_GAME_OVER
} GamePhase;

typedef enum GameOverReason {
    REASON_CRASH,
    REASON_QUOTA,
    REASON_TURN_LIMIT
} GameOverReason;

typedef enum BossType {
    BOSS_NONE,
    BOSS_BLOCKED_SECTOR,
    BOSS_EXPANDED_STACK,
    BOSS_TRAP_CELL,     
    BOSS_RESTRICTED_BOARD,
    BOSS_HIDDEN_CARDS,    
    BOSS_ROTTEN_DISCARD,  
    BOSS_EPHEMERAL_CARDS,
    BOSS_SCORE_THRESHOLD,
    BOSS_FORCED_DIAGONAL,
    BOSS_TYPE_COUNT
} BossType;

typedef enum StartingClass {
    CLASS_COMPILER,
    CLASS_BANKER,
    CLASS_ARCHITECT,
    CLASS_COUNT
} StartingClass;

typedef struct StartingClassInfo {
    const char *name;
    const char *description;
} StartingClassInfo;

static const StartingClassInfo CLASS_INFO[CLASS_COUNT] = {
    [CLASS_COMPILER]  = { "Compiler",  "Free Compiler Patch script +\n2 pre-glitched cards in the deck" },
    [CLASS_BANKER]    = { "Banker",    "Starts with Banker Chip:\nface cards score 0 toward the limit" },
    [CLASS_ARCHITECT] = { "Architect", "Starts with Redundant Color:\nHearts=Diamonds, Clubs=Spades\n(half points unless exact suit)" },
};

typedef enum InteractionMode {
    MODE_IDLE,
    MODE_AWAITING_SWAP_FIRST,
    MODE_AWAITING_SWAP_SECOND,
    MODE_AWAITING_FLIP_CHOICE,
    MODE_AWAITING_QUEEN_LOCK_FIRST,
    MODE_AWAITING_QUEEN_LOCK_SECOND,
    MODE_UNSTABLE_DECK_PICK,    
    MODE_WILDCARD_PICK_RANK,    
    MODE_WILDCARD_PICK_CELL,    
    MODE_MEMORY_FLUSH_PICK_CELL,
    MODE_COMPILER_PATCH_PICK_RANK, 
    MODE_NULL_POINTER_PICK_RANK    
} InteractionMode;

static const Rank WILDCARD_RANKS[13] = {
    RANK_ACE, RANK_TWO, RANK_THREE, RANK_FOUR, RANK_FIVE, RANK_SIX, RANK_SEVEN,
    RANK_EIGHT, RANK_NINE, RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING
};

typedef struct FlyingCard {
    bool active;
    Card card;
    Vector2 startPos, endPos;
    Vector2 startSize, endSize;
    float elapsed; 
} FlyingCard;

typedef struct ScorePopup {
    bool active;
    int chips;
    float elapsed;
} ScorePopup;

typedef struct CascadeState {
    bool active;
    int wave;
    float waveTimer; 
    bool refillMayCrash; 
    int chips;
    int totalMatches;
    bool anyCombo;
    bool revealing;       
    ComboType revealType;  
} CascadeState;

typedef enum TutStepKind { TUT_MSG, TUT_HAND, TUT_CELL, TUT_YES, TUT_WAIT } TutStepKind;

typedef enum TutHighlight { TUT_HL_NONE, TUT_HL_STACK, TUT_HL_GRID, TUT_HL_L1CACHE } TutHighlight;

typedef struct TutStep {
    TutStepKind kind;
    int a, b; 
    const char *line1;
    const char *line2; 
    TutHighlight hl;   
} TutStep;

static const TutStep TUTORIAL_SCRIPT[] = {
    { TUT_MSG, 0, 0, "Welcome to STACK OVERFLOW!",
                     "Let's play a short guided round together. Click anywhere to continue.", TUT_HL_NONE },
    { TUT_MSG, 0, 0, "This is your Memory Grid - always full, 3x3.",
                     "Playing a card overwrites whatever's already sitting in that cell.", TUT_HL_GRID },
    { TUT_MSG, 0, 0, "Your STACK SCORE (top-left) is the sum of every card's value on the grid.",
                     "Go over the STACK LIMIT and it's an instant game over - watch that gauge.", TUT_HL_STACK },
    { TUT_HAND, 0, 0, "Click the glowing card in your hand to select it.", NULL, TUT_HL_NONE },
    { TUT_CELL, 0, 0, "Now click the glowing cell to place it there.", NULL, TUT_HL_NONE },
    { TUT_CELL, 1, 0, "JACKS trigger SWAP: exchange two cards on the grid.",
                      "Click the glowing cell to pick the first card to swap.", TUT_HL_NONE },
    { TUT_CELL, 2, 1, "Click the glowing cell to pick the second card to swap with.", NULL, TUT_HL_NONE },
    { TUT_HAND, 1, 0, "Nicely done! Click your next card.", NULL, TUT_HL_NONE },
    { TUT_CELL, 2, 2, "Click the glowing cell to place it.", NULL, TUT_HL_NONE },
    { TUT_CELL, 1, 2, "QUEENS trigger ABSORB & LOCK: resets a neighbor cell's value to 0 and locks it.",
                      "Click the glowing cell to pick a neighbor to lock.", TUT_HL_NONE },
    { TUT_CELL, 2, 1, "Click the glowing cell to lock a second neighbor too.", NULL, TUT_HL_NONE },
    { TUT_HAND, 2, 0, "One more face card - click it.", NULL, TUT_HL_NONE },
    { TUT_CELL, 0, 1, "Click the glowing cell to place it.", NULL, TUT_HL_NONE },
    { TUT_YES, 0, 0, "KINGS trigger FLIP: switch whether rows/columns or diagonals score.",
                     "Diagonal combos score double. Click YES to switch to diagonal-only detection.", TUT_HL_NONE },
    { TUT_HAND, 3, 0, "Last card! Click it.", NULL, TUT_HL_NONE },
    { TUT_CELL, 2, 0, "ACES are smart: they pick 1 or 11, whichever helps you most.",
                      "Click the glowing cell to place it.", TUT_HL_NONE },
    { TUT_WAIT, 0, 0, "Watch closely...", NULL, TUT_HL_NONE },
    { TUT_MSG, 0, 0, "That's a SAME SUIT combo! 3 cells of one suit in a row, column, or diagonal.",
                     "A combo scores points AND refills those cells - which lowers your Stack Score.", TUT_HL_GRID },
    { TUT_MSG, 0, 0, "SAME SUIT is just one of 4 ways to match a line, worth 100+ pts.",
                     "STRAIGHT (3 consecutive ranks) is 250+ pts; N-OF-A-KIND (3 same rank) is 400+ pts.", TUT_HL_GRID },
    { TUT_MSG, 0, 0, "All 4 combo types score more when built from higher-value cards - riskier",
                     "since those cards also push your Stack Score up faster.", TUT_HL_GRID },
    { TUT_MSG, 0, 0, "STRAIGHT FLUSH (consecutive AND same suit) is worth 1000+ pts and wipes",
                     "the whole grid! STRAIGHT and N-OF-A-KIND also halve your whole Stack Score, once per turn.", TUT_HL_GRID },
    { TUT_MSG, 0, 0, "The center cell is worth 1.5x on any combo through it - see its gold border.",
                     NULL, TUT_HL_L1CACHE },
    { TUT_MSG, 0, 0, "That's the core loop: score enough each round, don't let the grid overflow.",
                     "Spend gold you earn in the shop between rounds. Click to start your real run.", TUT_HL_NONE },
};
#define TUTORIAL_STEP_COUNT (int)(sizeof(TUTORIAL_SCRIPT) / sizeof(TUTORIAL_SCRIPT[0]))

typedef struct Game {
    Deck deck;
    MemoryGrid grid;
    Hand hand;
    Inventory inventory;

    int selectedHandIndex;
    int roundScore;
    int gold;
    int roundNumber;
    RoundConfig roundCfg;

    int turnCounter; 

    int tempBlockRow, tempBlockCol;
    int tempBlockTurnsLeft;

    int turnsUntilNextEphemeralPick;
    int ephemeralTurnsLeft;

    StartingClass startingClass;
    BossType currentBossType;
    bool pendingEventCorruptCard; 

    GamePhase phase;
    GameOverReason gameOverReason;
    InteractionMode mode;

    int swapFirstRow, swapFirstCol;
    int pendingScriptSlot;  
    Rank wildcardRank;

    int queenRow, queenCol;             
    int queenLockFirstRow, queenLockFirstCol;

    int extraPlaysRemaining;
    bool deckPeekActive;   

    int unstableDeckSlot;
    Card unstableDeckOptionA, unstableDeckOptionB;

    bool pendingPowerResolution;
    Rank pendingPowerRank;
    int pendingPowerRow, pendingPowerCol;

    int moduleChoiceOffer[3];
    int moduleChoiceCount;

    bool hasUndoSnapshot;
    MemoryGrid undoGrid;
    Hand undoHand;
    Deck undoDeck;
    int undoRoundScore;
    int undoExtraPlays;
    int undoTurnCounter;

    char statusMessage[96];
    float statusMessageTimer;

    int shopOffer[SHOP_OFFER_COUNT]; 
    bool shopOfferSold[SHOP_OFFER_COUNT]; 
    int shopRerollCost;

    Card shopCardOffer[SHOP_CARD_OFFER_COUNT];
    bool shopCardOfferSold[SHOP_CARD_OFFER_COUNT];

    Card boughtCards[MAX_BOUGHT_CARDS];
    int  boughtCardCount;

    Card removedCards[MAX_BOUGHT_CARDS];
    int  removedCardCount;

    bool deckEditOpen;
    bool deckEditUpgradeMode;

    bool shopSwapPromptActive;
    bool shopSwapIsModule;
    int  shopSwapPendingItem; 
    int  shopSwapPendingOfferSlot;

    bool isDragging;
    int dragHandIndex;
    Vector2 dragStartPos;

    bool deckPopupOpen;

    bool isPaused;
    bool wantsQuit;

    float animSpeed;

    bool helpOverlayOpen; 

    bool tutorialActive; 
    int  tutorialStep;

    FlyingCard flyingCards[MAX_FLYING_CARDS];

    ScorePopup scorePopup;

    float shakeTimer;
    float shakeDuration;
    float shakeMagnitude;

    float comboFlashTimer;
    Color comboFlashColor;
    bool comboFlashCell[GRID_SIZE_MAX][GRID_SIZE_MAX];

    float handHoverLift[HAND_SIZE];

    CascadeState cascade; 
} Game;

static float gridCardWidth(int size)  { return size <= GRID_SIZE_MIN ? CARD_DISP_W : 62.0f; }
static float gridCardHeight(int size) { return size <= GRID_SIZE_MIN ? CARD_DISP_H : 89.0f; }
static float gridCardGap(int size)    { return size <= GRID_SIZE_MIN ? GRID_GAP : 10.0f; }

static Rectangle gridCellRect(int row, int col, int size)
{
    float w = gridCardWidth(size), h = gridCardHeight(size), gap = gridCardGap(size);
    float gridW = size * w + (size - 1) * gap;
    float gridX = (SCREEN_WIDTH - gridW) / 2.0f;
    float gridY = 130;
    return (Rectangle){
        gridX + col * (w + gap),
        gridY + row * (h + gap),
        w, h
    };
}

static Rectangle gridBoundsRect(int size)
{
    Rectangle topLeft = gridCellRect(0, 0, size);
    Rectangle bottomRight = gridCellRect(size - 1, size - 1, size);
    float pad = 10;
    return (Rectangle){
        topLeft.x - pad, topLeft.y - pad,
        (bottomRight.x + bottomRight.width) - topLeft.x + 2 * pad,
        (bottomRight.y + bottomRight.height) - topLeft.y + 2 * pad
    };
}

static Rectangle handSlotRect(int index, int capacity)
{
    float handW = capacity * CARD_DISP_W + (capacity - 1) * HAND_GAP;
    float handX = (SCREEN_WIDTH - handW) / 2.0f;
    float handY = SCREEN_HEIGHT - CARD_DISP_H - 40;
    return (Rectangle){ handX + index * (CARD_DISP_W + HAND_GAP), handY, CARD_DISP_W, CARD_DISP_H };
}

static Rectangle deckStackRect(void)
{
    return (Rectangle){ SCREEN_WIDTH - 90, 55, PILE_CARD_W, PILE_CARD_H };
}

static Rectangle discardStackRect(void)
{
    return (Rectangle){ SCREEN_WIDTH - 170, 55, PILE_CARD_W, PILE_CARD_H };
}

static Rectangle moduleSlotRect(int index, int gridSize)
{
    float gridW = gridSize * gridCardWidth(gridSize) + (gridSize - 1) * gridCardGap(gridSize);
    float gridX = (SCREEN_WIDTH - gridW) / 2.0f;
    float x = gridX - SLOT_GAP - SLOT_W;
    float y = 130 + index * (SLOT_H + SLOT_GAP);
    return (Rectangle){ x, y, SLOT_W, SLOT_H };
}

static Rectangle scriptSlotRect(int index, int gridSize)
{
    float gridW = gridSize * gridCardWidth(gridSize) + (gridSize - 1) * gridCardGap(gridSize);
    float gridX = (SCREEN_WIDTH - gridW) / 2.0f;
    float x = gridX + gridW + SLOT_GAP;
    float y = 130 + index * (SLOT_H + SLOT_GAP);
    return (Rectangle){ x, y, SLOT_W, SLOT_H };
}

static Rectangle rankPickRect(int index)
{
    float w = 13 * RANK_PICK_W + 12 * RANK_PICK_GAP;
    float x = (SCREEN_WIDTH - w) / 2.0f;
    float y = 300;
    return (Rectangle){ x + index * (RANK_PICK_W + RANK_PICK_GAP), y, RANK_PICK_W, RANK_PICK_H };
}

static Rectangle unstablePickRect(int index) 
{
    float gap = 40;
    float totalW = 2 * CARD_DISP_W + gap;
    float x = (SCREEN_WIDTH - totalW) / 2.0f + index * (CARD_DISP_W + gap);
    float y = 300;
    return (Rectangle){ x, y, CARD_DISP_W, CARD_DISP_H };
}

static Rectangle deckPopupCardRect(int index)
{
    int col = index % DECK_POPUP_COLS;
    int row = index / DECK_POPUP_COLS;
    float totalW = DECK_POPUP_COLS * (DECK_POPUP_CARD_W + DECK_POPUP_CARD_GAP) - DECK_POPUP_CARD_GAP;
    float startX = (SCREEN_WIDTH - totalW) / 2.0f;
    return (Rectangle){
        startX + col * (DECK_POPUP_CARD_W + DECK_POPUP_CARD_GAP),
        DECK_POPUP_ORIGIN_Y + row * (DECK_POPUP_CARD_H + DECK_POPUP_CARD_GAP),
        DECK_POPUP_CARD_W, DECK_POPUP_CARD_H
    };
}

static int rankSortIndex(Rank rank)
{
    for (int i = 0; i < 13; i++)
        if (WILDCARD_RANKS[i] == rank) return i;
    return 0;
}

static Rectangle shopItemRect(int index)
{
    float totalW = SHOP_COLS * SHOP_BOX_W + (SHOP_COLS - 1) * SHOP_BOX_GAP;
    float originX = (SCREEN_WIDTH - totalW) / 2.0f;
    int col = index % SHOP_COLS, row = index / SHOP_COLS;
    return (Rectangle){
        originX + col * (SHOP_BOX_W + SHOP_BOX_GAP),
        SHOP_ORIGIN_Y + row * (SHOP_BOX_H + SHOP_BOX_GAP),
        SHOP_BOX_W, SHOP_BOX_H
    };
}

#define SHOP_CARD_ORIGIN_Y (SHOP_ORIGIN_Y + SHOP_BOX_H + 40)
static Rectangle shopCardOfferRect(int index)
{
    float totalW = SHOP_CARD_OFFER_COUNT * CARD_DISP_W + (SHOP_CARD_OFFER_COUNT - 1) * SHOP_BOX_GAP;
    float originX = (SCREEN_WIDTH - totalW) / 2.0f;
    return (Rectangle){ originX + index * (CARD_DISP_W + SHOP_BOX_GAP), SHOP_CARD_ORIGIN_Y, CARD_DISP_W, CARD_DISP_H };
}

static Rectangle shopOwnedModuleRect(int index) { return (Rectangle){ 195 + index * 220, 540, 205, 24 }; }
static Rectangle shopOwnedScriptRect(int index) { return (Rectangle){ 195 + index * 220, 565, 205, 24 }; }

static Rectangle swapPromptSlotRect(int index)
{
    float w = 520, h = 74;
    return (Rectangle){ SCREEN_WIDTH / 2.0f - w / 2.0f, 250 + index * (h + 14), w, h };
}
static Rectangle swapPromptCancelRect(void)
{
    return (Rectangle){ SCREEN_WIDTH / 2.0f - 100, 470, 200, 46 };
}

static Rectangle flipChoiceRect(int index)
{
    float w = 100, h = 34, gap = 16;
    float x = SCREEN_WIDTH / 2.0f - (2 * w + gap) / 2.0f + index * (w + gap);
    return (Rectangle){ x, 112, w, h };
}

static int sellRefund(ShopItemId id)
{
    int cost = shop_getItemInfo(id)->cost;
    int refund = cost / 2;
    return refund < 1 ? 1 : refund;
}

static void setModuleGridFlag(Game *g, ShopItemId id, bool active)
{
    if (id == ITEM_REDUNDANT_COLOR) memorygrid_setRedundantColorActive(&g->grid, active);
    else if (id == ITEM_BANKER_CHIP) memorygrid_setBankerChipActive(&g->grid, active);
    else if (id == ITEM_SEGFAULT_HANDLER) memorygrid_setSegfaultHandlerActive(&g->grid, active);
    else if (id == ITEM_CACHE_BOOST) memorygrid_setCacheBoostActive(&g->grid, active);
    else if (id == ITEM_LOOP_UNROLL) memorygrid_setFaceValueBoostActive(&g->grid, active);
}

static bool deckEditIsRemoved(const Game *g, Suit suit, Rank rank)
{
    for (int i = 0; i < g->removedCardCount; i++)
        if (g->removedCards[i].suit == suit && g->removedCards[i].rank == rank) return true;
    return false;
}

static int buildFullDeckComposition(const Game *g, Card out[DECK_MAX_SIZE])
{
    int count = 0;
    for (Suit suit = 0; suit < SUIT_COUNT; suit++)
    {
        for (Rank rank = RANK_TWO; rank <= RANK_KING; rank++)
            if (!deckEditIsRemoved(g, suit, rank)) out[count++] = card_make(suit, rank);
        if (!deckEditIsRemoved(g, suit, RANK_ACE)) out[count++] = card_make(suit, RANK_ACE);
    }
    for (int i = 0; i < g->boughtCardCount; i++) out[count++] = g->boughtCards[i];
    return count;
}

static Rank upgradeRank(Rank rank)
{
    int idx = rankSortIndex(rank);
    return idx < 12 ? WILDCARD_RANKS[idx + 1] : rank;
}

static int deckEditCost(const Game *g)
{
    return (g->deckEditUpgradeMode ? 5 : 4) + g->roundNumber / 3;
}

static void deckEditRemoveCard(Game *g, Card card)
{
    for (int i = 0; i < g->boughtCardCount; i++)
        if (g->boughtCards[i].suit == card.suit && g->boughtCards[i].rank == card.rank)
        {
            for (int j = i; j < g->boughtCardCount - 1; j++) g->boughtCards[j] = g->boughtCards[j + 1];
            g->boughtCardCount--;
            return;
        }
    if (g->removedCardCount < MAX_BOUGHT_CARDS)
        g->removedCards[g->removedCardCount++] = card;
}

static Rectangle menuButtonRect(int index, float topY)
{
    float x = (SCREEN_WIDTH - MENU_BUTTON_W) / 2.0f;
    return (Rectangle){ x, topY + index * (MENU_BUTTON_H + MENU_BUTTON_GAP), MENU_BUTTON_W, MENU_BUTTON_H };
}
static Rectangle mainMenuButtonRect(int index) { return menuButtonRect(index, 420.0f); } 
static Rectangle pauseButtonRect(int index)    { return menuButtonRect(index, 300.0f); } 

static void drawCard(const Card *card, Rectangle dest)
{
    Texture2D tex = card->isHidden ? cardtex_getBack() : cardtex_get(card->suit, card->rank);
    Color tint = card->isRotted ? COLOR_ROTTED_TINT : (card->isSpecial ? COLOR_SPECIAL_TINT : WHITE);
    DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                    dest, (Vector2){ 0, 0 }, 0.0f, tint);
}

static void drawMenuButton(Rectangle rect, const char *label, Color fillColor)
{
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, hovered ? Fade(fillColor, 0.85f) : Fade(fillColor, 0.55f));
    DrawRectangleLinesEx(rect, 2, fillColor);
    int textSize = 20;
    int w = MeasureText(label, textSize);
    DrawText(label, (int)(rect.x + rect.width / 2.0f - w / 2.0f), (int)(rect.y + rect.height / 2.0f - textSize / 2.0f),
              textSize, RAYWHITE);
}

static void drawButton(Rectangle rect, const char *label, Color color, bool enabled, bool filled)
{
    bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), rect);
    Color c = enabled ? color : Fade(color, 0.35f);
    int textSize = 18;
    int w = MeasureText(label, textSize);
    float tx = rect.x + rect.width / 2.0f - w / 2.0f;
    float ty = rect.y + rect.height / 2.0f - textSize / 2.0f;
    if (filled)
    {
        DrawRectangleRec(rect, hovered ? c : Fade(c, 0.85f));
        if (hovered) DrawRectangleLinesEx(rect, 2, RAYWHITE);
        Color textColor = enabled ? (Color){ 20, 20, 20, 255 } : (Color){ 70, 70, 70, 255 };
        DrawText(label, (int)tx, (int)ty, textSize, textColor);
    }
    else
    {
        DrawRectangleRec(rect, hovered ? Fade(c, 0.18f) : COLOR_SLOT_BG);
        DrawRectangleLinesEx(rect, hovered ? 3 : 2, c);
        DrawText(label, (int)tx, (int)ty, textSize, c);
    }
}

static void drawGlow(Rectangle rect, Color color, float intensity)
{
    BeginBlendMode(BLEND_ADDITIVE);
    const int LAYERS = 5;
    for (int i = LAYERS; i >= 1; i--)
    {
        float grow = i * 7.0f;
        Rectangle layer = { rect.x - grow, rect.y - grow, rect.width + 2 * grow, rect.height + 2 * grow };
        float alphaT = (1.0f - (float)i / (float)LAYERS) * intensity;
        Color layerColor = Fade(color, alphaT * 0.5f);
        DrawRectangleRounded(layer, 0.35f, 12, layerColor);
    }
    EndBlendMode();
}

static void drawTextCentered(const char *text, float centerX, float y, int fontSize, Color color)
{
    int w = MeasureText(text, fontSize);
    DrawText(text, (int)(centerX - w / 2.0f), (int)y, fontSize, color);
}

static float drawTextWrapped(const char *text, float x, float y, float maxWidth, int fontSize, float lineHeight, Color color)
{
    char line[512] = { 0 };
    float cursorY = y;
    const char *wordStart = text;

    while (*wordStart)
    {
        const char *wordEnd = wordStart;
        while (*wordEnd && *wordEnd != ' ') wordEnd++;
        int wordLen = (int)(wordEnd - wordStart);
        if (wordLen > 200) wordLen = 200;

        char candidate[512];
        int lineLen = (int)strlen(line);
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-truncation"
        if (lineLen > 0) snprintf(candidate, sizeof(candidate), "%s %.*s", line, wordLen, wordStart);
        else snprintf(candidate, sizeof(candidate), "%.*s", wordLen, wordStart);
        #pragma GCC diagnostic pop

        if (lineLen > 0 && MeasureText(candidate, fontSize) > (int)maxWidth)
        {
            DrawText(line, (int)x, (int)cursorY, fontSize, color);
            cursorY += lineHeight;
            snprintf(line, sizeof(line), "%.*s", wordLen, wordStart);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", candidate);
        }

        wordStart = wordEnd;
        while (*wordStart == ' ') wordStart++;
    }
    if (line[0] != '\0')
    {
        DrawText(line, (int)x, (int)cursorY, fontSize, color);
        cursorY += lineHeight;
    }
    return cursorY;
}

static void drawCardBack(Rectangle dest)
{
    Texture2D tex = cardtex_getBack();
    DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height }, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

static void drawPile(Rectangle topRect, int count, bool showFaceOnTop, Card faceCard)
{
    int visibleCards = count > 0 ? (count < 3 ? count : 3) : 0;
    for (int i = 0; i < visibleCards; i++)
    {
        Rectangle r = { topRect.x - i * PILE_STACK_OFFSET, topRect.y - i * PILE_STACK_OFFSET, topRect.width, topRect.height };
        if (i == 0 && showFaceOnTop) drawCard(&faceCard, r);
        else drawCardBack(r);
    }
    if (count == 0) DrawRectangleLinesEx(topRect, 1, COLOR_FREE_CELL);
    drawTextCentered(TextFormat("%d", count), topRect.x + topRect.width / 2.0f, topRect.y + topRect.height + 6, 14, LIGHTGRAY);
}

static void drawComboLegendRow(float x, float y, Suit suits[3], Rank ranks[3], const char *label, Color labelColor)
{
    const float w = 24.0f, h = 34.0f, gap = 4.0f;
    for (int i = 0; i < 3; i++)
    {
        Card c = card_make(suits[i], ranks[i]);
        drawCard(&c, (Rectangle){ x + i * (w + gap), y, w, h });
    }
    DrawText(label, (int)(x + 3 * (w + gap) + 6), (int)(y + h / 2.0f - 8), 14, labelColor);
}

static void drawComboLegend(float x, float y)
{
    Suit sameSuits[3]  = { SUIT_HEART, SUIT_HEART, SUIT_HEART };
    Rank sameRanks[3]  = { RANK_THREE, RANK_SEVEN, RANK_JACK };
    Suit straightSuits[3] = { SUIT_HEART, SUIT_CLUB, SUIT_SPADE };
    Rank straightRanks[3] = { RANK_FOUR, RANK_FIVE, RANK_SIX };
    Suit brelanSuits[3] = { SUIT_HEART, SUIT_CLUB, SUIT_SPADE };
    Rank brelanRanks[3] = { RANK_SEVEN, RANK_SEVEN, RANK_SEVEN };
    Suit flushSuits[3]  = { SUIT_DIAMOND, SUIT_DIAMOND, SUIT_DIAMOND };
    Rank flushRanks[3]  = { RANK_EIGHT, RANK_NINE, RANK_TEN };

    float rowH = 40.0f;
    drawComboLegendRow(x, y,               sameSuits,     sameRanks,     "SAME SUIT: 100+ pts",         COLOR_FLASH_SAME_SUIT);
    drawComboLegendRow(x, y + rowH,        straightSuits, straightRanks, "STRAIGHT: 250+ pts (scales)", COLOR_FLASH_STRAIGHT);
    drawComboLegendRow(x, y + rowH * 2,    brelanSuits,   brelanRanks,   "N-OF-A-KIND: 400+ pts (scales)", COLOR_FLASH_BRELAN);
    drawComboLegendRow(x, y + rowH * 3,    flushSuits,    flushRanks,    "STRAIGHT FLUSH: 1000+ pts", COLOR_FLASH_STRAIGHT_FLUSH);
}

static float easeOutBack(float t)
{
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    t -= 1.0f;
    return 1.0f + c3 * t * t * t + c1 * t * t;
}

static float easeOutCubic(float t)
{
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

static bool cardVisuallyDiffers(Card a, Card b)
{
    return a.suit != b.suit || a.rank != b.rank || a.isSpecial != b.isSpecial;
}

static Vector2 rectCenter(Rectangle r) { return (Vector2){ r.x + r.width / 2.0f, r.y + r.height / 2.0f }; }

static Rectangle rectFromCenter(Vector2 center, Vector2 size)
{
    return (Rectangle){ center.x - size.x / 2.0f, center.y - size.y / 2.0f, size.x, size.y };
}

static void spawnFlyingCard(Game *g, Card card, Rectangle from, Rectangle to, float delay)
{
    for (int i = 0; i < MAX_FLYING_CARDS; i++)
    {
        if (g->flyingCards[i].active) continue;
        g->flyingCards[i].active = true;
        g->flyingCards[i].card = card;
        g->flyingCards[i].startPos = (Vector2){ from.x, from.y };
        g->flyingCards[i].endPos = (Vector2){ to.x, to.y };
        g->flyingCards[i].startSize = (Vector2){ from.width, from.height };
        g->flyingCards[i].endSize = (Vector2){ to.width, to.height };
        g->flyingCards[i].elapsed = -delay;
        return;
    }
}

static void spawnCellReplaceAnimation(Game *g, Rectangle cellRect, Card oldCard, Card newCard)
{
    spawnFlyingCard(g, oldCard, cellRect, rectFromCenter(rectCenter(discardStackRect()), (Vector2){ PILE_CARD_W, PILE_CARD_H }), 0.0f);
    spawnFlyingCard(g, newCard, rectFromCenter(rectCenter(deckStackRect()), (Vector2){ PILE_CARD_W, PILE_CARD_H }), cellRect, FLY_IN_DELAY);
}

static void spawnGridDiffAnimations(Game *g, const MemoryGrid *before)
{
    for (int row = 0; row < g->grid.size; row++)
        for (int col = 0; col < g->grid.size; col++)
            if (cardVisuallyDiffers(before->cards[row][col], g->grid.cards[row][col]))
                spawnCellReplaceAnimation(g, gridCellRect(row, col, g->grid.size),
                                            before->cards[row][col], g->grid.cards[row][col]);
}

static const char *comboTypeName(ComboType type)
{
    switch (type)
    {
        case COMBO_SAME_SUIT:      return "SAME SUIT";
        case COMBO_STRAIGHT:       return "STRAIGHT";
        case COMBO_BRELAN:         return "N-OF-A-KIND";
        case COMBO_STRAIGHT_FLUSH: return "STRAIGHT FLUSH";
        default:                   return "NONE";
    }
}

static const char *bossHudMessage(const Game *g)
{
    switch (g->currentBossType)
    {
        case BOSS_BLOCKED_SECTOR:   return "BOSS: Blocked Sector - center cell locked all round";
        case BOSS_EXPANDED_STACK:   return "BOSS: Expanded Stack - 4x4 grid, need 4 in a row";
        case BOSS_TRAP_CELL:
            return TextFormat("BOSS: Trap Cell (%d,%d) - line through it scores 0",
                                g->grid.trapRow + 1, g->grid.trapCol + 1);
        case BOSS_RESTRICTED_BOARD:
        {
            const char *axis = g->grid.bannedAxis == BANNED_AXIS_ROWS ? "rows" :
                                 g->grid.bannedAxis == BANNED_AXIS_COLS ? "columns" : "diagonals";
            return TextFormat("BOSS: Restricted Board - %s don't score", axis);
        }
        case BOSS_HIDDEN_CARDS:     return "BOSS: Hidden Cards - some cards face-down";
        case BOSS_ROTTEN_DISCARD:   return "BOSS: Rotten Discard - rot blocks a cell";
        case BOSS_EPHEMERAL_CARDS:  return "BOSS: Ephemeral - play marked card in time";
        case BOSS_SCORE_THRESHOLD:  return "BOSS: Score Threshold - weak lines pay less";
        case BOSS_FORCED_DIAGONAL: return "BOSS: Forced Diagonal - only diagonals score, King can't flip";
        default:                    return "";
    }
}

static void setStatus(Game *g, const char *message)
{
    strncpy(g->statusMessage, message, sizeof(g->statusMessage) - 1);
    g->statusMessage[sizeof(g->statusMessage) - 1] = '\0';
    g->statusMessageTimer = 2.5f;
}

#define RESHUFFLE_ANIM_CARDS 6

static void spawnReshuffleAnimation(Game *g)
{
    int n = g->deck.discardCount < RESHUFFLE_ANIM_CARDS ? g->deck.discardCount : RESHUFFLE_ANIM_CARDS;
    for (int i = 0; i < n; i++)
        spawnFlyingCard(g, g->deck.discardPile[i],
                          rectFromCenter(rectCenter(discardStackRect()), (Vector2){ PILE_CARD_W, PILE_CARD_H }),
                          rectFromCenter(rectCenter(deckStackRect()), (Vector2){ PILE_CARD_W, PILE_CARD_H }),
                          i * 0.05f);
    audio_playShuffle();
    setStatus(g, "Deck exhausted - reshuffling the discard pile back in");
}

static void triggerShake(Game *g, float duration, float magnitude)
{
    if (g->shakeTimer > 0.0f && magnitude < g->shakeMagnitude) return;
    g->shakeTimer = duration;
    g->shakeDuration = duration;
    g->shakeMagnitude = magnitude;
}

static Color comboFlashColorFor(const ComboResult *combo)
{
    if (combo->straightFlushMatches > 0) return COLOR_FLASH_STRAIGHT_FLUSH;
    if (combo->brelanMatches > 0)        return COLOR_FLASH_BRELAN;
    if (combo->straightMatches > 0)      return COLOR_FLASH_STRAIGHT;
    return COLOR_FLASH_SAME_SUIT;
}

static bool classLineContainsCell(const LineClassification *line, int row, int col)
{
    for (int i = 0; i < line->length; i++)
        if (line->cells[i][0] == row && line->cells[i][1] == col) return true;
    return false;
}

typedef struct PlacementPreview {
    int scoreBefore;
    int scoreAfter;
    bool wouldCrash;
    bool isAce;
    bool aceAsEleven;
    LineClassification lines[LINE_COUNT_MAX];
    int lineCount;
    int comboPointsThroughTarget;
} PlacementPreview;

static PlacementPreview computePlacementPreview(const Game *g, Card card, int row, int col)
{
    PlacementPreview preview = { 0 };
    preview.scoreBefore = g->grid.stackScore;

    MemoryGrid scratch = g->grid;
    memorygrid_placeCard(&scratch, row, col, card);
    memorygrid_resolveAceValues(&scratch, g->roundCfg.stackLimit);

    preview.scoreAfter = scratch.stackScore;
    preview.wouldCrash = scratch.stackScore > g->roundCfg.stackLimit;
    preview.isAce = (card.rank == RANK_ACE);
    preview.aceAsEleven = scratch.cards[row][col].aceAsEleven;
    preview.lineCount = memorygrid_classifyAllLines(&scratch, preview.lines);

    for (int l = 0; l < preview.lineCount; l++)
    {
        if (!preview.lines[l].active || preview.lines[l].type == COMBO_NONE) continue;
        if (!classLineContainsCell(&preview.lines[l], row, col)) continue;
        int pts = memorygrid_comboBasePoints(preview.lines[l].type);
        if (scratch.size == 3 && classLineContainsCell(&preview.lines[l], L1_CACHE_ROW, L1_CACHE_COL))
            pts = (int)(pts * 1.5f);

        if (scratch.trapRow >= 0 && classLineContainsCell(&preview.lines[l], scratch.trapRow, scratch.trapCol))
            pts = 0;
        else if (scratch.scoreThresholdActive)
        {
            int strongCount = 0;
            for (int i = 0; i < preview.lines[l].length; i++)
            {
                Card c = scratch.cards[preview.lines[l].cells[i][0]][preview.lines[l].cells[i][1]];
                if (card_getEffectiveValue(&c) > 3) strongCount++;
            }
            pts = pts * strongCount / preview.lines[l].length;
        }
        preview.comboPointsThroughTarget += pts;
    }
    return preview;
}

static void enterShop(Game *g);
static void tickEphemeralCard(Game *g);

static void applyGarbageCollector(Game *g, const ComboResult *combo)
{
    if (combo->totalScoreGained <= 0 || combo->gridWasWiped) return;
    if (!inventory_hasModule(&g->inventory, ITEM_GARBAGE_COLLECTOR)) return;

    int bestRow = -1, bestCol = -1, bestValue = 0;
    for (int row = 0; row < g->grid.size; row++)
        for (int col = 0; col < g->grid.size; col++)
        {
            if (combo->cellInvolved[row][col]) continue;
            if (g->grid.cards[row][col].isLocked) continue;
            int value = card_getEffectiveValue(&g->grid.cards[row][col]);
            if (bestRow == -1 || value < bestValue) { bestRow = row; bestCol = col; bestValue = value; }
        }
    if (bestRow == -1) return;

    Card oldCard = g->grid.cards[bestRow][bestCol];
    memorygrid_memoryFlush(&g->grid, &g->deck, bestRow, bestCol);
    if (cardVisuallyDiffers(oldCard, g->grid.cards[bestRow][bestCol]))
        spawnCellReplaceAnimation(g, gridCellRect(bestRow, bestCol, g->grid.size), oldCard, g->grid.cards[bestRow][bestCol]);
    setStatus(g, "GARBAGE COLLECTOR: freed the lowest tile");
}

static void refillHandStep(Game *g)
{
    while (true)
    {
        bool anyEmpty = false;
        for (int i = 0; i < g->hand.capacity; i++)
            if (!g->hand.occupied[i]) { anyEmpty = true; break; }
        if (!anyEmpty || deck_isEmpty(&g->deck)) return;

        if (g->deck.count <= 0 && g->deck.discardCount > 0)
        {
            spawnReshuffleAnimation(g);
            deck_reshuffleDiscardIntoDeck(&g->deck);
        }

        if (g->roundCfg.unstableDeckActive && g->deck.count >= 2 &&
            GetRandomValue(1, 100) <= UNSTABLE_DECK_TRIGGER_PERCENT)
        {
            int slot = -1;
            for (int i = 0; i < g->hand.capacity; i++)
                if (!g->hand.occupied[i]) { slot = i; break; }
            g->unstableDeckSlot = slot;
            g->unstableDeckOptionA = deck_drawCard(&g->deck);
            g->unstableDeckOptionB = deck_drawCard(&g->deck);
            g->mode = MODE_UNSTABLE_DECK_PICK;
            return;
        }

        int filledSlot = hand_fillOneSlot(&g->hand, &g->deck);
        if (filledSlot < 0) return;

        if (g->currentBossType == BOSS_HIDDEN_CARDS)
        {
            int hiddenCount = 0;
            for (int i = 0; i < g->hand.capacity; i++)
                if (g->hand.occupied[i] && g->hand.cards[i].isHidden) hiddenCount++;
            if (hiddenCount < 2 && GetRandomValue(1, 100) <= 50)
                g->hand.cards[filledSlot].isHidden = true;
        }
    }
}

static void triggerCrash(Game *g)
{
    g->phase = PHASE_GAME_OVER;
    g->gameOverReason = REASON_CRASH;
    triggerShake(g, SHAKE_DURATION_CRASH, SHAKE_MAGNITUDE_CRASH);
    audio_playCrash();
}

static void resolveGridChange(Game *g)
{
    g->cascade.active = true;
    g->cascade.wave = 0;
    g->cascade.waveTimer = 0.0f; 
    g->cascade.refillMayCrash = g->grid.stackScore <= g->roundCfg.stackLimit;
    g->cascade.chips = 0;
    g->cascade.totalMatches = 0;
    g->cascade.anyCombo = false;
    g->cascade.revealing = false;
}

static void finishCascade(Game *g)
{
    int chips = g->cascade.chips;
    int totalMatches = g->cascade.totalMatches;
    bool anyCombo = g->cascade.anyCombo;
    g->cascade.active = false;

    int gained = chips;
    if (inventory_hasModule(&g->inventory, ITEM_OVERCLOCK)) gained = (int)(gained * 1.2f);

    if (g->roundNumber >= FULL_STACK_MIN_ROUND && totalMatches >= 2)
    {
        gained += FULL_STACK_BONUS;
        setStatus(g, TextFormat("FULL STACK! +%d bonus (2+ combos at once)", FULL_STACK_BONUS));
    }
    g->roundScore += gained;

    if (anyCombo)
    {
        g->scorePopup.active = true;
        g->scorePopup.chips = chips;
        g->scorePopup.elapsed = 0.0f;

        if (totalMatches >= 2)
            triggerShake(g, SHAKE_DURATION_COMBO, 5.0f + fminf((float)totalMatches, 6.0f) * 2.0f);
    }

    if (g->roundScore >= g->roundCfg.objective)
    {
        int bonus = round_goldBonus(g->deck.count);
        int bossBonus = (g->currentBossType != BOSS_NONE) ? BOSS_CLEAR_BONUS_GOLD : 0;
        g->gold += g->roundCfg.goldReward + bonus + bossBonus;
        if (bossBonus > 0) setStatus(g, TextFormat("BOSS CLEARED! +$%d bonus gold", bossBonus));
        audio_playRoundClear();
        enterShop(g);
        g->phase = PHASE_SHOP;
        return;
    }

    if (g->grid.stackScore > g->roundCfg.stackLimit)
    {
        if (g->inventory.tryCatchChargeAvailable)
        {
            g->grid.stackScore = g->roundCfg.stackLimit;
            g->inventory.tryCatchChargeAvailable = false;
        }
        else
        {
            triggerCrash(g);
            return;
        }
    }

    if (deck_isEmpty(&g->deck) && g->hand.count == 0)
    {
        g->phase = PHASE_GAME_OVER;
        g->gameOverReason = REASON_QUOTA;
        return;
    }

    if (g->turnCounter >= g->roundCfg.turnLimit)
    {
        g->phase = PHASE_GAME_OVER;
        g->gameOverReason = REASON_TURN_LIMIT;
    }
}

static bool revealCascadeWave(Game *g)
{
    LineClassification lines[LINE_COUNT_MAX];
    int lineCount = memorygrid_classifyAllLines(&g->grid, lines);

    bool matchCells[GRID_SIZE_MAX][GRID_SIZE_MAX] = { 0 };
    bool anyMatch = false;
    int sameSuitN = 0, straightN = 0, brelanN = 0, sfN = 0;
    for (int l = 0; l < lineCount; l++)
    {
        if (!lines[l].active || lines[l].type == COMBO_NONE) continue;
        anyMatch = true;
        for (int i = 0; i < lines[l].length; i++)
            matchCells[lines[l].cells[i][0]][lines[l].cells[i][1]] = true;
        switch (lines[l].type)
        {
            case COMBO_SAME_SUIT:      sameSuitN++; break;
            case COMBO_STRAIGHT:       straightN++; break;
            case COMBO_BRELAN:         brelanN++; break;
            case COMBO_STRAIGHT_FLUSH: sfN++;      break;
            default: break;
        }
    }
    if (!anyMatch) return false;

    g->cascade.revealType = sfN > 0 ? COMBO_STRAIGHT_FLUSH
                            : brelanN > 0 ? COMBO_BRELAN
                            : straightN > 0 ? COMBO_STRAIGHT
                            : COMBO_SAME_SUIT;

    memcpy(g->comboFlashCell, matchCells, sizeof(g->comboFlashCell));
    g->comboFlashColor = comboFlashColorFor(&(ComboResult){
        .sameSuitMatches = sameSuitN, .straightMatches = straightN,
        .brelanMatches = brelanN, .straightFlushMatches = sfN });
    g->comboFlashTimer = CASCADE_REVEAL_DELAY + COMBO_FLASH_DURATION;
    audio_playCombo(1.0f + fminf((float)g->cascade.wave, 4.0f) * 0.06f);
    return true;
}

static void applyCascadeWave(Game *g)
{
    MemoryGrid beforeWave = g->grid;
    ComboResult combo = memorygrid_resolveAlignments(&g->grid, &g->deck, g->roundCfg.stackLimit);
    int waveMatches = combo.sameSuitMatches + combo.straightMatches + combo.brelanMatches + combo.straightFlushMatches;

    g->cascade.anyCombo = true;
    g->cascade.totalMatches += waveMatches;
    g->cascade.chips += combo.totalScoreGained;
    g->cascade.wave++;
    g->cascade.revealing = false;

    applyGarbageCollector(g, &combo);
    memorygrid_resolveAceValues(&g->grid, g->roundCfg.stackLimit);

    if (g->cascade.refillMayCrash && g->grid.stackScore > g->roundCfg.stackLimit)
    {
        memorygrid_ensureFairDeal(&g->grid, &g->deck, g->roundCfg.stackLimit, false, FAIR_DEAL_MAX_ATTEMPTS);
        if (g->grid.stackScore <= g->roundCfg.stackLimit)
            setStatus(g, "STACK GUARD: reshuffled a refill to prevent an unfair crash");
    }

    spawnGridDiffAnimations(g, &beforeWave);

    if (deck_isEmpty(&g->deck)) { finishCascade(g); return; } 
    g->cascade.waveTimer = CASCADE_WAVE_DELAY; 
}

static void updateCascade(Game *g, float dt)
{
    if (!g->cascade.active) return;

    g->cascade.waveTimer -= dt;
    if (g->cascade.waveTimer > 0.0f) return;

    if (g->cascade.revealing) { applyCascadeWave(g); return; }

    if (g->cascade.wave >= MAX_COMBO_CASCADE_WAVES) { finishCascade(g); return; }

    if (!revealCascadeWave(g)) { finishCascade(g); return; }
    g->cascade.revealing = true;
    g->cascade.waveTimer = CASCADE_REVEAL_DELAY;
}

static void resolveTurnEnd(Game *g)
{
    memorygrid_tickTurn(&g->grid); 
    if (g->currentBossType == BOSS_EPHEMERAL_CARDS) tickEphemeralCard(g);
    memorygrid_resolveAceValues(&g->grid, g->roundCfg.stackLimit);
    resolveGridChange(g);
}

static void endOrContinueTurn(Game *g)
{
    if (g->extraPlaysRemaining > 0)
    {
        g->extraPlaysRemaining--;
        setStatus(g, "MULTITHREAD: play your second card");
        return;
    }
    resolveTurnEnd(g);
}

static void afterCardPlaced(Game *g, Rank playedRank, int row, int col)
{
    audio_playPlace();

    if (playedRank == RANK_JACK)
    {
        g->mode = MODE_AWAITING_SWAP_FIRST;
    }
    else if (playedRank == RANK_QUEEN)
    {
        g->queenRow = row;
        g->queenCol = col;
        g->queenLockFirstRow = -1;
        g->queenLockFirstCol = -1;
        g->mode = MODE_AWAITING_QUEEN_LOCK_FIRST;
    }
    else if (playedRank == RANK_KING && g->grid.diagonalModeFrozenTurns == 0 && !g->grid.diagonalModeForced)
    {
        g->mode = MODE_AWAITING_FLIP_CHOICE;
    }
    else
    {
        if (playedRank == RANK_KING && g->grid.diagonalModeForced)
            setStatus(g, "KING: axis is forced this round and can't be flipped");
        else if (playedRank == RANK_KING)
            setStatus(g, TextFormat("KING: axis locked for %d more turn(s)", g->grid.diagonalModeFrozenTurns));
        g->mode = MODE_IDLE;
        endOrContinueTurn(g);
    }
}

static void tickTempBlock(Game *g)
{
    if (g->tempBlockTurnsLeft <= 0) return;
    g->tempBlockTurnsLeft--;
    if (g->tempBlockTurnsLeft == 0)
    {
        memorygrid_unblockCell(&g->grid, g->tempBlockRow, g->tempBlockCol);
        g->tempBlockRow = -1;
        g->tempBlockCol = -1;
    }
}

static void applyRottenDiscardPenalty(Game *g)
{
    int candidates[GRID_SIZE_MAX * GRID_SIZE_MAX][2];
    int count = 0;
    for (int r = 0; r < g->grid.size; r++)
        for (int c = 0; c < g->grid.size; c++)
            if (memorygrid_isCellFree(&g->grid, r, c)) { candidates[count][0] = r; candidates[count][1] = c; count++; }
    if (count == 0) return;

    int pick = GetRandomValue(0, count - 1);
    g->tempBlockRow = candidates[pick][0];
    g->tempBlockCol = candidates[pick][1];
    g->tempBlockTurnsLeft = 1;
    memorygrid_blockCell(&g->grid, g->tempBlockRow, g->tempBlockCol);
    setStatus(g, "ROTTEN DISCARD: a card rotted - a cell is blocked next turn");
}

static void addRandomRottenSlotAnywhere(Game *g)
{
    int gridCount = memorygrid_countRottenCandidates(&g->grid);
    int handCount = hand_countRottenCandidates(&g->hand);
    int total = gridCount + handCount;
    if (total <= 0) return;

    int pick = GetRandomValue(0, total - 1);
    if (pick < gridCount) memorygrid_addRottenSlotAtIndex(&g->grid, pick);
    else hand_addRottenSlotAtIndex(&g->hand, pick - gridCount);
}

static void tickEphemeralCard(Game *g)
{
    int markedSlot = -1;
    for (int i = 0; i < g->hand.capacity; i++)
        if (g->hand.occupied[i] && g->hand.cards[i].isEphemeral) markedSlot = i;

    if (markedSlot != -1)
    {
        g->ephemeralTurnsLeft--;
        if (g->ephemeralTurnsLeft <= 0)
        {
            hand_removeAt(&g->hand, markedSlot); 
            setStatus(g, "EPHEMERAL: an unplayed card was destroyed");
            g->turnsUntilNextEphemeralPick = EPHEMERAL_INTERVAL_TURNS;
        }
        return;
    }

    g->turnsUntilNextEphemeralPick--;
    if (g->turnsUntilNextEphemeralPick > 0) return;

    int candidates[HAND_SIZE], count = 0;
    for (int i = 0; i < g->hand.capacity; i++)
        if (g->hand.occupied[i]) candidates[count++] = i;
    if (count == 0) return;
    int slot = candidates[GetRandomValue(0, count - 1)];
    g->hand.cards[slot].isEphemeral = true;
    g->ephemeralTurnsLeft = EPHEMERAL_PLAY_WINDOW;
    setStatus(g, TextFormat("EPHEMERAL: play that card within %d turns or lose it", EPHEMERAL_PLAY_WINDOW));
}

static void commitPlacement(Game *g, int handIndex, int row, int col)
{
    tickTempBlock(g);

    g->undoGrid = g->grid;
    g->undoHand = g->hand;
    g->undoDeck = g->deck;
    g->undoRoundScore = g->roundScore;
    g->undoExtraPlays = g->extraPlaysRemaining;
    g->undoTurnCounter = g->turnCounter;
    g->hasUndoSnapshot = true;

    Rectangle fromHandRect = handSlotRect(handIndex, g->hand.capacity);
    Card played = hand_removeAt(&g->hand, handIndex);
    Rank playedRank = played.rank;
    played.isHidden = false;
    Card displaced = g->grid.cards[row][col];
    memorygrid_placeCard(&g->grid, row, col, played);
    deck_discard(&g->deck, displaced);
    spawnFlyingCard(g, played, fromHandRect, gridCellRect(row, col, g->grid.size), 0.0f);
    g->turnCounter++;
    if (g->roundCfg.rottenSlotsActive)
    {
        if (g->turnCounter % 3 == 0) addRandomRottenSlotAnywhere(g);
    }
    else
    {
        bool justRotted = hand_ageHeldCards(&g->hand, g->turnCounter > HAND_ROT_GRACE_TURNS);
        if (justRotted && g->currentBossType == BOSS_ROTTEN_DISCARD) applyRottenDiscardPenalty(g);
    }
    g->selectedHandIndex = -1;

    g->pendingPowerResolution = true;
    g->pendingPowerRank = playedRank;
    g->pendingPowerRow = row;
    g->pendingPowerCol = col;
    refillHandStep(g);
    if (g->mode != MODE_UNSTABLE_DECK_PICK)
    {
        g->pendingPowerResolution = false;
        afterCardPlaced(g, playedRank, g->pendingPowerRow, g->pendingPowerCol);
    }
}

static void resolveUnstableDeckPick(Game *g, bool pickedA)
{
    Card chosen = pickedA ? g->unstableDeckOptionA : g->unstableDeckOptionB;
    Card other  = pickedA ? g->unstableDeckOptionB : g->unstableDeckOptionA;

    g->hand.cards[g->unstableDeckSlot] = chosen;
    if (g->hand.rottenSlot[g->unstableDeckSlot]) g->hand.cards[g->unstableDeckSlot].isRotted = true;
    g->hand.occupied[g->unstableDeckSlot] = true;
    g->hand.turnsHeld[g->unstableDeckSlot] = 0;
    g->hand.count++;
    deck_injectCard(&g->deck, other);
    audio_playSlide();

    g->mode = MODE_IDLE;
    refillHandStep(g);

    if (g->mode != MODE_UNSTABLE_DECK_PICK && g->pendingPowerResolution)
    {
        g->pendingPowerResolution = false;
        afterCardPlaced(g, g->pendingPowerRank, g->pendingPowerRow, g->pendingPowerCol);
    }
}

static void shuffleShopPool(Game *g)
{
    int pool[ITEM_COUNT];
    for (int i = 0; i < ITEM_COUNT; i++) pool[i] = i;
    for (int i = ITEM_COUNT - 1; i > 0; i--)
    {
        int j = GetRandomValue(0, i);
        int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
    }
    for (int i = 0; i < SHOP_OFFER_COUNT; i++)
    {
        g->shopOffer[i] = pool[i];
        g->shopOfferSold[i] = false;
    }

    for (int i = 0; i < SHOP_CARD_OFFER_COUNT; i++)
    {
        Suit suit = (Suit)GetRandomValue(SUIT_HEART, SUIT_SPADE);
        Rank rank = WILDCARD_RANKS[GetRandomValue(0, 12)];
        g->shopCardOffer[i] = card_make(suit, rank);
        g->shopCardOfferSold[i] = false;
    }
}

static int shopCardPrice(Card card)
{
    return SHOP_CARD_PRICE_CEIL - card_getEffectiveValue(&card);
}

static void enterShop(Game *g)
{
    shuffleShopPool(g);
    g->shopRerollCost = SHOP_REROLL_BASE_COST;
    g->shopSwapPromptActive = false;
}

static void startNewRound(Game *g)
{
    g->cascade.active = false;
    g->roundCfg = round_getConfig(g->roundNumber);
    if (inventory_hasModule(&g->inventory, ITEM_OVERCLOCK)) 
        g->roundCfg.stackLimit = (int)(g->roundCfg.stackLimit * 0.9f);

    g->currentBossType = BOSS_NONE;
    int gridSize = GRID_SIZE_MIN;
    if (g->roundCfg.isBossRound)
    {
        g->currentBossType = (BossType)GetRandomValue(BOSS_BLOCKED_SECTOR, BOSS_TYPE_COUNT - 1);
        if (g->currentBossType == BOSS_EXPANDED_STACK)
        {
            gridSize = GRID_SIZE_MAX;
            g->roundCfg.stackLimit = (int)(g->roundCfg.stackLimit *
                ((float)(GRID_SIZE_MAX * GRID_SIZE_MAX) / (float)(GRID_SIZE_MIN * GRID_SIZE_MIN)));
        }
    }

    deck_initStandard52(&g->deck);
    for (int i = 0; i < g->removedCardCount; i++)
        deck_removeOneMatching(&g->deck, g->removedCards[i].suit, g->removedCards[i].rank);
    for (int i = 0; i < g->boughtCardCount; i++) deck_injectCard(&g->deck, g->boughtCards[i]);
    deck_shuffle(&g->deck);

    if (g->pendingEventCorruptCard && g->deck.count > 0)
    {
        g->deck.cards[GetRandomValue(0, g->deck.count - 1)].isSpecial = true;
        g->pendingEventCorruptCard = false;
    }
    if (g->roundNumber == 1 && g->startingClass == CLASS_COMPILER && g->deck.count > 0)
    {
        int first = GetRandomValue(0, g->deck.count - 1);
        g->deck.cards[first].isSpecial = true;
        if (g->deck.count > 1)
        {
            int second;
            do { second = GetRandomValue(0, g->deck.count - 1); } while (second == first);
            g->deck.cards[second].isSpecial = true;
        }
    }
    if (g->roundCfg.memoryCorruptionActive && g->deck.count > 0)
        g->deck.cards[GetRandomValue(0, g->deck.count - 1)].isSpecial = true;

    memorygrid_init(&g->grid, &g->deck, gridSize);
    memorygrid_setDisabledCombo(&g->grid, g->roundCfg.disabledCombo);
    if (g->currentBossType == BOSS_BLOCKED_SECTOR)
        memorygrid_blockCell(&g->grid, L1_CACHE_ROW, L1_CACHE_COL);
    if (g->currentBossType == BOSS_TRAP_CELL)
        memorygrid_setTrapCell(&g->grid, GetRandomValue(0, gridSize - 1), GetRandomValue(0, gridSize - 1));
    if (g->currentBossType == BOSS_RESTRICTED_BOARD)
        memorygrid_setBannedAxis(&g->grid, GetRandomValue(BANNED_AXIS_ROWS, BANNED_AXIS_DIAGONALS));
    if (g->currentBossType == BOSS_SCORE_THRESHOLD)
        memorygrid_setScoreThresholdActive(&g->grid, true);
    if (g->currentBossType == BOSS_FORCED_DIAGONAL)
        memorygrid_setDiagonalModeForced(&g->grid, true);

    g->tempBlockRow = -1;
    g->tempBlockCol = -1;
    g->tempBlockTurnsLeft = 0;
    g->turnsUntilNextEphemeralPick = EPHEMERAL_INTERVAL_TURNS;
    g->ephemeralTurnsLeft = 0;

    memorygrid_resolveAceValues(&g->grid, g->roundCfg.stackLimit);
    memorygrid_ensureFairDeal(&g->grid, &g->deck, g->roundCfg.stackLimit, true, FAIR_DEAL_MAX_ATTEMPTS);
    memorygrid_resolveAceValues(&g->grid, g->roundCfg.stackLimit);
    memorygrid_ensureFairDeal(&g->grid, &g->deck, g->roundCfg.stackLimit, true, FAIR_DEAL_MAX_ATTEMPTS);

    if (g->roundNumber == 1)
    {
        if (g->startingClass == CLASS_BANKER)
        {
            inventory_buyModule(&g->inventory, ITEM_BANKER_CHIP);
            memorygrid_setBankerChipActive(&g->grid, true);
        }
        else if (g->startingClass == CLASS_ARCHITECT)
        {
            inventory_buyModule(&g->inventory, ITEM_REDUNDANT_COLOR);
            memorygrid_setRedundantColorActive(&g->grid, true);
        }
        else if (g->startingClass == CLASS_COMPILER)
        {
            inventory_buyScript(&g->inventory, ITEM_COMPILER_PATCH);
        }
    }

    int handCapacity = inventory_hasModule(&g->inventory, ITEM_PREFETCH) ? HAND_SIZE : HAND_DEFAULT_CAPACITY;
    hand_init(&g->hand, handCapacity);

    g->selectedHandIndex = -1;
    g->roundScore = 0;
    g->mode = MODE_IDLE;
    g->isDragging = false;
    g->extraPlaysRemaining = 0;
    g->deckPeekActive = false;
    g->pendingPowerResolution = false;
    g->hasUndoSnapshot = false;
    g->statusMessage[0] = '\0';
    g->statusMessageTimer = 0.0f;
    g->deckPopupOpen = false;
    g->turnCounter = 0;
    g->scorePopup.active = false;
    for (int i = 0; i < MAX_FLYING_CARDS; i++) g->flyingCards[i].active = false;
    inventory_onRoundStart(&g->inventory);
    g->phase = PHASE_PLAYING;

    refillHandStep(g); 
}

static void startTutorial(Game *g)
{
    g->roundNumber = 1;
    g->gold = 0;
    inventory_init(&g->inventory);
    memorygrid_construct(&g->grid);
    g->pendingEventCorruptCard = false;
    g->boughtCardCount = 0;
    g->removedCardCount = 0;
    g->deckEditOpen = false;
    g->startingClass = CLASS_COMPILER;

    g->roundCfg = round_getConfig(1);
    g->roundCfg.stackLimit = 150;  
    g->roundCfg.objective = 9999; 
    g->roundCfg.disabledCombo = COMBO_NONE;
    g->currentBossType = BOSS_NONE;

    deck_initStandard52(&g->deck);
    deck_shuffle(&g->deck); 

    g->grid.size = GRID_SIZE_MIN;
    for (int r = 0; r < GRID_SIZE_MAX; r++)
        for (int c = 0; c < GRID_SIZE_MAX; c++)
        {
            g->grid.lockOwnerRow[r][c] = -1;
            g->grid.lockOwnerCol[r][c] = -1;
        }
    g->grid.diagonalMode = false;
    g->grid.diagonalModeFrozenTurns = 0;
    g->grid.trapRow = -1;
    g->grid.trapCol = -1;
    g->grid.bannedAxis = BANNED_AXIS_NONE;
    g->grid.scoreThresholdActive = false;
    memorygrid_setDisabledCombo(&g->grid, COMBO_NONE);

    g->grid.cards[0][0] = card_make(SUIT_SPADE,   RANK_NINE);
    g->grid.cards[0][1] = card_make(SUIT_CLUB,    RANK_FOUR);
    g->grid.cards[0][2] = card_make(SUIT_HEART,   RANK_FIVE);
    g->grid.cards[1][0] = card_make(SUIT_CLUB,    RANK_THREE);
    g->grid.cards[1][1] = card_make(SUIT_HEART,   RANK_NINE);
    g->grid.cards[1][2] = card_make(SUIT_DIAMOND, RANK_SIX);
    g->grid.cards[2][0] = card_make(SUIT_SPADE,   RANK_EIGHT);
    g->grid.cards[2][1] = card_make(SUIT_DIAMOND, RANK_SEVEN);
    g->grid.cards[2][2] = card_make(SUIT_CLUB,    RANK_TWO);
    g->grid.stackScore = memorygrid_calculateStackScore(&g->grid);

    hand_init(&g->hand, HAND_DEFAULT_CAPACITY);
    g->hand.cards[0] = card_make(SUIT_DIAMOND, RANK_JACK);  g->hand.occupied[0] = true;
    g->hand.cards[1] = card_make(SUIT_SPADE,   RANK_QUEEN); g->hand.occupied[1] = true;
    g->hand.cards[2] = card_make(SUIT_CLUB,    RANK_KING);  g->hand.occupied[2] = true;
    g->hand.cards[3] = card_make(SUIT_HEART,   RANK_ACE);   g->hand.occupied[3] = true;
    g->hand.count = 4;

    g->selectedHandIndex = -1;
    g->roundScore = 0;
    g->mode = MODE_IDLE;
    g->isDragging = false;
    g->extraPlaysRemaining = 0;
    g->deckPeekActive = false;
    g->pendingPowerResolution = false;
    g->hasUndoSnapshot = false;
    g->statusMessage[0] = '\0';
    g->statusMessageTimer = 0.0f;
    g->deckPopupOpen = false;
    g->turnCounter = 0;
    g->scorePopup.active = false;
    for (int i = 0; i < MAX_FLYING_CARDS; i++) g->flyingCards[i].active = false;
    g->cascade.active = false;
    g->helpOverlayOpen = false;
    g->isPaused = false;

    g->tutorialActive = true;
    g->tutorialStep = 0;
    g->phase = PHASE_PLAYING;
}

static bool tutorialClickMatches(const Game *g, Vector2 mouse)
{
    const TutStep *step = &TUTORIAL_SCRIPT[g->tutorialStep];
    switch (step->kind)
    {
        case TUT_HAND: return CheckCollisionPointRec(mouse, handSlotRect(step->a, g->hand.capacity));
        case TUT_CELL: return CheckCollisionPointRec(mouse, gridCellRect(step->a, step->b, g->grid.size));
        case TUT_YES:  return CheckCollisionPointRec(mouse, flipChoiceRect(0));
        default:       return false;
    }
}

static void tutorialAdvance(Game *g)
{
    g->tutorialStep++;
    if (g->tutorialStep >= TUTORIAL_STEP_COUNT)
    {
        g->tutorialActive = false;
        tutorial_markCompleted();
        g->phase = PHASE_CLASS_SELECT;
    }
}

static bool tutorialHighlightRect(const Game *g, const TutStep *step, Rectangle *out)
{
    switch (step->kind)
    {
        case TUT_HAND: *out = handSlotRect(step->a, g->hand.capacity); return true;
        case TUT_CELL: *out = gridCellRect(step->a, step->b, g->grid.size); return true;
        case TUT_YES:  *out = flipChoiceRect(0); return true;
        case TUT_MSG:
            if (step->hl == TUT_HL_STACK)   { *out = (Rectangle){ 12, 54, 340, 126 }; return true; }
            if (step->hl == TUT_HL_GRID)    { *out = gridBoundsRect(g->grid.size); return true; }
            if (step->hl == TUT_HL_L1CACHE) { *out = gridCellRect(L1_CACHE_ROW, L1_CACHE_COL, g->grid.size); return true; }
            return false;
        default: return false;
    }
}

static void drawTutorialOverlay(const Game *g)
{
    const TutStep *step = &TUTORIAL_SCRIPT[g->tutorialStep];

    Rectangle target = { 0 };
    bool hasTarget = tutorialHighlightRect(g, step, &target);

    float panelH = step->line2 ? 76.0f : 48.0f;
    float panelY = 60.0f;
    if (hasTarget)
    {
        float overlap = fminf(target.y + target.height, panelY + panelH) - fmaxf(target.y, panelY);
        if (overlap > 20.0f) panelY = target.y + target.height + 14.0f;
    }

    DrawRectangle(0, (int)panelY, SCREEN_WIDTH, (int)panelH, COLOR_PANEL);
    drawTextCentered(step->line1, SCREEN_WIDTH / 2.0f, panelY + 8.0f, 21, COLOR_PROMPT);
    if (step->line2)
        drawTextCentered(step->line2, SCREEN_WIDTH / 2.0f, panelY + 36.0f, 15, RAYWHITE);
    if (step->kind == TUT_MSG)
        drawTextCentered("click anywhere to continue", SCREEN_WIDTH / 2.0f, panelY + panelH - 18.0f, 12, GRAY);
    DrawText(TextFormat("step %d / %d", g->tutorialStep + 1, TUTORIAL_STEP_COUNT),
              SCREEN_WIDTH - 110, (int)panelY + 6, 13, GRAY);

    if (hasTarget)
    {
        float pulse = (sinf((float)GetTime() * 6.0f) + 1.0f) / 2.0f;
        drawGlow(target, COLOR_PROMPT, 0.7f + pulse * 0.3f);
    }
}

static bool rollModuleChoiceOffer(Game *g)
{
    bool hasFreeSlot = false;
    for (int i = 0; i < MODULE_SLOTS; i++)
        if (g->inventory.modules[i] == NO_ITEM) hasFreeSlot = true;
    if (!hasFreeSlot) return false;

    int candidates[ITEM_COUNT], candidateCount = 0;
    for (int id = 0; id < ITEM_COUNT; id++)
        if (shop_getItemInfo((ShopItemId)id)->isModule && !inventory_hasModule(&g->inventory, (ShopItemId)id))
            candidates[candidateCount++] = id;
    if (candidateCount == 0) return false;

    for (int i = candidateCount - 1; i > 0; i--)
    {
        int j = GetRandomValue(0, i);
        int tmp = candidates[i]; candidates[i] = candidates[j]; candidates[j] = tmp;
    }

    g->moduleChoiceCount = candidateCount < 3 ? candidateCount : 3;
    for (int i = 0; i < g->moduleChoiceCount; i++) g->moduleChoiceOffer[i] = candidates[i];
    return true;
}

static void proceedToNextRound(Game *g)
{
    if (GetRandomValue(1, 100) <= EVENT_TRIGGER_CHANCE_PERCENT)
        g->phase = PHASE_EVENT;
    else
        startNewRound(g);
}

static void fullRestart(Game *g)
{
    g->roundNumber = 1;
    g->gold = 0;
    inventory_init(&g->inventory);
    memorygrid_construct(&g->grid);
    g->pendingEventCorruptCard = false;
    g->boughtCardCount = 0;
    g->removedCardCount = 0;
    g->deckEditOpen = false;
    g->deckEditUpgradeMode = false;
    g->phase = PHASE_CLASS_SELECT;
    g->tutorialActive = false;
    g->cascade.active = false;
}

int main(void)
{
    srand((unsigned int)time(NULL));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE); 
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "STACK OVERFLOW");
    SetWindowMinSize(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    cardtex_loadAll();
    uitex_loadAll();
    audio_loadAll();
    fonts_loadAll();
    g_gameFont = fonts_get();
    background_load(SCREEN_WIDTH, SCREEN_HEIGHT);

    RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);

    Game game = { 0 };
    game.animSpeed = 1.0f;
    fullRestart(&game);
    game.phase = PHASE_MAIN_MENU;

    while (!WindowShouldClose() && !game.wantsQuit)
    {
        Game *g = &game;

        updateRenderTransform();
        if (IsKeyPressed(KEY_F11) || (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))))
            ToggleBorderlessWindowed();

        if (IsKeyPressed(KEY_TAB))
        {
            g->animSpeed = (g->animSpeed >= 4.0f) ? 1.0f : g->animSpeed * 2.0f;
        }

        if (g->statusMessageTimer > 0.0f) g->statusMessageTimer -= GetFrameTime();

        float dt = GetFrameTime() * g->animSpeed;
        for (int i = 0; i < MAX_FLYING_CARDS; i++)
        {
            if (!g->flyingCards[i].active) continue;
            g->flyingCards[i].elapsed += dt;
            if (g->flyingCards[i].elapsed >= FLY_DURATION) g->flyingCards[i].active = false;
        }
        if (g->scorePopup.active)
        {
            g->scorePopup.elapsed += dt;
            if (g->scorePopup.elapsed >= SCORE_POPUP_DURATION) g->scorePopup.active = false;
        }
        if (g->comboFlashTimer > 0.0f)
        {
            g->comboFlashTimer -= dt;
            if (g->comboFlashTimer < 0.0f) g->comboFlashTimer = 0.0f;
        }

        if (!g->isPaused) updateCascade(g, dt); 

        if (g->tutorialActive && TUTORIAL_SCRIPT[g->tutorialStep].kind == TUT_WAIT && !g->cascade.active)
            tutorialAdvance(g);

        Vector2 shakeOffset = { 0.0f, 0.0f };
        if (g->shakeTimer > 0.0f)
        {
            g->shakeTimer -= dt;
            if (g->shakeTimer < 0.0f) g->shakeTimer = 0.0f;
            float t = (g->shakeDuration > 0.0f) ? g->shakeTimer / g->shakeDuration : 0.0f;
            float mag = g->shakeMagnitude * t;
            shakeOffset.x = ((float)GetRandomValue(-1000, 1000) / 1000.0f) * mag;
            shakeOffset.y = ((float)GetRandomValue(-1000, 1000) / 1000.0f) * mag;
        }

        for (int i = 0; i < HAND_SIZE; i++)
        {
            bool hovered = g->phase == PHASE_PLAYING && !g->isDragging && !g->deckPopupOpen && !g->isPaused &&
                            g->hand.occupied[i] &&
                            CheckCollisionPointRec(GetMousePosition(), handSlotRect(i, g->hand.capacity));
            float target = hovered ? 1.0f : 0.0f;
            g->handHoverLift[i] += (target - g->handHoverLift[i]) * fminf(1.0f, dt * 14.0f);
        }

        if (g->helpOverlayOpen)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_H))
                g->helpOverlayOpen = false;
        }
        else if (g->phase == PHASE_PLAYING && IsKeyPressed(KEY_H) && !g->tutorialActive &&
                  !g->deckPopupOpen && !g->isPaused && !g->cascade.active && !g->shopSwapPromptActive)
        {
            g->helpOverlayOpen = true;
        }
        else if (g->deckPopupOpen)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE)) g->deckPopupOpen = false;
        }
        else if (g->deckEditOpen)
        {
            if (IsKeyPressed(KEY_ESCAPE)) g->deckEditOpen = false;
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                Rectangle removeBtn = { SCREEN_WIDTH / 2.0f - 220, 95, 200, 40 };
                Rectangle upgradeBtn = { SCREEN_WIDTH / 2.0f + 20, 95, 200, 40 };
                Rectangle closeBtn = { SCREEN_WIDTH - 130, 20, 100, 40 };
                if (CheckCollisionPointRec(mouse, removeBtn)) g->deckEditUpgradeMode = false;
                else if (CheckCollisionPointRec(mouse, upgradeBtn)) g->deckEditUpgradeMode = true;
                else if (CheckCollisionPointRec(mouse, closeBtn)) g->deckEditOpen = false;
                else
                {
                    Card composition[DECK_MAX_SIZE];
                    int n = buildFullDeckComposition(g, composition);
                    int cost = deckEditCost(g);
                    for (int i = 0; i < n; i++)
                    {
                        if (!CheckCollisionPointRec(mouse, deckPopupCardRect(i))) continue;
                        if (g->gold < cost) { setStatus(g, TextFormat("Not enough gold - costs $%d", cost)); break; }
                        g->gold -= cost;
                        Card target = composition[i];
                        deckEditRemoveCard(g, target);
                        if (g->deckEditUpgradeMode && g->boughtCardCount < MAX_BOUGHT_CARDS)
                        {
                            Card upgraded = card_make(target.suit, upgradeRank(target.rank));
                            g->boughtCards[g->boughtCardCount++] = upgraded;
                            setStatus(g, "DECK EDIT: card upgraded");
                        }
                        else
                        {
                            setStatus(g, "DECK EDIT: card removed");
                        }
                        audio_playSlide();
                        break;
                    }
                }
            }
        }
        else if (g->isPaused)
        {
            if (IsKeyPressed(KEY_ESCAPE))
            {
                g->isPaused = false;
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, pauseButtonRect(0))) g->isPaused = false;
                else if (CheckCollisionPointRec(mouse, pauseButtonRect(1))) { g->isPaused = false; fullRestart(g); }
                else if (CheckCollisionPointRec(mouse, pauseButtonRect(2))) g->wantsQuit = true;
                else if (CheckCollisionPointRec(mouse, pauseButtonRect(3)))
                    g->animSpeed = (g->animSpeed >= 4.0f) ? 1.0f : g->animSpeed * 2.0f;
            }
        }
        else if (g->phase == PHASE_PLAYING && IsKeyPressed(KEY_ESCAPE) && !g->tutorialActive)
        {
            g->isPaused = true;
        }
        else if (g->cascade.active)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
                g->cascade.waveTimer = 0.0f;
        }
        else if (g->tutorialActive && TUTORIAL_SCRIPT[g->tutorialStep].kind == TUT_MSG)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
                tutorialAdvance(g);
        }
        else if (g->phase == PHASE_MAIN_MENU)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, mainMenuButtonRect(0))) 
                {
                    audio_playSlide();
                    if (!tutorial_hasCompleted()) startTutorial(g);
                    else g->phase = PHASE_CLASS_SELECT;
                }
                else if (CheckCollisionPointRec(mouse, mainMenuButtonRect(1))) g->wantsQuit = true; 
            }
        }
        else if (g->phase == PHASE_CLASS_SELECT)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int i = 0; i < CLASS_COUNT; i++)
                    if (CheckCollisionPointRec(mouse, shopItemRect(i)))
                    {
                        audio_playSlide();
                        g->startingClass = (StartingClass)i;
                        startNewRound(g);
                    }
            }
        }
        else if (g->phase == PHASE_GAME_OVER)
        {
            if (IsKeyPressed(KEY_R)) fullRestart(g);
        }
        else if (g->phase == PHASE_SHOP && g->shopSwapPromptActive)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                int slotCount = g->shopSwapIsModule ? MODULE_SLOTS : SCRIPT_SLOTS;
                for (int i = 0; i < slotCount; i++)
                {
                    if (!CheckCollisionPointRec(mouse, swapPromptSlotRect(i))) continue;

                    ShopItemId newId = (ShopItemId)g->shopSwapPendingItem;
                    const ShopItemInfo *newInfo = shop_getItemInfo(newId);
                    if (g->shopSwapIsModule)
                    {
                        ShopItemId oldId = (ShopItemId)g->inventory.modules[i];
                        setModuleGridFlag(g, oldId, false);
                        inventory_removeModule(&g->inventory, i);
                        inventory_buyModule(&g->inventory, newId);
                        setModuleGridFlag(g, newId, true);
                    }
                    else
                    {
                        inventory_consumeScript(&g->inventory, i);
                        inventory_buyScript(&g->inventory, newId);
                    }
                    g->gold -= newInfo->cost;
                    g->shopOfferSold[g->shopSwapPendingOfferSlot] = true;
                    audio_playShopBuy();
                    g->shopSwapPromptActive = false;
                }
                if (CheckCollisionPointRec(mouse, swapPromptCancelRect()))
                    g->shopSwapPromptActive = false;
            }
            else if (IsKeyPressed(KEY_ESCAPE))
                g->shopSwapPromptActive = false;
        }
        else if (g->phase == PHASE_SHOP)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int slot = 0; slot < SHOP_OFFER_COUNT; slot++)
                {
                    Rectangle box = shopItemRect(slot);
                    if (!CheckCollisionPointRec(mouse, box)) continue;
                    if (g->shopOfferSold[slot]) continue;

                    ShopItemId id = (ShopItemId)g->shopOffer[slot];
                    const ShopItemInfo *info = shop_getItemInfo(id);
                    bool owned = info->isModule && inventory_hasModule(&g->inventory, id);
                    if (owned) continue;
                    if (g->gold < info->cost)
                    {
                        setStatus(g, TextFormat("Not enough gold - %s costs $%d", info->name, info->cost));
                        continue;
                    }

                    bool full = info->isModule
                        ? inventory_moduleSlotsFull(&g->inventory)
                        : inventory_scriptSlotsFull(&g->inventory);
                    if (full)
                    {
                        g->shopSwapPromptActive = true;
                        g->shopSwapIsModule = info->isModule;
                        g->shopSwapPendingItem = (int)id;
                        g->shopSwapPendingOfferSlot = slot;
                        continue;
                    }

                    bool bought = info->isModule
                        ? inventory_buyModule(&g->inventory, id)
                        : inventory_buyScript(&g->inventory, id);
                    if (!bought) continue;

                    g->gold -= info->cost;
                    g->shopOfferSold[slot] = true;
                    audio_playShopBuy();
                    setModuleGridFlag(g, id, true);
                }

                for (int slot = 0; slot < SHOP_CARD_OFFER_COUNT; slot++)
                {
                    Rectangle box = shopCardOfferRect(slot);
                    if (!CheckCollisionPointRec(mouse, box)) continue;
                    if (g->shopCardOfferSold[slot]) continue;

                    int price = shopCardPrice(g->shopCardOffer[slot]);
                    if (g->gold < price)
                    {
                        setStatus(g, TextFormat("Not enough gold - this card costs $%d", price));
                        continue;
                    }
                    if (g->boughtCardCount >= MAX_BOUGHT_CARDS) continue;

                    g->gold -= price;
                    g->boughtCards[g->boughtCardCount++] = g->shopCardOffer[slot];
                    g->shopCardOfferSold[slot] = true;
                    audio_playShopBuy();
                }

                for (int i = 0; i < MODULE_SLOTS; i++)
                {
                    if (g->inventory.modules[i] == NO_ITEM) continue;
                    if (!CheckCollisionPointRec(mouse, shopOwnedModuleRect(i))) continue;
                    ShopItemId id = (ShopItemId)g->inventory.modules[i];
                    g->gold += sellRefund(id);
                    setModuleGridFlag(g, id, false);
                    inventory_removeModule(&g->inventory, i);
                    audio_playSlide();
                }
                for (int i = 0; i < SCRIPT_SLOTS; i++)
                {
                    if (g->inventory.scripts[i] == NO_ITEM) continue;
                    if (!CheckCollisionPointRec(mouse, shopOwnedScriptRect(i))) continue;
                    ShopItemId id = (ShopItemId)g->inventory.scripts[i];
                    g->gold += sellRefund(id);
                    inventory_consumeScript(&g->inventory, i);
                    audio_playSlide();
                }

                Rectangle rerollBtn = { SCREEN_WIDTH / 2.0f - 260, 610, 150, 50 };
                if (CheckCollisionPointRec(mouse, rerollBtn))
                {
                    if (g->gold >= g->shopRerollCost)
                    {
                        g->gold -= g->shopRerollCost;
                        shuffleShopPool(g);
                        g->shopRerollCost++;
                        audio_playSlide();
                    }
                    else
                    {
                        setStatus(g, TextFormat("Not enough gold - reroll costs $%d", g->shopRerollCost));
                    }
                }

                Rectangle continueBtn = { SCREEN_WIDTH / 2.0f - 100, 610, 200, 50 };
                if (CheckCollisionPointRec(mouse, continueBtn))
                {
                    audio_playSlide();
                    g->roundNumber++;
                    if (g->roundNumber % MODULE_CHOICE_EVERY_N_ROUNDS == 0 && rollModuleChoiceOffer(g))
                        g->phase = PHASE_MODULE_CHOICE;
                    else
                        proceedToNextRound(g);
                }

                Rectangle editDeckBtn = { SCREEN_WIDTH / 2.0f + 110, 610, 150, 50 };
                if (CheckCollisionPointRec(mouse, editDeckBtn)) g->deckEditOpen = true;
            }
        }
        else if (g->phase == PHASE_EVENT)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                Rectangle acceptBtn  = { SCREEN_WIDTH / 2.0f - 220, 420, 200, 60 };
                Rectangle declineBtn = { SCREEN_WIDTH / 2.0f + 20,  420, 200, 60 };
                if (CheckCollisionPointRec(mouse, acceptBtn))
                {
                    audio_playShopBuy();
                    g->gold += EVENT_GOLD_REWARD;
                    g->pendingEventCorruptCard = true;
                    startNewRound(g);
                }
                else if (CheckCollisionPointRec(mouse, declineBtn))
                {
                    audio_playSlide();
                    startNewRound(g);
                }
            }
        }
        else if (g->phase == PHASE_MODULE_CHOICE)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int i = 0; i < g->moduleChoiceCount; i++)
                    if (CheckCollisionPointRec(mouse, shopItemRect(i)))
                    {
                        ShopItemId id = (ShopItemId)g->moduleChoiceOffer[i];
                        if (!inventory_buyModule(&g->inventory, id)) continue;
                        setModuleGridFlag(g, id, true);
                        audio_playShopBuy();
                        proceedToNextRound(g);
                    }
            }
        }
        else if (g->mode == MODE_UNSTABLE_DECK_PICK)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                if (CheckCollisionPointRec(mouse, unstablePickRect(0))) resolveUnstableDeckPick(g, true);
                else if (CheckCollisionPointRec(mouse, unstablePickRect(1))) resolveUnstableDeckPick(g, false);
            }
        }
        else if (g->mode == MODE_AWAITING_FLIP_CHOICE)
        {
            if (g->tutorialActive)
            {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), flipChoiceRect(0)))
                {
                    memorygrid_toggleAxisMode(&g->grid);
                    g->mode = MODE_IDLE;
                    tutorialAdvance(g);
                    endOrContinueTurn(g);
                }
            }
            else
            {
                bool clickYes = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), flipChoiceRect(0));
                bool clickNo  = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), flipChoiceRect(1));
                if (IsKeyPressed(KEY_Y) || clickYes)
                {
                    memorygrid_toggleAxisMode(&g->grid);
                    g->mode = MODE_IDLE;
                    endOrContinueTurn(g);
                }
                else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ENTER) || clickNo) { g->mode = MODE_IDLE; endOrContinueTurn(g); }
            }
        }
        else if (g->mode == MODE_AWAITING_SWAP_FIRST)
        {
            if (g->tutorialActive)
            {
                Vector2 mouse = GetMousePosition();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && tutorialClickMatches(g, mouse))
                {
                    g->swapFirstRow = TUTORIAL_SCRIPT[g->tutorialStep].a;
                    g->swapFirstCol = TUTORIAL_SCRIPT[g->tutorialStep].b;
                    g->mode = MODE_AWAITING_SWAP_SECOND;
                    tutorialAdvance(g);
                }
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; endOrContinueTurn(g); }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int row = 0; row < g->grid.size; row++)
                    for (int col = 0; col < g->grid.size; col++)
                        if (memorygrid_isCellFree(&g->grid, row, col) &&
                            CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size)))
                        {
                            g->swapFirstRow = row;
                            g->swapFirstCol = col;
                            g->mode = MODE_AWAITING_SWAP_SECOND;
                        }
            }
        }
        else if (g->mode == MODE_AWAITING_SWAP_SECOND)
        {
            if (g->tutorialActive)
            {
                Vector2 mouse = GetMousePosition();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && tutorialClickMatches(g, mouse))
                {
                    int row = TUTORIAL_SCRIPT[g->tutorialStep].a, col = TUTORIAL_SCRIPT[g->tutorialStep].b;
                    Rectangle rectA = gridCellRect(g->swapFirstRow, g->swapFirstCol, g->grid.size);
                    Rectangle rectB = gridCellRect(row, col, g->grid.size);
                    Card cardA = g->grid.cards[g->swapFirstRow][g->swapFirstCol];
                    Card cardB = g->grid.cards[row][col];
                    memorygrid_swapCells(&g->grid, g->swapFirstRow, g->swapFirstCol, row, col);
                    spawnFlyingCard(g, cardA, rectA, rectB, 0.0f);
                    spawnFlyingCard(g, cardB, rectB, rectA, 0.0f);
                    audio_playSlide();
                    g->mode = MODE_IDLE;
                    tutorialAdvance(g);
                    endOrContinueTurn(g);
                }
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; endOrContinueTurn(g); }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int row = 0; row < g->grid.size; row++)
                    for (int col = 0; col < g->grid.size; col++)
                        if (memorygrid_isCellFree(&g->grid, row, col) &&
                            CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size)) &&
                            !(row == g->swapFirstRow && col == g->swapFirstCol))
                        {
                            Rectangle rectA = gridCellRect(g->swapFirstRow, g->swapFirstCol, g->grid.size);
                            Rectangle rectB = gridCellRect(row, col, g->grid.size);
                            Card cardA = g->grid.cards[g->swapFirstRow][g->swapFirstCol];
                            Card cardB = g->grid.cards[row][col];
                            memorygrid_swapCells(&g->grid, g->swapFirstRow, g->swapFirstCol, row, col);
                            spawnFlyingCard(g, cardA, rectA, rectB, 0.0f);
                            spawnFlyingCard(g, cardB, rectB, rectA, 0.0f);
                            audio_playSlide();
                            g->mode = MODE_IDLE;
                            endOrContinueTurn(g);
                        }
            }
        }
        else if (g->mode == MODE_AWAITING_QUEEN_LOCK_FIRST)
        {
            if (g->tutorialActive)
            {
                Vector2 mouse = GetMousePosition();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && tutorialClickMatches(g, mouse))
                {
                    g->queenLockFirstRow = TUTORIAL_SCRIPT[g->tutorialStep].a;
                    g->queenLockFirstCol = TUTORIAL_SCRIPT[g->tutorialStep].b;
                    g->mode = MODE_AWAITING_QUEEN_LOCK_SECOND;
                    tutorialAdvance(g);
                }
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; endOrContinueTurn(g); }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                int neighbors[8][2];
                int count = memorygrid_queenNeighbors(g->queenRow, g->queenCol, g->grid.size, g->roundCfg.extendedLockActive, neighbors);
                for (int n = 0; n < count; n++)
                    if (CheckCollisionPointRec(mouse, gridCellRect(neighbors[n][0], neighbors[n][1], g->grid.size)))
                    {
                        g->queenLockFirstRow = neighbors[n][0];
                        g->queenLockFirstCol = neighbors[n][1];
                        g->mode = MODE_AWAITING_QUEEN_LOCK_SECOND;
                    }
            }
        }
        else if (g->mode == MODE_AWAITING_QUEEN_LOCK_SECOND)
        {
            if (g->tutorialActive)
            {
                Vector2 mouse = GetMousePosition();
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && tutorialClickMatches(g, mouse))
                {
                    int row = TUTORIAL_SCRIPT[g->tutorialStep].a, col = TUTORIAL_SCRIPT[g->tutorialStep].b;
                    memorygrid_queenLock(&g->grid, g->queenRow, g->queenCol,
                                          g->queenLockFirstRow, g->queenLockFirstCol, row, col);
                    audio_playSlide();
                    g->mode = MODE_IDLE;
                    tutorialAdvance(g);
                    endOrContinueTurn(g);
                }
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            {
                memorygrid_queenLock(&g->grid, g->queenRow, g->queenCol,
                                      g->queenLockFirstRow, g->queenLockFirstCol, -1, -1);
                g->mode = MODE_IDLE;
                endOrContinueTurn(g);
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                int neighbors[8][2];
                int count = memorygrid_queenNeighbors(g->queenRow, g->queenCol, g->grid.size, g->roundCfg.extendedLockActive, neighbors);
                for (int n = 0; n < count; n++)
                {
                    int row = neighbors[n][0], col = neighbors[n][1];
                    if (row == g->queenLockFirstRow && col == g->queenLockFirstCol) continue;
                    if (!CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size))) continue;

                    memorygrid_queenLock(&g->grid, g->queenRow, g->queenCol,
                                          g->queenLockFirstRow, g->queenLockFirstCol, row, col);
                    audio_playSlide();
                    g->mode = MODE_IDLE;
                    endOrContinueTurn(g);
                }
            }
        }
        else if (g->mode == MODE_WILDCARD_PICK_RANK)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int i = 0; i < 13; i++)
                    if (CheckCollisionPointRec(mouse, rankPickRect(i)))
                    {
                        g->wildcardRank = WILDCARD_RANKS[i];
                        g->mode = MODE_WILDCARD_PICK_CELL;
                    }
            }
        }
        else if (g->mode == MODE_WILDCARD_PICK_CELL)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int row = 0; row < g->grid.size; row++)
                    for (int col = 0; col < g->grid.size; col++)
                        if (memorygrid_isCellFree(&g->grid, row, col) &&
                            CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size)))
                        {
                            Suit suit = (Suit)GetRandomValue(SUIT_HEART, SUIT_SPADE);
                            Card manufactured = card_make(suit, g->wildcardRank);
                            Card displaced = g->grid.cards[row][col];
                            memorygrid_placeCard(&g->grid, row, col, manufactured);
                            deck_discard(&g->deck, displaced);
                            inventory_consumeScript(&g->inventory, g->pendingScriptSlot);
                            g->mode = MODE_IDLE;
                            afterCardPlaced(g, g->wildcardRank, row, col);
                        }
            }
        }
        else if (g->mode == MODE_MEMORY_FLUSH_PICK_CELL)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int row = 0; row < g->grid.size; row++)
                    for (int col = 0; col < g->grid.size; col++)
                        if (CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size)))
                        {
                            Card oldCard = g->grid.cards[row][col];
                            memorygrid_memoryFlush(&g->grid, &g->deck, row, col);
                            if (cardVisuallyDiffers(oldCard, g->grid.cards[row][col]))
                                spawnCellReplaceAnimation(g, gridCellRect(row, col, g->grid.size), oldCard, g->grid.cards[row][col]);
                            audio_playSlide();
                            inventory_consumeScript(&g->inventory, g->pendingScriptSlot);
                            g->mode = MODE_IDLE;
                            memorygrid_resolveAceValues(&g->grid, g->roundCfg.stackLimit);
                            resolveGridChange(g); 
                        }
            }
        }
        else if (g->mode == MODE_COMPILER_PATCH_PICK_RANK)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int i = 0; i < 13; i++)
                    if (CheckCollisionPointRec(mouse, rankPickRect(i)))
                    {
                        Suit suit = (Suit)GetRandomValue(SUIT_HEART, SUIT_SPADE);
                        Card special = card_make(suit, WILDCARD_RANKS[i]);
                        special.isSpecial = true;
                        deck_injectCard(&g->deck, special);
                        audio_playSlide();
                        inventory_consumeScript(&g->inventory, g->pendingScriptSlot);
                        g->mode = MODE_IDLE;
                    }
            }
        }
        else if (g->mode == MODE_NULL_POINTER_PICK_RANK)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { g->mode = MODE_IDLE; }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                for (int i = 0; i < 13; i++)
                    if (CheckCollisionPointRec(mouse, rankPickRect(i)))
                    {
                        deck_purgeRank(&g->deck, WILDCARD_RANKS[i]);
                        audio_playSlide();
                        inventory_consumeScript(&g->inventory, g->pendingScriptSlot);
                        g->mode = MODE_IDLE;
                    }
            }
        }
        else if (g->tutorialActive && g->mode == MODE_IDLE)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                if (tutorialClickMatches(g, mouse))
                {
                    TutStepKind kind = TUTORIAL_SCRIPT[g->tutorialStep].kind;
                    if (kind == TUT_HAND)
                    {
                        g->selectedHandIndex = TUTORIAL_SCRIPT[g->tutorialStep].a;
                        tutorialAdvance(g);
                    }
                    else if (kind == TUT_CELL && g->selectedHandIndex != -1)
                    {
                        int row = TUTORIAL_SCRIPT[g->tutorialStep].a, col = TUTORIAL_SCRIPT[g->tutorialStep].b;
                        int handIndex = g->selectedHandIndex;
                        tutorialAdvance(g);
                        commitPlacement(g, handIndex, row, col);
                    }
                }
            }
        }
        else if (g->mode == MODE_IDLE) 
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                bool handled = false;

                if (CheckCollisionPointRec(mouse, deckStackRect()))
                {
                    g->deckPopupOpen = true;
                    handled = true;
                }

                for (int i = 0; i < SCRIPT_SLOTS && !handled; i++)
                {
                    if (g->inventory.scripts[i] == NO_ITEM) continue;
                    if (!CheckCollisionPointRec(mouse, scriptSlotRect(i, g->grid.size))) continue;
                    handled = true;

                    ShopItemId id = (ShopItemId)g->inventory.scripts[i];
                    if (id == ITEM_BUFFER_RELOAD)
                    {
                        hand_discardAll(&g->hand);
                        refillHandStep(g);
                        audio_playSlide();
                        inventory_consumeScript(&g->inventory, i);
                    }
                    else if (id == ITEM_WILDCARD)
                    {
                        g->pendingScriptSlot = i;
                        g->mode = MODE_WILDCARD_PICK_RANK;
                    }
                    else if (id == ITEM_MEMORY_FLUSH)
                    {
                        g->pendingScriptSlot = i;
                        g->mode = MODE_MEMORY_FLUSH_PICK_CELL;
                    }
                    else if (id == ITEM_COMPILER_PATCH)
                    {
                        g->pendingScriptSlot = i;
                        g->mode = MODE_COMPILER_PATCH_PICK_RANK;
                    }
                    else if (id == ITEM_NULL_POINTER)
                    {
                        g->pendingScriptSlot = i;
                        g->mode = MODE_NULL_POINTER_PICK_RANK;
                    }
                    else if (id == ITEM_STACK_TRACE)
                    {
                        g->deckPeekActive = true;
                        inventory_consumeScript(&g->inventory, i);
                        setStatus(g, "STACK TRACE: revealing the next 3 cards");
                    }
                    else if (id == ITEM_MULTITHREAD)
                    {
                        g->extraPlaysRemaining++;
                        inventory_consumeScript(&g->inventory, i);
                        setStatus(g, "MULTITHREAD: play 2 cards this turn");
                    }
                    else if (id == ITEM_ROLLBACK)
                    {
                        if (g->hasUndoSnapshot)
                        {
                            g->grid = g->undoGrid;
                            g->hand = g->undoHand;
                            g->deck = g->undoDeck;
                            g->roundScore = g->undoRoundScore;
                            g->extraPlaysRemaining = g->undoExtraPlays;
                            g->turnCounter = g->undoTurnCounter;
                            g->hasUndoSnapshot = false;
                            g->selectedHandIndex = -1;
                            inventory_consumeScript(&g->inventory, i);
                            setStatus(g, "ROLLBACK: last placement undone");
                        }
                        else
                        {
                            setStatus(g, "ROLLBACK: nothing to undo");
                        }
                    }
                    else if (id == ITEM_DEFRAG)
                    {
                        for (int h = 0; h < g->hand.capacity; h++)
                        {
                            g->hand.cards[h].isRotted = false;
                            g->hand.turnsHeld[h] = 0;
                            g->hand.rottenSlot[h] = false;
                        }
                        memorygrid_clearAllRot(&g->grid);
                        inventory_consumeScript(&g->inventory, i);
                        setStatus(g, "DEFRAG: all rot cleared");
                    }
                }

                if (!handled)
                {
                    for (int i = 0; i < g->hand.capacity; i++)
                        if (g->hand.occupied[i] && CheckCollisionPointRec(mouse, handSlotRect(i, g->hand.capacity)))
                        {
                            g->isDragging = true;
                            g->dragHandIndex = i;
                            g->dragStartPos = mouse;
                            handled = true;
                        }
                }

                if (!handled && g->selectedHandIndex != -1)
                {
                    for (int row = 0; row < g->grid.size; row++)
                        for (int col = 0; col < g->grid.size; col++)
                            if (memorygrid_isCellFree(&g->grid, row, col) &&
                                CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size)))
                                commitPlacement(g, g->selectedHandIndex, row, col);
                }
            }
            else if (g->isDragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                Vector2 mouse = GetMousePosition();
                float dx = mouse.x - g->dragStartPos.x, dy = mouse.y - g->dragStartPos.y;
                float dragDist = sqrtf(dx * dx + dy * dy);
                int dragHandIndex = g->dragHandIndex;
                g->isDragging = false;

                if (dragDist < DRAG_CLICK_THRESHOLD)
                {
                    g->selectedHandIndex = (g->selectedHandIndex == dragHandIndex) ? -1 : dragHandIndex;
                }
                else
                {
                    for (int row = 0; row < g->grid.size; row++)
                        for (int col = 0; col < g->grid.size; col++)
                            if (memorygrid_isCellFree(&g->grid, row, col) &&
                                CheckCollisionPointRec(mouse, gridCellRect(row, col, g->grid.size)))
                                commitPlacement(g, dragHandIndex, row, col);
                }
            }
        }

        BeginTextureMode(canvas); 
            Camera2D shakeCam = { .target = { 0, 0 }, .offset = shakeOffset, .rotation = 0.0f, .zoom = 1.0f };
            BeginMode2D(shakeCam);
            background_draw(SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG, (Color){ COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, 14 });

            if (g->phase == PHASE_MAIN_MENU)
            {
                drawTextCentered("STACK OVERFLOW", SCREEN_WIDTH / 2.0f, 220, 48, COLOR_ACCENT);
                drawTextCentered("a tactical card-puzzle roguelite", SCREEN_WIDTH / 2.0f, 280, 18, LIGHTGRAY);
                drawMenuButton(mainMenuButtonRect(0), "START", COLOR_ACCENT);
                drawMenuButton(mainMenuButtonRect(1), "QUIT", COLOR_SLOT_BG);
            }
            else if (g->phase == PHASE_CLASS_SELECT)
            {
                DrawText("CHOOSE YOUR STARTING CLASS", 20, 20, 30, COLOR_ACCENT);
                for (int i = 0; i < CLASS_COUNT; i++)
                {
                    Rectangle box = shopItemRect(i);
                    bool hover = CheckCollisionPointRec(GetMousePosition(), box);
                    DrawRectangleRec(box, hover ? (Color){35,65,55,255} : COLOR_SLOT_BG);
                    DrawRectangleLinesEx(box, hover ? 3 : 2, COLOR_ACCENT);
                    DrawText(CLASS_INFO[i].name, (int)box.x + 12, (int)box.y + 30, 22, RAYWHITE);
                    DrawText(CLASS_INFO[i].description, (int)box.x + 12, (int)box.y + 70, 13, LIGHTGRAY);
                }
            }
            else if (g->phase == PHASE_SHOP)
            {
                DrawText("SHOP", 20, 20, 30, COLOR_ACCENT);
                DrawText(TextFormat("Gold: $%d", g->gold), SCREEN_WIDTH - 200, 25, 24, COLOR_GOLD);
                RoundConfig nextCfg = round_getConfig(g->roundNumber + 1);
                drawTextCentered(TextFormat("Round %d cleared! Next: Round %d - Objective %d pts, Stack Limit %d%s",
                                             g->roundNumber, g->roundNumber + 1, nextCfg.objective, nextCfg.stackLimit,
                                             nextCfg.isBossRound ? " -- BOSS ROUND!" : ""),
                                  SCREEN_WIDTH / 2.0f, 70, 18, nextCfg.isBossRound ? COLOR_BOSS : RAYWHITE);

                for (int slot = 0; slot < SHOP_OFFER_COUNT; slot++)
                {
                    Rectangle box = shopItemRect(slot);
                    ShopItemId id = (ShopItemId)g->shopOffer[slot];
                    const ShopItemInfo *info = shop_getItemInfo(id);

                    bool owned = info->isModule && inventory_hasModule(&g->inventory, id);
                    bool sold = g->shopOfferSold[slot];
                    bool affordable = g->gold >= info->cost;
                    bool clickable = !owned && !sold && affordable;
                    bool hover = clickable && CheckCollisionPointRec(GetMousePosition(), box);
                    Color boxColor = (owned || sold) ? (Color){40,70,50,255} : (affordable ? (hover ? (Color){55,55,55,255} : COLOR_SLOT_BG) : (Color){35,30,30,255});

                    DrawRectangleRec(box, boxColor);
                    DrawRectangleLinesEx(box, hover ? 3 : 2, info->isModule ? COLOR_ACCENT : COLOR_PROMPT);
                    DrawText(info->isModule ? "MODULE" : "SCRIPT", (int)box.x + 12, (int)box.y + 10, 14, GRAY);
                    drawTextWrapped(info->name, box.x + 12, box.y + 30, box.width - 24, 17, 19, RAYWHITE);
                    drawTextWrapped(info->description, box.x + 12, box.y + 58, box.width - 24, 13, 16, LIGHTGRAY);
                    DrawText(owned ? "OWNED" : sold ? "SOLD" : TextFormat("$%d", info->cost),
                             (int)box.x + 12, (int)box.y + 150, 20, (owned || sold) ? COLOR_ACCENT : COLOR_GOLD);
                }

                drawTextCentered("Cards for sale (added to your deck for the rest of the run):",
                                  SCREEN_WIDTH / 2.0f, SHOP_CARD_ORIGIN_Y - 22, 15, GRAY);
                for (int slot = 0; slot < SHOP_CARD_OFFER_COUNT; slot++)
                {
                    Rectangle box = shopCardOfferRect(slot);
                    Card card = g->shopCardOffer[slot];
                    bool sold = g->shopCardOfferSold[slot];
                    int price = shopCardPrice(card);
                    bool affordable = g->gold >= price;

                    bool hover = !sold && affordable && CheckCollisionPointRec(GetMousePosition(), box);
                    if (sold)
                    {
                        DrawRectangleRec(box, COLOR_SLOT_BG);
                        DrawRectangleLinesEx(box, 2, (Color){40,70,50,255});
                        drawTextCentered("SOLD", box.x + box.width / 2.0f, box.y + box.height / 2.0f - 10, 18, COLOR_ACCENT);
                    }
                    else
                    {
                        drawCard(&card, box);
                        DrawRectangleLinesEx(box, hover ? 3 : 2, affordable ? COLOR_GOLD : (Color){70,60,40,255});
                    }
                    drawTextCentered(sold ? "" : TextFormat("$%d", price), box.x + box.width / 2.0f, box.y + box.height + 6, 16,
                                      affordable ? COLOR_GOLD : GRAY);
                }

                Vector2 shopMouse = GetMousePosition();
                DrawText("Modules:", 90, 550, 18, GRAY);
                for (int i = 0; i < MODULE_SLOTS; i++)
                {
                    if (g->inventory.modules[i] == NO_ITEM)
                    {
                        DrawText("-- empty --", 200 + i * 220, 550, 16, GRAY);
                        continue;
                    }
                    Rectangle r = shopOwnedModuleRect(i);
                    bool hover = CheckCollisionPointRec(shopMouse, r);
                    const ShopItemInfo *ownedInfo = shop_getItemInfo((ShopItemId)g->inventory.modules[i]);
                    DrawText(ownedInfo->name, (int)r.x, (int)r.y, 16, COLOR_ACCENT);
                    if (hover)
                        drawTextCentered(TextFormat("click to sell for $%d", sellRefund((ShopItemId)g->inventory.modules[i])),
                                          r.x + r.width / 2.0f, r.y + 20, 12, COLOR_GOLD);
                }
                DrawText("Scripts:", 90, 575, 18, GRAY);
                for (int i = 0; i < SCRIPT_SLOTS; i++)
                {
                    if (g->inventory.scripts[i] == NO_ITEM)
                    {
                        DrawText("-- empty --", 200 + i * 220, 575, 16, GRAY);
                        continue;
                    }
                    Rectangle r = shopOwnedScriptRect(i);
                    bool hover = CheckCollisionPointRec(shopMouse, r);
                    const ShopItemInfo *ownedInfo = shop_getItemInfo((ShopItemId)g->inventory.scripts[i]);
                    DrawText(ownedInfo->name, (int)r.x, (int)r.y, 16, COLOR_PROMPT);
                    if (hover)
                        drawTextCentered(TextFormat("click to sell for $%d", sellRefund((ShopItemId)g->inventory.scripts[i])),
                                          r.x + r.width / 2.0f, r.y + 20, 12, COLOR_GOLD);
                }

                Rectangle rerollBtn = { SCREEN_WIDTH / 2.0f - 260, 610, 150, 50 };
                bool canReroll = g->gold >= g->shopRerollCost;
                drawButton(rerollBtn, TextFormat("REROLL $%d", g->shopRerollCost), COLOR_PROMPT, canReroll, true);

                Rectangle continueBtn = { SCREEN_WIDTH / 2.0f - 100, 610, 200, 50 };
                drawButton(continueBtn, "CONTINUE ->", COLOR_ACCENT, true, true);

                Rectangle editDeckBtn = { SCREEN_WIDTH / 2.0f + 110, 610, 150, 50 };
                drawButton(editDeckBtn, "EDIT DECK", COLOR_PROMPT, true, false);

                if (g->statusMessageTimer > 0.0f)
                    drawTextCentered(g->statusMessage, SCREEN_WIDTH / 2.0f, 590, 16, COLOR_DANGER);

                if (g->shopSwapPromptActive)
                {
                    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 200 });
                    const ShopItemInfo *newInfo = shop_getItemInfo((ShopItemId)g->shopSwapPendingItem);
                    const char *kind = g->shopSwapIsModule ? "Module" : "Script";
                    drawTextCentered(TextFormat("%s SLOTS ARE FULL", g->shopSwapIsModule ? "MODULE" : "SCRIPT"),
                                      SCREEN_WIDTH / 2.0f, 180, 26, COLOR_PROMPT);
                    drawTextCentered(TextFormat("Swap one out to make room for %s (%s, $%d):",
                                                  newInfo->name, kind, newInfo->cost),
                                      SCREEN_WIDTH / 2.0f, 215, 16, LIGHTGRAY);

                    int slotCount = g->shopSwapIsModule ? MODULE_SLOTS : SCRIPT_SLOTS;
                    for (int i = 0; i < slotCount; i++)
                    {
                        int ownedId = g->shopSwapIsModule ? g->inventory.modules[i] : g->inventory.scripts[i];
                        const ShopItemInfo *ownedInfo = shop_getItemInfo((ShopItemId)ownedId);
                        Rectangle r = swapPromptSlotRect(i);
                        bool hover = CheckCollisionPointRec(shopMouse, r);
                        DrawRectangleRec(r, hover ? (Color){60,50,30,255} : COLOR_SLOT_BG);
                        DrawRectangleLinesEx(r, hover ? 3 : 2, COLOR_PROMPT);
                        DrawText(ownedInfo->name, (int)r.x + 12, (int)r.y + 10, 18, RAYWHITE);
                        drawTextWrapped(ownedInfo->description, r.x + 12, r.y + 38, r.width - 24 - 110, 13, 16, LIGHTGRAY);
                        drawTextCentered("click to swap out", r.x + r.width - 90, r.y + r.height / 2.0f - 8, 13, COLOR_DANGER);
                    }

                    drawButton(swapPromptCancelRect(), "CANCEL", COLOR_ACCENT, true, false);
                }
            }
            else if (g->phase == PHASE_EVENT)
            {
                DrawText("SYSTEM EVENT", 20, 20, 30, COLOR_ACCENT);
                DrawText(TextFormat("Gold: $%d", g->gold), SCREEN_WIDTH - 200, 25, 24, COLOR_GOLD);
                drawTextCentered("A bug offers you a trade before the next round loads.", SCREEN_WIDTH / 2.0f, 220, 22, RAYWHITE);
                drawTextCentered("Accept: mark a random undrawn card as Glitched (half value on the grid,",
                                  SCREEN_WIDTH / 2.0f, 255, 16, LIGHTGRAY);
                drawTextCentered(TextFormat("a huge payout when cleared in a combo) in exchange for $%d.", EVENT_GOLD_REWARD),
                                  SCREEN_WIDTH / 2.0f, 275, 16, LIGHTGRAY);

                Rectangle acceptBtn  = { SCREEN_WIDTH / 2.0f - 220, 420, 200, 60 };
                Rectangle declineBtn = { SCREEN_WIDTH / 2.0f + 20,  420, 200, 60 };
                drawButton(acceptBtn, TextFormat("ACCEPT (+$%d)", EVENT_GOLD_REWARD), COLOR_ACCENT, true, true);
                drawButton(declineBtn, "DECLINE", COLOR_PROMPT, true, false);
            }
            else if (g->phase == PHASE_MODULE_CHOICE)
            {
                DrawText("STRUCTURAL REWARD", 20, 20, 30, COLOR_ACCENT);
                drawTextCentered(TextFormat("Every %d rounds, pick a free module", MODULE_CHOICE_EVERY_N_ROUNDS),
                                  SCREEN_WIDTH / 2.0f, 80, 20, RAYWHITE);
                for (int i = 0; i < g->moduleChoiceCount; i++)
                {
                    Rectangle box = shopItemRect(i);
                    bool hover = CheckCollisionPointRec(GetMousePosition(), box);
                    const ShopItemInfo *info = shop_getItemInfo((ShopItemId)g->moduleChoiceOffer[i]);
                    DrawRectangleRec(box, hover ? (Color){35,65,55,255} : COLOR_SLOT_BG);
                    DrawRectangleLinesEx(box, hover ? 3 : 2, COLOR_ACCENT);
                    drawTextWrapped(info->name, box.x + 12, box.y + 30, box.width - 24, 17, 19, RAYWHITE);
                    drawTextWrapped(info->description, box.x + 12, box.y + 58, box.width - 24, 13, 16, LIGHTGRAY);
                    DrawText("FREE", (int)box.x + 12, (int)box.y + 150, 20, COLOR_ACCENT);
                }
            }
            else
            {
                DrawText("STACK OVERFLOW", 20, 20, 30, COLOR_ACCENT);
                DrawText("[H] Help", 300, 12, 13, GRAY);
                DrawText(TextFormat("[TAB] Speed: %gx", g->animSpeed), 300, 28, 13, GRAY);
                DrawText(TextFormat("Round %d", g->roundNumber), SCREEN_WIDTH / 2 - 50, 20, 24, RAYWHITE);
                {
                    const char *turnMsg;
                    Color turnColor;
                    if (g->roundCfg.rottenSlotsActive)
                    {
                        int nextRot = 3 - (g->turnCounter % 3);
                        turnMsg = TextFormat("Turn %d / %d (next rotten slot in %d)", g->turnCounter, g->roundCfg.turnLimit, nextRot);
                        turnColor = COLOR_ROTTED_TINT;
                    }
                    else
                    {
                        bool rotActive = g->turnCounter > HAND_ROT_GRACE_TURNS;
                        turnMsg = rotActive
                            ? TextFormat("Turn %d / %d (cards can rot)", g->turnCounter, g->roundCfg.turnLimit)
                            : TextFormat("Turn %d / %d (rot in %d)", g->turnCounter, g->roundCfg.turnLimit, HAND_ROT_GRACE_TURNS - g->turnCounter + 1);
                        turnColor = rotActive ? COLOR_ROTTED_TINT : GRAY;
                    }
                    drawTextCentered(turnMsg, SCREEN_WIDTH / 2.0f, 48, 14, turnColor);
                }
                DrawText(TextFormat("Gold: $%d", g->gold), SCREEN_WIDTH - 150, 20, 20, COLOR_GOLD);

                DrawText(TextFormat("Stack Score: %d / %d", g->grid.stackScore, g->roundCfg.stackLimit), 20, 60, 20, RAYWHITE);
                DrawText(TextFormat("Round Score: %d / %d", g->roundScore, g->roundCfg.objective), 20, 85, 20, RAYWHITE);
                if (g->grid.diagonalModeFrozenTurns > 0)
                    DrawText(TextFormat("Axis: %s (locked %d)", g->grid.diagonalMode ? "DIAGONAL" : "ROW/COL",
                                          g->grid.diagonalModeFrozenTurns), 20, 110, 18, COLOR_DANGER);
                else if (g->grid.diagonalModeForced)
                    DrawText("Axis: DIAGONAL (forced by boss)", 20, 110, 18, COLOR_BOSS);
                else
                    DrawText(TextFormat("Axis: %s", g->grid.diagonalMode ? "DIAGONAL" : "ROW/COL"), 20, 110, 18, COLOR_ACCENT);

                if (g->cascade.active)
                {
                    float pulse = (sinf((float)GetTime() * 8.0f) + 1.0f) / 2.0f;
                    Color c = { COLOR_MULT.r, COLOR_MULT.g, COLOR_MULT.b, (unsigned char)(180 + pulse * 75) };
                    const char *banner = g->cascade.revealing
                        ? TextFormat("%s!", comboTypeName(g->cascade.revealType))
                        : TextFormat("RESOLVING COMBO CHAIN... (wave %d)", g->cascade.wave + 1);
                    drawTextCentered(banner, SCREEN_WIDTH / 2.0f, 100, 20, c);
                    drawTextCentered("click or press SPACE to skip ahead", SCREEN_WIDTH / 2.0f, 122, 13, GRAY);
                }

                drawPile(discardStackRect(), g->deck.discardCount, g->deck.discardCount > 0,
                          g->deck.discardCount > 0 ? g->deck.discardPile[g->deck.discardCount - 1] : (Card){ 0 });
                drawPile(deckStackRect(), g->deck.count, false, (Card){ 0 });
                if (CheckCollisionPointRec(GetMousePosition(), deckStackRect()))
                    drawTextCentered("click", deckStackRect().x + PILE_CARD_W / 2.0f, deckStackRect().y - 16, 12, COLOR_PROMPT);

                if (g->currentBossType != BOSS_NONE)
                    drawTextWrapped(bossHudMessage(g), SCREEN_WIDTH - 420, 82, 400, 13, 15, COLOR_BOSS);
                if (g->grid.disabledComboType != COMBO_NONE)
                    DrawText(TextFormat("MUTATOR: %s disabled this round", comboTypeName(g->grid.disabledComboType)),
                              SCREEN_WIDTH - 420, 100, 13, COLOR_DANGER);
                if (g->roundCfg.unstableDeckActive || g->roundCfg.extendedLockActive || g->roundCfg.memoryCorruptionActive)
                {
                    char tierLine[128] = "TIER: ";
                    if (g->roundCfg.unstableDeckActive) strncat(tierLine, "Unstable Deck  ", sizeof(tierLine) - strlen(tierLine) - 1);
                    if (g->roundCfg.extendedLockActive) strncat(tierLine, "Extended Lock  ", sizeof(tierLine) - strlen(tierLine) - 1);
                    if (g->roundCfg.memoryCorruptionActive) strncat(tierLine, "Memory Corruption", sizeof(tierLine) - strlen(tierLine) - 1);
                    DrawText(tierLine, SCREEN_WIDTH - 420, 118, 13, COLOR_PROMPT);
                }
                if (g->deckPeekActive)
                {
                    char peek[128] = "STACK TRACE next: ";
                    for (int i = 0; i < 3 && i < g->deck.count; i++)
                    {
                        Card c = g->deck.cards[g->deck.count - 1 - i];
                        char piece[24];
                        snprintf(piece, sizeof(piece), "%s-%c ", card_rankLabel(c.rank), card_suitName(c.suit)[0]);
                        strncat(peek, piece, sizeof(peek) - strlen(peek) - 1);
                    }
                    DrawText(peek, SCREEN_WIDTH - 420, 136, 13, COLOR_PROMPT);
                }


                DrawText("Danger Meter (Stack Limit)", 20, 134, 13, GRAY);
                Rectangle gaugeRect = { 20, 150, 220, 14 };
                float stackRatio = (float)g->grid.stackScore / (float)g->roundCfg.stackLimit;
                float gaugeFill = stackRatio < 0.0f ? 0.0f : (stackRatio > 1.0f ? 1.0f : stackRatio);
                Color gaugeColor = (stackRatio < 0.5f) ? (Color){ 90, 200, 110, 255 }
                                  : (stackRatio < 0.8f) ? (Color){ 230, 200, 60, 255 }
                                  : COLOR_DANGER;
                DrawRectangleRec(gaugeRect, COLOR_GAUGE_BG);
                DrawRectangleRec((Rectangle){ gaugeRect.x, gaugeRect.y, gaugeRect.width * gaugeFill, gaugeRect.height }, gaugeColor);
                DrawRectangleLinesEx(gaugeRect, 1, RAYWHITE);

                DrawText("Round Score Progress", 20, 170, 13, GRAY);
                Rectangle roundGaugeRect = { 20, 186, 220, 14 };
                float objectiveRatio = (float)g->roundScore / (float)g->roundCfg.objective;
                float objectiveFill = objectiveRatio < 0.0f ? 0.0f : (objectiveRatio > 1.0f ? 1.0f : objectiveRatio);
                DrawRectangleRec(roundGaugeRect, COLOR_GAUGE_BG);
                DrawRectangleRec((Rectangle){ roundGaugeRect.x, roundGaugeRect.y, roundGaugeRect.width * objectiveFill, roundGaugeRect.height },
                                  (Color){ 90, 200, 110, 255 });
                DrawRectangleLinesEx(roundGaugeRect, 1, RAYWHITE);

                for (int i = 0; i < MODULE_SLOTS; i++)
                {
                    Rectangle slot = moduleSlotRect(i, g->grid.size);
                    DrawRectangleRec(slot, COLOR_SLOT_BG);
                    DrawRectangleLinesEx(slot, 2, COLOR_ACCENT);
                    const char *label = g->inventory.modules[i] == NO_ITEM ? "empty"
                        : shop_getItemInfo((ShopItemId)g->inventory.modules[i])->name;
                    DrawText(label, (int)slot.x + 8, (int)slot.y + 8, 14, RAYWHITE);
                    if ((ShopItemId)g->inventory.modules[i] == ITEM_TRY_CATCH)
                        DrawText(g->inventory.tryCatchChargeAvailable ? "charged" : "used", (int)slot.x + 8, (int)slot.y + 70, 13, GRAY);
                }
                for (int i = 0; i < SCRIPT_SLOTS; i++)
                {
                    Rectangle slot = scriptSlotRect(i, g->grid.size);
                    DrawRectangleRec(slot, COLOR_SLOT_BG);
                    DrawRectangleLinesEx(slot, 2, COLOR_PROMPT);
                    const char *label = g->inventory.scripts[i] == NO_ITEM ? "empty"
                        : shop_getItemInfo((ShopItemId)g->inventory.scripts[i])->name;
                    DrawText(label, (int)slot.x + 8, (int)slot.y + 8, 14, RAYWHITE);
                    if (g->inventory.scripts[i] != NO_ITEM && g->mode == MODE_IDLE)
                        DrawText("click to use", (int)slot.x + 8, (int)slot.y + 70, 12, GRAY);
                }

                {
                    Vector2 hudMouse = GetMousePosition();
                    ShopItemId hoveredId = (ShopItemId)NO_ITEM;
                    Rectangle hoveredSlot = { 0 };
                    for (int i = 0; i < MODULE_SLOTS; i++)
                    {
                        Rectangle slot = moduleSlotRect(i, g->grid.size);
                        if (g->inventory.modules[i] != NO_ITEM && CheckCollisionPointRec(hudMouse, slot))
                        {
                            hoveredId = (ShopItemId)g->inventory.modules[i];
                            hoveredSlot = slot;
                        }
                    }
                    for (int i = 0; i < SCRIPT_SLOTS; i++)
                    {
                        Rectangle slot = scriptSlotRect(i, g->grid.size);
                        if (g->inventory.scripts[i] != NO_ITEM && CheckCollisionPointRec(hudMouse, slot))
                        {
                            hoveredId = (ShopItemId)g->inventory.scripts[i];
                            hoveredSlot = slot;
                        }
                    }
                    if (hoveredId != (ShopItemId)NO_ITEM)
                    {
                        const ShopItemInfo *info = shop_getItemInfo(hoveredId);
                        Rectangle tip = { hoveredSlot.x, hoveredSlot.y + hoveredSlot.height + 6, 220, 60 };
                        DrawRectangleRec(tip, COLOR_PANEL);
                        DrawRectangleLinesEx(tip, 2, COLOR_PROMPT);
                        drawTextWrapped(info->description, tip.x + 8, tip.y + 8, tip.width - 16, 13, 16, RAYWHITE);
                    }
                }

                drawComboLegend(20, 375);

                Vector2 mouseNow = GetMousePosition();

                bool hasPreview = false;
                int previewRow = -1, previewCol = -1;
                PlacementPreview preview = { 0 };

                bool selectedIsHidden = g->selectedHandIndex != -1 && g->hand.cards[g->selectedHandIndex].isHidden;
                if (g->mode == MODE_IDLE && g->selectedHandIndex != -1 && !selectedIsHidden)
                {
                    for (int row = 0; row < g->grid.size && !hasPreview; row++)
                        for (int col = 0; col < g->grid.size && !hasPreview; col++)
                            if (memorygrid_isCellFree(&g->grid, row, col) &&
                                CheckCollisionPointRec(mouseNow, gridCellRect(row, col, g->grid.size)))
                            {
                                previewRow = row;
                                previewCol = col;
                                hasPreview = true;
                            }
                }
                if (hasPreview)
                    preview = computePlacementPreview(g, g->hand.cards[g->selectedHandIndex], previewRow, previewCol);

                if (hasPreview && preview.wouldCrash)
                {
                    float pulse = (sinf((float)GetTime() * 10.0f) + 1.0f) / 2.0f;
                    Color flash = { 255, 255, 255, (unsigned char)(pulse * 220) };
                    DrawRectangleLinesEx(gaugeRect, 2, flash);
                }

                for (int row = 0; row < g->grid.size; row++)
                {
                    for (int col = 0; col < g->grid.size; col++)
                    {
                        Rectangle cell = gridCellRect(row, col, g->grid.size);
                        drawCard(&g->grid.cards[row][col], cell);

                        if (hasPreview)
                        {
                            Color tint = { 0 };
                            bool hasTint = false;
                            for (int l = 0; l < preview.lineCount; l++)
                            {
                                if (!preview.lines[l].active) continue;
                                if (!classLineContainsCell(&preview.lines[l], previewRow, previewCol)) continue;
                                if (!classLineContainsCell(&preview.lines[l], row, col)) continue;
                                if (preview.lines[l].type != COMBO_NONE) { tint = COLOR_COMBO_GREEN; hasTint = true; }
                                else if (preview.lines[l].nearCombo && !hasTint) { tint = COLOR_COMBO_ORANGE; hasTint = true; }
                            }
                            if (hasTint)
                            {
                                tint.a = 90;
                                DrawRectangleRec(cell, tint);
                            }
                        }

                        bool isSwapFirst = (g->mode == MODE_AWAITING_SWAP_SECOND &&
                                             row == g->swapFirstRow && col == g->swapFirstCol);
                        bool isDropTarget = g->isDragging && memorygrid_isCellFree(&g->grid, row, col) &&
                                             CheckCollisionPointRec(mouseNow, cell);
                        bool isPreviewTarget = hasPreview && row == previewRow && col == previewCol;

                        bool isQueenLockFirst = (g->mode == MODE_AWAITING_QUEEN_LOCK_SECOND &&
                                                   row == g->queenLockFirstRow && col == g->queenLockFirstCol);
                        bool isQueenLockCandidate = false;
                        if (g->mode == MODE_AWAITING_QUEEN_LOCK_FIRST || g->mode == MODE_AWAITING_QUEEN_LOCK_SECOND)
                        {
                            int neighbors[8][2];
                            int count = memorygrid_queenNeighbors(g->queenRow, g->queenCol, g->grid.size, g->roundCfg.extendedLockActive, neighbors);
                            for (int n = 0; n < count; n++)
                                if (neighbors[n][0] == row && neighbors[n][1] == col) isQueenLockCandidate = true;
                        }

                        if (isSwapFirst || isQueenLockFirst)
                            DrawRectangleLinesEx(cell, 3, COLOR_PROMPT);
                        else if (isDropTarget || isPreviewTarget) DrawRectangleLinesEx(cell, 3, COLOR_ACCENT);
                        else if (isQueenLockCandidate) DrawRectangleLinesEx(cell, 3, COLOR_DANGER);
                        else DrawRectangleLinesEx(cell, 1, COLOR_FREE_CELL);

                        if (g->grid.size == 3 && row == L1_CACHE_ROW && col == L1_CACHE_COL)
                        {
                            DrawRectangleLinesEx((Rectangle){ cell.x - 3, cell.y - 3, cell.width + 6, cell.height + 6 },
                                                   2, COLOR_L1_CACHE);
                            drawTextCentered("1.5x", cell.x + cell.width / 2.0f, cell.y - 16, 12, COLOR_L1_CACHE);
                        }

                        if (row == g->grid.trapRow && col == g->grid.trapCol)
                        {
                            DrawRectangleLinesEx((Rectangle){ cell.x - 3, cell.y - 3, cell.width + 6, cell.height + 6 },
                                                   2, COLOR_DANGER);
                            drawTextCentered("TRAP", cell.x + cell.width / 2.0f, cell.y - 16, 12, COLOR_DANGER);
                        }

                        if (g->comboFlashTimer > 0.0f && g->comboFlashCell[row][col])
                        {
                            float flashT = g->comboFlashTimer / COMBO_FLASH_DURATION;
                            Color fillColor = Fade(g->comboFlashColor, 0.35f * flashT);
                            Color lineColor = g->comboFlashColor;
                            lineColor.a = (unsigned char)(220.0f * flashT);
                            DrawRectangleRec(cell, fillColor);
                            DrawRectangleLinesEx(cell, 4, lineColor);
                        }

                        if (g->grid.cards[row][col].isLocked)
                        {
                            Texture2D lock = uitex_getPadlock();
                            Rectangle badge = { cell.x + cell.width - LOCK_BADGE - 4, cell.y + 4, LOCK_BADGE, LOCK_BADGE };
                            DrawTexturePro(lock, (Rectangle){ 0, 0, (float)lock.width, (float)lock.height },
                                            badge, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        }
                    }
                }

                if (hasPreview)
                {
                    Color scoreColor = preview.wouldCrash ? COLOR_DANGER : COLOR_ACCENT;
                    DrawText(TextFormat("Preview Stack Score: %d -> %d", preview.scoreBefore, preview.scoreAfter),
                              20, 178, 16, scoreColor);
                    if (preview.isAce)
                        DrawText(TextFormat("Ace would count as: %d", preview.aceAsEleven ? 11 : 1), 20, 198, 16, COLOR_PROMPT);
                    if (preview.comboPointsThroughTarget > 0)
                        DrawText(TextFormat("Would score: +%d pts", preview.comboPointsThroughTarget),
                                  20, preview.isAce ? 218 : 198, 16, COLOR_COMBO_GREEN);
                }

                float dangerRatio = (float)g->grid.stackScore / (float)g->roundCfg.stackLimit;
                if (dangerRatio >= 0.8f)
                {
                    float pulse = (sinf((float)GetTime() * 6.0f) + 1.0f) / 2.0f;
                    Color flash = { COLOR_DANGER.r, COLOR_DANGER.g, COLOR_DANGER.b, (unsigned char)(pulse * 180) };
                    DrawRectangleLinesEx(gridBoundsRect(g->grid.size), 5, flash);
                }

                for (int i = 0; i < g->hand.capacity; i++)
                {
                    Rectangle slot = handSlotRect(i, g->hand.capacity);
                    bool beingDragged = g->isDragging && i == g->dragHandIndex;
                    if (g->hand.occupied[i] && !beingDragged)
                    {
                        float lift = g->handHoverLift[i];
                        float scale = 1.0f + 0.10f * lift;
                        Rectangle liftedSlot = {
                            slot.x - (slot.width * scale - slot.width) / 2.0f,
                            slot.y - (slot.height * scale - slot.height) / 2.0f - 14.0f * lift,
                            slot.width * scale,
                            slot.height * scale
                        };
                        drawCard(&g->hand.cards[i], liftedSlot);
                        if (i == g->selectedHandIndex) DrawRectangleLinesEx(liftedSlot, 3, COLOR_SELECTED);
                        if (g->hand.cards[i].isRotted)
                            drawTextCentered("ROTTED", slot.x + slot.width / 2.0f, slot.y + slot.height + 4, 12, COLOR_ROTTED_TINT);
                        else if (g->hand.turnsHeld[i] == HAND_ROT_TURNS - 1)
                            drawTextCentered("last turn!", slot.x + slot.width / 2.0f, slot.y + slot.height + 4, 12, COLOR_PROMPT);
                        if (g->hand.cards[i].isEphemeral)
                        {
                            DrawRectangleLinesEx(liftedSlot, 3, COLOR_DANGER);
                            drawTextCentered(TextFormat("PLAY IN %d!", g->ephemeralTurnsLeft),
                                              slot.x + slot.width / 2.0f, slot.y - 16, 12, COLOR_DANGER);
                        }
                    }
                    else if (g->hand.occupied[i])
                    {
                        DrawRectangleLinesEx(slot, 2, COLOR_FREE_CELL);
                    }
                    else
                    {
                        Texture2D empty = cardtex_getEmpty();
                        DrawTexturePro(empty, (Rectangle){ 0, 0, (float)empty.width, (float)empty.height },
                                        slot, (Vector2){ 0, 0 }, 0.0f, WHITE);
                    }
                }

                if (g->isDragging)
                {
                    Rectangle floating = { mouseNow.x - CARD_DISP_W / 2.0f, mouseNow.y - CARD_DISP_H / 2.0f,
                                            CARD_DISP_W, CARD_DISP_H };
                    drawCard(&g->hand.cards[g->dragHandIndex], floating);
                }

                if (g->tutorialActive)
                {
                    if (g->mode == MODE_AWAITING_FLIP_CHOICE)
                    {
                        drawButton(flipChoiceRect(0), "YES", COLOR_ACCENT, true, true);
                        drawButton(flipChoiceRect(1), "NO", RAYWHITE, true, false);
                    }
                    drawTutorialOverlay(g);
                }
                else if (g->mode == MODE_AWAITING_SWAP_FIRST || g->mode == MODE_AWAITING_SWAP_SECOND)
                {
                    const char *msg = (g->mode == MODE_AWAITING_SWAP_FIRST)
                        ? "JACK: click a cell to swap (right-click to skip)"
                        : "JACK: click the second cell to swap with (right-click to cancel)";
                    DrawRectangle(0, 70, SCREEN_WIDTH, 34, COLOR_PANEL);
                    drawTextCentered(msg, SCREEN_WIDTH / 2.0f, 76, 22, COLOR_PROMPT);
                }
                else if (g->mode == MODE_AWAITING_FLIP_CHOICE)
                {
                    const char *msg = g->grid.diagonalMode
                        ? "KING: switch back to ROW/COLUMN detection? Locks for 2 turns"
                        : "KING: switch to DIAGONAL-ONLY detection? Locks for 2 turns";
                    DrawRectangle(0, 70, SCREEN_WIDTH, 76, COLOR_PANEL);
                    drawTextCentered(msg, SCREEN_WIDTH / 2.0f, 76, 22, COLOR_PROMPT);
                    drawButton(flipChoiceRect(0), "YES", COLOR_ACCENT, true, true);
                    drawButton(flipChoiceRect(1), "NO", RAYWHITE, true, false);
                }
                else if (g->mode == MODE_AWAITING_QUEEN_LOCK_FIRST || g->mode == MODE_AWAITING_QUEEN_LOCK_SECOND)
                {
                    const char *msg = (g->mode == MODE_AWAITING_QUEEN_LOCK_FIRST)
                        ? "QUEEN: click a neighbor cell to lock (right-click to lock none)"
                        : "QUEEN: click a second neighbor to lock (right-click to lock just one)";
                    DrawRectangle(0, 70, SCREEN_WIDTH, 34, COLOR_PANEL);
                    drawTextCentered(msg, SCREEN_WIDTH / 2.0f, 76, 22, COLOR_PROMPT);
                }
                else if (g->mode == MODE_UNSTABLE_DECK_PICK)
                {
                    DrawRectangle(0, 260, SCREEN_WIDTH, CARD_DISP_H + 60, COLOR_PANEL);
                    drawTextCentered("UNSTABLE DECK: the deck glitched - pick one of these two cards",
                                      SCREEN_WIDTH / 2.0f, 268, 20, COLOR_PROMPT);
                    Rectangle rectA = unstablePickRect(0), rectB = unstablePickRect(1);
                    drawCard(&g->unstableDeckOptionA, rectA);
                    drawCard(&g->unstableDeckOptionB, rectB);
                    DrawRectangleLinesEx(rectA, 2, COLOR_ACCENT);
                    DrawRectangleLinesEx(rectB, 2, COLOR_ACCENT);
                }
                else if (g->mode == MODE_WILDCARD_PICK_RANK || g->mode == MODE_COMPILER_PATCH_PICK_RANK ||
                         g->mode == MODE_NULL_POINTER_PICK_RANK)
                {
                    const char *msg =
                        (g->mode == MODE_WILDCARD_PICK_RANK) ? "WILDCARD: pick a value (right-click to cancel)" :
                        (g->mode == MODE_COMPILER_PATCH_PICK_RANK) ? "COMPILER PATCH: pick a value to inject (right-click to cancel)" :
                        "NULL POINTER: pick a value to purge (right-click to cancel)";
                    DrawRectangle(0, 260, SCREEN_WIDTH, RANK_PICK_H + 60, COLOR_PANEL);
                    drawTextCentered(msg, SCREEN_WIDTH / 2.0f, 268, 20, COLOR_PROMPT);
                    for (int i = 0; i < 13; i++)
                    {
                        Rectangle r = rankPickRect(i);
                        Card preview = card_make(SUIT_SPADE, WILDCARD_RANKS[i]);
                        if (g->mode == MODE_COMPILER_PATCH_PICK_RANK) preview.isSpecial = true;
                        drawCard(&preview, r);
                        DrawRectangleLinesEx(r, 1, COLOR_FREE_CELL);
                    }
                }
                else if (g->mode == MODE_WILDCARD_PICK_CELL)
                {
                    DrawRectangle(0, 70, SCREEN_WIDTH, 34, COLOR_PANEL);
                    drawTextCentered("WILDCARD: click a free cell to place it (right-click to cancel)", SCREEN_WIDTH / 2.0f, 76, 20, COLOR_PROMPT);
                }
                else if (g->mode == MODE_MEMORY_FLUSH_PICK_CELL)
                {
                    DrawRectangle(0, 70, SCREEN_WIDTH, 34, COLOR_PANEL);
                    drawTextCentered("MEMORY FLUSH: click any cell to destroy it (right-click to cancel)", SCREEN_WIDTH / 2.0f, 76, 20, COLOR_PROMPT);
                }

                for (int i = 0; i < MAX_FLYING_CARDS; i++)
                {
                    if (!g->flyingCards[i].active || g->flyingCards[i].elapsed < 0.0f) continue;
                    float t = g->flyingCards[i].elapsed / FLY_DURATION;
                    if (t > 1.0f) t = 1.0f;
                    float eased = easeOutCubic(t);
                    Vector2 pos = {
                        g->flyingCards[i].startPos.x + (g->flyingCards[i].endPos.x - g->flyingCards[i].startPos.x) * eased,
                        g->flyingCards[i].startPos.y + (g->flyingCards[i].endPos.y - g->flyingCards[i].startPos.y) * eased
                    };
                    Vector2 size = {
                        g->flyingCards[i].startSize.x + (g->flyingCards[i].endSize.x - g->flyingCards[i].startSize.x) * eased,
                        g->flyingCards[i].startSize.y + (g->flyingCards[i].endSize.y - g->flyingCards[i].startSize.y) * eased
                    };
                    drawCard(&g->flyingCards[i].card, (Rectangle){ pos.x, pos.y, size.x, size.y });
                }

                if (g->scorePopup.active)
                {
                    float t = g->scorePopup.elapsed / SCORE_POPUP_DURATION;
                    float scale = t < 0.25f ? easeOutBack(t / 0.25f) : 1.0f;
                    if (scale < 0.0f) scale = 0.0f;
                    float alpha = t > 0.7f ? (1.0f - (t - 0.7f) / 0.3f) : 1.0f;
                    if (alpha < 0.0f) alpha = 0.0f;

                    Vector2 center = { SCREEN_WIDTH / 2.0f, 225.0f };
                    int chipsSize = (int)(42 * scale);
                    drawTextCentered(TextFormat("+%d", g->scorePopup.chips), center.x, center.y - chipsSize / 2.0f, chipsSize, Fade(COLOR_CHIPS, alpha));
                }
            }

            if (g->statusMessageTimer > 0.0f)
                drawTextCentered(g->statusMessage, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - CARD_DISP_H - 70, 18, COLOR_DANGER);

            if (g->deckPopupOpen)
            {
                Card sorted[DECK_MAX_SIZE];
                int n = g->deck.count;
                for (int i = 0; i < n; i++) sorted[i] = g->deck.cards[i];
                for (int i = 1; i < n; i++)
                {
                    Card key = sorted[i];
                    int keyRank = rankSortIndex(key.rank);
                    int j = i - 1;
                    while (j >= 0 && (rankSortIndex(sorted[j].rank) > keyRank ||
                           (rankSortIndex(sorted[j].rank) == keyRank && sorted[j].suit > key.suit)))
                    {
                        sorted[j + 1] = sorted[j];
                        j--;
                    }
                    sorted[j + 1] = key;
                }

                int suitCounts[SUIT_COUNT] = { 0 };
                for (int i = 0; i < n; i++) suitCounts[sorted[i].suit]++;

                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 200 });
                drawTextCentered(TextFormat("REMAINING IN DECK: %d cards", n), SCREEN_WIDTH / 2.0f, 70, 26, COLOR_ACCENT);
                for (int i = 0; i < n; i++)
                    drawCard(&sorted[i], deckPopupCardRect(i));

                int rows = (n + DECK_POPUP_COLS - 1) / DECK_POPUP_COLS;
                if (rows < 1) rows = 1;
                float summaryY = DECK_POPUP_ORIGIN_Y + rows * (DECK_POPUP_CARD_H + DECK_POPUP_CARD_GAP) + 20;
                drawTextCentered(TextFormat("Hearts: %d    Diamonds: %d    Clubs: %d    Spades: %d",
                                             suitCounts[SUIT_HEART], suitCounts[SUIT_DIAMOND],
                                             suitCounts[SUIT_CLUB], suitCounts[SUIT_SPADE]),
                                  SCREEN_WIDTH / 2.0f, summaryY, 18, LIGHTGRAY);
                drawTextCentered("click anywhere to close", SCREEN_WIDTH / 2.0f, summaryY + 26, 14, GRAY);
            }

            if (g->deckEditOpen)
            {
                Card composition[DECK_MAX_SIZE];
                int n = buildFullDeckComposition(g, composition);
                int cost = deckEditCost(g);

                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 210 });
                drawTextCentered(TextFormat("EDIT DECK: %d cards - click a card to %s for $%d",
                                              n, g->deckEditUpgradeMode ? "upgrade" : "remove", cost),
                                  SCREEN_WIDTH / 2.0f, 40, 20, COLOR_ACCENT);

                Rectangle removeBtn = { SCREEN_WIDTH / 2.0f - 220, 95, 200, 40 };
                Rectangle upgradeBtn = { SCREEN_WIDTH / 2.0f + 20, 95, 200, 40 };
                Rectangle closeBtn = { SCREEN_WIDTH - 130, 20, 100, 40 };
                drawButton(removeBtn, "REMOVE MODE", COLOR_DANGER, true, !g->deckEditUpgradeMode);
                drawButton(upgradeBtn, "UPGRADE MODE", COLOR_ACCENT, true, g->deckEditUpgradeMode);
                drawButton(closeBtn, "CLOSE", COLOR_PROMPT, true, false);

                for (int i = 0; i < n; i++)
                {
                    Rectangle r = deckPopupCardRect(i);
                    drawCard(&composition[i], r);
                    if (CheckCollisionPointRec(GetMousePosition(), r))
                        DrawRectangleLinesEx(r, 2, COLOR_PROMPT);
                }
            }

            if (g->phase == PHASE_GAME_OVER)
            {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 180 });
                const char *line1 = (g->gameOverReason == REASON_CRASH) ? "FATAL ERROR: Memory Limit Exceeded"
                    : (g->gameOverReason == REASON_QUOTA) ? "FATAL ERROR: Quota Not Met"
                    : "FATAL ERROR: Turn Limit Reached";
                drawTextCentered(line1, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 60, 26, COLOR_DANGER);

                const char *line2 = (g->gameOverReason == REASON_CRASH)
                    ? TextFormat("Stack Score hit %d, over the Round %d Stack Limit of %d.",
                                  g->grid.stackScore, g->roundNumber, g->roundCfg.stackLimit)
                    : (g->gameOverReason == REASON_QUOTA)
                    ? TextFormat("Deck ran out on Round %d with %d / %d points - short of the objective.",
                                  g->roundNumber, g->roundScore, g->roundCfg.objective)
                    : TextFormat("Turn %d limit hit on Round %d with %d / %d points - short of the objective.",
                                  g->roundCfg.turnLimit, g->roundNumber, g->roundScore, g->roundCfg.objective);
                drawTextCentered(line2, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 25, 17, LIGHTGRAY);

                const char *tip = (g->gameOverReason == REASON_CRASH)
                    ? "Tip: a placement is safe as long as the Stack Score gauge stays under the red line."
                    : (g->gameOverReason == REASON_QUOTA)
                    ? "Tip: clear combos to discard cards and refill your hand - an empty deck ends the round."
                    : "Tip: score big combos early - the turn counter keeps climbing whether you score or not.";
                drawTextCentered(tip, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 5, 14, GRAY);

                drawTextCentered("Press R to restart", SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 40, 20, RAYWHITE);
            }

            if (g->helpOverlayOpen)
            {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 215 });
                drawTextCentered("HOW TO PLAY", SCREEN_WIDTH / 2.0f, 40, 28, COLOR_ACCENT);

                int x = 90, y = 90, lh = 21;
                DrawText("GOAL", x, y, 17, COLOR_PROMPT); y += lh;
                DrawText("Reach the Round Score objective before the deck or the turn limit runs out.", x, y, 14, RAYWHITE); y += lh + 8;

                DrawText("STACK SCORE - the way you lose", x, y, 17, COLOR_DANGER); y += lh;
                DrawText("Every card on the 3x3 grid adds its value to the Stack Score.", x, y, 14, RAYWHITE); y += lh;
                DrawText("If it ever exceeds the Stack Limit, it's an instant game over -", x, y, 14, RAYWHITE); y += lh;
                DrawText("watch the red Danger Meter under the grid.", x, y, 14, RAYWHITE); y += lh;
                DrawText("Face cards (J/Q/K) and Aces are special - see below.", x, y, 14, GRAY); y += lh + 8;

                DrawText("COMBOS - the way you score, and the way you stay safe", x, y, 17, COLOR_PROMPT); y += lh;
                DrawText("Line up 3 cells in any row, column, or diagonal (see the legend", x, y, 14, RAYWHITE); y += lh;
                DrawText("in the bottom-left corner during play):", x, y, 14, RAYWHITE); y += lh;
                DrawText("  Same Suit (100+ pts)", x + 10, y, 14, COLOR_FLASH_SAME_SUIT); y += lh;
                DrawText("  Straight, consecutive ranks (250+ pts)", x + 10, y, 14, COLOR_FLASH_STRAIGHT); y += lh;
                DrawText("  Same rank / Brelan (400+ pts)", x + 10, y, 14, COLOR_FLASH_BRELAN); y += lh;
                DrawText("  Straight Flush, straight + same suit (1000+ pts)", x + 10, y, 14, COLOR_FLASH_STRAIGHT_FLUSH); y += lh;
                DrawText("Every combo type scores extra the higher the cards involved -", x, y, 14, RAYWHITE); y += lh;
                DrawText("riskier, since those same cards raise your Stack Score more.", x, y, 14, RAYWHITE); y += lh;
                DrawText("A combo discards and refills its cells - that LOWERS your Stack", x, y, 14, RAYWHITE); y += lh;
                DrawText("Score. Straight and N-of-a-Kind also halve your WHOLE Stack Score,", x, y, 14, RAYWHITE); y += lh;
                DrawText("once per turn no matter how many of those lines matched at once.", x, y, 14, RAYWHITE); y += lh;
                DrawText("Hovering a card tints cells green (would combo) or orange", x, y, 14, GRAY); y += lh;
                DrawText("(one card away) so you can see a line forming before you commit.", x, y, 14, GRAY); y += lh + 8;

                int x2 = 620, y2 = 90;
                DrawText("FACE CARDS & ACES", x2, y2, 17, COLOR_PROMPT); y2 += lh;
                DrawText("Jack: swap two cells on the grid.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("Queen: resets a neighbor cell's value to 0 and locks it", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("  (stays out of the Stack Score until unlocked).", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("King: flips whether rows/columns or diagonals are checked -", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("  diagonal combos score double while flipped.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("Ace: counts as 1 or 11, whichever keeps you safer.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("Center cell has a gold border: any combo through it", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("  scores 1.5x.", x2, y2, 14, RAYWHITE); y2 += lh + 8;

                DrawText("SHOP", x2, y2, 17, COLOR_PROMPT); y2 += lh;
                DrawText("Modules are permanent (2 slots); Scripts are one-use", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("(2 slots, duplicates OK). Buying while full offers a swap;", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("owned ones can be sold back for half their price. Click a module", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("or script during play to see a reminder of what it does.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("EDIT DECK (in the shop) permanently removes or upgrades a card.", x2, y2, 14, RAYWHITE); y2 += lh + 8;

                DrawText("OTHER HUD NUMBERS", x2, y2, 17, COLOR_PROMPT); y2 += lh;
                DrawText("Turn limit: run out of turns before the objective and the round", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("  is lost, same as busting the Stack Limit.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("Rot (rounds 1-4): a card left too long in your hand starts", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("  counting double if placed.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("Rot (round 5+): a new grid cell OR hand slot rots every 3 turns", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("  instead - any card that ever lands there counts double too.", x2, y2, 14, RAYWHITE); y2 += lh;
                DrawText("Mint-green cards are Glitched: half value on the grid, but a", x2, y2, 14, COLOR_SPECIAL_TINT); y2 += lh;
                DrawText("  big score bonus if you clear them in a combo.", x2, y2, 14, COLOR_SPECIAL_TINT); y2 += lh;
                DrawText("Press TAB anytime to speed up animations.", x2, y2, 14, GRAY); y2 += lh;

                drawTextCentered("click anywhere, or press H / ESC, to close", SCREEN_WIDTH / 2.0f,
                                  SCREEN_HEIGHT - 40, 15, GRAY);
            }

            if (g->isPaused)
            {
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){ 0, 0, 0, 190 });
                drawTextCentered("PAUSED", SCREEN_WIDTH / 2.0f, 220, 40, COLOR_ACCENT);
                drawMenuButton(pauseButtonRect(0), "RESUME", COLOR_ACCENT);
                drawMenuButton(pauseButtonRect(1), "RESTART RUN", COLOR_PROMPT);
                drawMenuButton(pauseButtonRect(2), "QUIT", COLOR_DANGER);
                drawMenuButton(pauseButtonRect(3), TextFormat("ANIMATION SPEED: %gx", g->animSpeed), COLOR_ACCENT);
            }
            EndMode2D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle canvasSrc = { 0, 0, (float)canvas.texture.width, -(float)canvas.texture.height };
            Rectangle canvasDst = { g_renderOffset.x, g_renderOffset.y,
                                     SCREEN_WIDTH * g_renderScale, SCREEN_HEIGHT * g_renderScale };
            DrawTexturePro(canvas.texture, canvasSrc, canvasDst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    background_unload();
    fonts_unloadAll();
    audio_unloadAll();
    uitex_unloadAll();
    cardtex_unloadAll();
    CloseWindow();
    return 0;
}
