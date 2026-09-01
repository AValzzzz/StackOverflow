#ifndef MEMORY_GRID_H
#define MEMORY_GRID_H

#include <stdbool.h>

#include "card.h"
#include "deck.h"

#define GRID_SIZE_MIN 3
#define GRID_SIZE_MAX 4
#define LINE_LEN_MAX   GRID_SIZE_MAX
#define LINE_COUNT_MAX (2 * GRID_SIZE_MAX + 2)

#define L1_CACHE_ROW 1
#define L1_CACHE_COL 1

typedef enum ComboType {
    COMBO_NONE,
    COMBO_SAME_SUIT,    
    COMBO_STRAIGHT,     
    COMBO_BRELAN,        
    COMBO_STRAIGHT_FLUSH 
} ComboType;

typedef struct ComboResult {
    int  totalScoreGained;
    int  sameSuitMatches;
    int  straightMatches;
    int  brelanMatches;
    int  straightFlushMatches;
    bool gridWasWiped;
    bool cellInvolved[GRID_SIZE_MAX][GRID_SIZE_MAX];
} ComboResult;

#define BANNED_AXIS_NONE      0
#define BANNED_AXIS_ROWS      1
#define BANNED_AXIS_COLS      2
#define BANNED_AXIS_DIAGONALS 3

typedef struct MemoryGrid {
    Card cards[GRID_SIZE_MAX][GRID_SIZE_MAX];
    int  size;
    int  stackScore;
    bool diagonalMode;
    int  diagonalModeFrozenTurns;
    bool diagonalModeForced;
    bool redundantColorActive;
    bool bankerChipActive;
    bool segfaultHandlerActive;
    bool cacheBoostActive;
    bool faceValueBoostActive;
    ComboType disabledComboType;
    int  lockOwnerRow[GRID_SIZE_MAX][GRID_SIZE_MAX];
    int  lockOwnerCol[GRID_SIZE_MAX][GRID_SIZE_MAX];

    int  trapRow, trapCol;
    int  bannedAxis;
    bool scoreThresholdActive;

    bool rottenSlot[GRID_SIZE_MAX][GRID_SIZE_MAX];
} MemoryGrid;

void memorygrid_construct(MemoryGrid *grid);

void memorygrid_init(MemoryGrid *grid, Deck *deck, int size);

int  memorygrid_calculateStackScore(const MemoryGrid *grid); 
bool memorygrid_isCellFree(const MemoryGrid *grid, int row, int col); 
void memorygrid_placeCard(MemoryGrid *grid, int row, int col, Card card); 

void memorygrid_blockCell(MemoryGrid *grid, int row, int col);

void memorygrid_unblockCell(MemoryGrid *grid, int row, int col);

void memorygrid_swapCells(MemoryGrid *grid, int row1, int col1, int row2, int col2);
bool memorygrid_toggleAxisMode(MemoryGrid *grid);

void memorygrid_tickTurn(MemoryGrid *grid);

int memorygrid_queenNeighbors(int row, int col, int size, bool includeDiagonals, int outNeighbors[8][2]);

void memorygrid_queenLock(MemoryGrid *grid, int queenRow, int queenCol,
                            int lockRow1, int lockCol1, int lockRow2, int lockCol2);

void memorygrid_setRedundantColorActive(MemoryGrid *grid, bool active);
void memorygrid_setBankerChipActive(MemoryGrid *grid, bool active);
void memorygrid_setSegfaultHandlerActive(MemoryGrid *grid, bool active);
void memorygrid_setCacheBoostActive(MemoryGrid *grid, bool active);
void memorygrid_setFaceValueBoostActive(MemoryGrid *grid, bool active);

void memorygrid_clearAllRot(MemoryGrid *grid);

void memorygrid_setDisabledCombo(MemoryGrid *grid, ComboType disabled);

void memorygrid_setTrapCell(MemoryGrid *grid, int row, int col);
void memorygrid_setBannedAxis(MemoryGrid *grid, int axis);
void memorygrid_setScoreThresholdActive(MemoryGrid *grid, bool active);

void memorygrid_setDiagonalModeForced(MemoryGrid *grid, bool forced);
int  memorygrid_countRottenCandidates(const MemoryGrid *grid);
bool memorygrid_addRottenSlotAtIndex(MemoryGrid *grid, int index);

void memorygrid_memoryFlush(MemoryGrid *grid, Deck *deck, int row, int col);

void memorygrid_resolveAceValues(MemoryGrid *grid, int stackLimit);

ComboResult memorygrid_resolveAlignments(MemoryGrid *grid, Deck *deck, int stackLimit);

#define FAIR_DEAL_MAX_ATTEMPTS 400

void memorygrid_ensureFairDeal(MemoryGrid *grid, Deck *deck, int stackLimit,
                                 bool avoidPreexistingMatches, int maxAttempts);

int memorygrid_comboBasePoints(ComboType type);

typedef struct LineClassification {
    ComboType type;
    bool active;
    bool nearCombo;
    int  cells[LINE_LEN_MAX][2];
    int  length;
} LineClassification;

int memorygrid_classifyAllLines(const MemoryGrid *grid, LineClassification outLines[LINE_COUNT_MAX]);

#endif
