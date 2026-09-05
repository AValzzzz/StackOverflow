#include "memory_grid.h"

#include <stdlib.h>

#define GLITCH_CLEAR_BONUS_MULTIPLIER 2

#define L1_CACHE_BONUS_MULTIPLIER 1.5f

#define KING_FLIP_FREEZE_TURNS 2

#define FACE_VALUE_SCALE 8

#define KING_DIAGONAL_SCORE_MULTIPLIER 2.5f

#define OVERCLOCK_BRELAN_CHIP_BONUS 50
#define JIT_COMPILER_BONUS_PER_GLITCH 2.0f
#define CLUB_BONUS_MULTIPLIER 2.0f
#define GLITCH_EXPLOIT_MULTIPLIER 3.0f
#define GARBAGE_COLLECTOR_BONUS_PER_COMBO 0.2f
#define MODULE_MAX_LEVEL_LOCAL 3

#define STACK_SOFT_CAP_RATIO  0.70f
#define STACK_SOFT_CAP_FACTOR 0.5f

#define DRAW_BIAS_DANGER_RATIO 0.35f
#define DRAW_BIAS_MAX          1.4f

static int buildLines(int size, int outLines[LINE_COUNT_MAX][LINE_LEN_MAX][2])
{
    int n = 0;
    for (int r = 0; r < size; r++, n++)
        for (int c = 0; c < size; c++)
            outLines[n][c][0] = r, outLines[n][c][1] = c;
    for (int c = 0; c < size; c++, n++)
        for (int r = 0; r < size; r++)
            outLines[n][r][0] = r, outLines[n][r][1] = c;
    for (int i = 0; i < size; i++) outLines[n][i][0] = i, outLines[n][i][1] = i;
    n++;
    for (int i = 0; i < size; i++) outLines[n][i][0] = i, outLines[n][i][1] = size - 1 - i;
    n++;
    return n;
}

static bool bankerChipCoversRank(int level, Rank rank)
{
    if (level <= 0) return false;
    if (rank == RANK_JACK) return true;
    if (level >= 2 && rank == RANK_QUEEN) return true;
    if (level >= 3 && rank == RANK_KING) return true;
    return false;
}

static int amortizationThreshold(int level)
{
    if (level >= 3) return 9;
    if (level == 2) return 7;
    return 5;
}

static int cellValue(const MemoryGrid *grid, int row, int col)
{
    const Card *card = &grid->cards[row][col];
    if (card->isLocked) return 0;
    if (bankerChipCoversRank(grid->bankerChipLevel, card->rank))
        return 0;

    int value;
    if (card->isRotted && grid->stackCanaryLevel > 0)
    {
        int mult = card_rotMultiplier(card);
        int reducedMult = 1 + (mult - 1) / (grid->stackCanaryLevel + 1);
        value = (int)card->rank * reducedMult;
    }
    else
    {
        value = card_getEffectiveValue(card);
    }

    if (card->isGlitched) value = (value + 1) / 2;
    if (card->isDiscounted) value = (value + 1) / 2;

    if (grid->coreDumpLevel > 0 && (card->isGlitched || card->isRotted))
    {
        value -= 2 * grid->coreDumpLevel;
        if (value < 0) value = 0;
    }

    if (grid->amortizationLevel > 0 && (int)card->rank <= amortizationThreshold(grid->amortizationLevel))
        value = (value + 1) / 2;

    if (grid->deallocatorLevel > 0 && (int)card->suit == grid->deallocatorSuit)
        value = (value * (4 - grid->deallocatorLevel) + 3) / 4;

    if (col == grid->leakColumn) value += grid->leakAmount;
    return value;
}

static int suitGroup(const MemoryGrid *grid, Suit suit)
{
    if (grid->redundantWarmLevel > 0 && (suit == SUIT_HEART || suit == SUIT_DIAMOND)) return 0;
    if (grid->redundantCoolLevel > 0 && (suit == SUIT_CLUB || suit == SUIT_SPADE)) return 1;
    return 10 + (int)suit;
}

static bool lineContainsCell(int line[LINE_LEN_MAX][2], int length, int row, int col)
{
    for (int i = 0; i < length; i++)
        if (line[i][0] == row && line[i][1] == col) return true;
    return false;
}

static bool lineIndex_isDiagonal(int lineIndex, int size)
{
    return lineIndex >= 2 * size;
}

static bool lineIsExactSameSuit(const MemoryGrid *grid, int line[LINE_LEN_MAX][2], int length)
{
    Suit first = grid->cards[line[0][0]][line[0][1]].suit;
    for (int i = 1; i < length; i++)
        if (grid->cards[line[i][0]][line[i][1]].suit != first) return false;
    return true;
}

static bool lineIsActive(int lineIndex, int size, bool diagonalMode, int bannedAxis)
{
    bool isDiagonal = (lineIndex >= 2 * size);
    bool isRow = (lineIndex < size);
    bool isCol = !isRow && !isDiagonal;
    if (bannedAxis == BANNED_AXIS_ROWS && isRow) return false;
    if (bannedAxis == BANNED_AXIS_COLS && isCol) return false;
    if (bannedAxis == BANNED_AXIS_DIAGONALS && isDiagonal) return false;
    return diagonalMode ? isDiagonal : !isDiagonal;
}

static void releaseQueenLocks(MemoryGrid *grid, int queenRow, int queenCol)
{
    for (int row = 0; row < grid->size; row++)
    {
        for (int col = 0; col < grid->size; col++)
        {
            if (grid->lockOwnerRow[row][col] != queenRow || grid->lockOwnerCol[row][col] != queenCol) continue;
            grid->lockOwnerRow[row][col] = -1;
            grid->lockOwnerCol[row][col] = -1;
            if (!grid->cards[row][col].isLocked) continue;
            int before = cellValue(grid, row, col);
            grid->cards[row][col].isLocked = false;
            grid->stackScore += cellValue(grid, row, col) - before;
        }
    }
}

int memorygrid_comboBasePoints(ComboType type)
{
    switch (type)
    {
        case COMBO_SAME_SUIT:      return 100;
        case COMBO_STRAIGHT:       return 250;
        case COMBO_BRELAN:         return 400;
        case COMBO_STRAIGHT_FLUSH: return 1000;
        default:                   return 0;
    }
}

static ComboType classifyLine(const MemoryGrid *grid, int line[LINE_LEN_MAX][2], int length)
{
    const Card *first = &grid->cards[line[0][0]][line[0][1]];
    int firstSuitGroup = suitGroup(grid, first->suit);
    Suit firstSuitExact = first->suit;
    Rank firstRank = first->rank;
    bool sameSuit = true, sameSuitExact = true, sameRank = true;
    int values[LINE_LEN_MAX];

    for (int i = 0; i < length; i++)
    {
        const Card *c = &grid->cards[line[i][0]][line[i][1]];
        if (suitGroup(grid, c->suit) != firstSuitGroup) sameSuit = false;
        if (c->suit != firstSuitExact) sameSuitExact = false;
        if (c->rank != firstRank) sameRank = false;
        values[i] = card_getEffectiveValue(c);
    }

    for (int i = 1; i < length; i++)
    {
        int v = values[i], j = i - 1;
        while (j >= 0 && values[j] > v) { values[j + 1] = values[j]; j--; }
        values[j + 1] = v;
    }
    bool isStraight = true;
    for (int i = 1; i < length; i++)
        if (values[i] != values[i - 1] + 1) { isStraight = false; break; }

    ComboType type = COMBO_NONE;
    if (sameRank) type = COMBO_BRELAN;
    else if (isStraight && sameSuitExact) type = COMBO_STRAIGHT_FLUSH;
    else if (isStraight) type = COMBO_STRAIGHT;
    else if (sameSuit) type = COMBO_SAME_SUIT;

    if (type != COMBO_NONE && type == grid->disabledComboType) type = COMBO_NONE;
    return type;
}

static bool lineIsNearCombo(const MemoryGrid *grid, int line[LINE_LEN_MAX][2], int length)
{
    int maxSuitFreq = 0, maxRankFreq = 0;
    for (int i = 0; i < length; i++)
    {
        const Card *a = &grid->cards[line[i][0]][line[i][1]];
        int suitFreq = 0, rankFreq = 0;
        for (int j = 0; j < length; j++)
        {
            const Card *b = &grid->cards[line[j][0]][line[j][1]];
            if (suitGroup(grid, a->suit) == suitGroup(grid, b->suit)) suitFreq++;
            if (a->rank == b->rank) rankFreq++;
        }
        if (suitFreq > maxSuitFreq) maxSuitFreq = suitFreq;
        if (rankFreq > maxRankFreq) maxRankFreq = rankFreq;
    }
    bool suitPair = maxSuitFreq >= length - 1;
    bool rankPair = maxRankFreq >= length - 1;

    int values[LINE_LEN_MAX];
    for (int i = 0; i < length; i++)
        values[i] = card_getEffectiveValue(&grid->cards[line[i][0]][line[i][1]]);
    for (int i = 1; i < length; i++)
    {
        int v = values[i], j = i - 1;
        while (j >= 0 && values[j] > v) { values[j + 1] = values[j]; j--; }
        values[j + 1] = v;
    }
    bool straightPair = (values[length - 1] - values[0]) <= length;

    return suitPair || rankPair || straightPair;
}

int memorygrid_classifyAllLines(const MemoryGrid *grid, LineClassification outLines[LINE_COUNT_MAX])
{
    int lines[LINE_COUNT_MAX][LINE_LEN_MAX][2];
    int lineCount = buildLines(grid->size, lines);

    for (int l = 0; l < lineCount; l++)
    {
        outLines[l].active = lineIsActive(l, grid->size, grid->diagonalMode, grid->bannedAxis);
        outLines[l].length = grid->size;
        for (int i = 0; i < grid->size; i++)
        {
            outLines[l].cells[i][0] = lines[l][i][0];
            outLines[l].cells[i][1] = lines[l][i][1];
        }
        outLines[l].type = outLines[l].active ? classifyLine(grid, lines[l], grid->size) : COMBO_NONE;
        outLines[l].nearCombo = outLines[l].active && outLines[l].type == COMBO_NONE &&
                                  lineIsNearCombo(grid, lines[l], grid->size);
    }
    return lineCount;
}

int memorygrid_softenedStackScore(int rawScore, int stackLimit)
{
    if (stackLimit <= 0) return rawScore;
    float threshold = STACK_SOFT_CAP_RATIO * (float)stackLimit;
    if ((float)rawScore <= threshold) return rawScore;
    float excess = (float)rawScore - threshold;
    return (int)(threshold + excess * STACK_SOFT_CAP_FACTOR);
}

float memorygrid_drawBiasForHeadroom(int stackScore, int stackLimit)
{
    if (stackLimit <= 0) return 0.0f;
    int effective = memorygrid_softenedStackScore(stackScore, stackLimit);
    float headroom = (float)(stackLimit - effective) / (float)stackLimit;
    if (headroom >= DRAW_BIAS_DANGER_RATIO) return 0.0f;
    if (headroom < 0.0f) headroom = 0.0f;
    float danger = 1.0f - headroom / DRAW_BIAS_DANGER_RATIO;
    return DRAW_BIAS_MAX * danger * danger;
}

static int refillCell(MemoryGrid *grid, Deck *deck, int row, int col, int stackLimit, bool discountRefill)
{
    if (grid->cards[row][col].isLocked) return 0;
    releaseQueenLocks(grid, row, col);
    int bonus = 0;
    if (grid->cards[row][col].isGlitched)
        bonus = card_getChipValue(&grid->cards[row][col]) * GLITCH_CLEAR_BONUS_MULTIPLIER;
    if (deck_isEmpty(deck)) return bonus;
    int oldValue = cellValue(grid, row, col);
    deck_discard(deck, grid->cards[row][col]);
    float bias = memorygrid_drawBiasForHeadroom(grid->stackScore, stackLimit);
    grid->cards[row][col] = deck_drawCardWeighted(deck, bias);
    grid->cards[row][col].isDiscounted = discountRefill;
    grid->stackScore += cellValue(grid, row, col) - oldValue;
    return bonus;
}

void memorygrid_construct(MemoryGrid *grid)
{
    for (int row = 0; row < GRID_SIZE_MAX; row++)
        for (int col = 0; col < GRID_SIZE_MAX; col++)
            grid->cards[row][col] = card_make(SUIT_HEART, RANK_TWO);
    grid->size = GRID_SIZE_MIN;
    grid->stackScore = 0;
    grid->diagonalMode = false;
    grid->diagonalModeFrozenTurns = 0;
    grid->diagonalModeForced = false;
    grid->redundantWarmLevel = 0;
    grid->redundantCoolLevel = 0;
    grid->bankerChipLevel = 0;
    grid->cacheBoostLevel = 0;
    grid->faceValueBoostLevel = 0;
    grid->overclockLevel = 0;
    grid->jitCompilerLevel = 0;
    grid->clubBonusLevel = 0;
    grid->glitchExploitLevel = 0;
    grid->garbageCollectorLevel = 0;
    grid->garbageCollectorMultiplier = 0.0f;
    grid->coreDumpLevel = 0;
    grid->stackCanaryLevel = 0;
    grid->amortizationLevel = 0;
    grid->diagonalCacheLevel = 0;
    grid->deallocatorLevel = 0;
    grid->deallocatorSuit = -1;
    grid->compressionLevel = 0;
    grid->leakColumn = -1;
    grid->leakAmount = 0;
    grid->disabledComboType = COMBO_NONE;
    for (int row = 0; row < GRID_SIZE_MAX; row++)
        for (int col = 0; col < GRID_SIZE_MAX; col++)
        {
            grid->lockOwnerRow[row][col] = -1;
            grid->lockOwnerCol[row][col] = -1;
        }
}

void memorygrid_init(MemoryGrid *grid, Deck *deck, int size)
{
    grid->size = size;
    for (int row = 0; row < size; row++)
        for (int col = 0; col < size; col++)
        {
            grid->cards[row][col] = deck_drawCard(deck);
            grid->lockOwnerRow[row][col] = -1;
            grid->lockOwnerCol[row][col] = -1;
        }
    grid->diagonalMode = false;
    grid->diagonalModeFrozenTurns = 0;
    grid->diagonalModeForced = false;
    grid->trapRow = -1;
    grid->trapCol = -1;
    grid->bannedAxis = BANNED_AXIS_NONE;
    grid->scoreThresholdActive = false;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

int memorygrid_calculateStackScore(const MemoryGrid *grid)
{
    int score = 0;
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            score += cellValue(grid, row, col);
    return score;
}

bool memorygrid_isCellFree(const MemoryGrid *grid, int row, int col)
{
    return !grid->cards[row][col].isLocked;
}

void memorygrid_placeCard(MemoryGrid *grid, int row, int col, Card card)
{
    releaseQueenLocks(grid, row, col);
    int oldValue = cellValue(grid, row, col);
    grid->cards[row][col] = card;
    grid->stackScore += cellValue(grid, row, col) - oldValue;
}

void memorygrid_blockCell(MemoryGrid *grid, int row, int col)
{
    int before = cellValue(grid, row, col);
    grid->cards[row][col].isLocked = true;
    grid->lockOwnerRow[row][col] = -1;
    grid->lockOwnerCol[row][col] = -1;
    grid->stackScore += cellValue(grid, row, col) - before;
}

void memorygrid_unblockCell(MemoryGrid *grid, int row, int col)
{
    int before = cellValue(grid, row, col);
    grid->cards[row][col].isLocked = false;
    grid->stackScore += cellValue(grid, row, col) - before;
}

void memorygrid_swapCells(MemoryGrid *grid, int row1, int col1, int row2, int col2)
{
    releaseQueenLocks(grid, row1, col1);
    releaseQueenLocks(grid, row2, col2);

    int before1 = cellValue(grid, row1, col1);
    int before2 = cellValue(grid, row2, col2);

    Card tmp = grid->cards[row1][col1];
    grid->cards[row1][col1] = grid->cards[row2][col2];
    grid->cards[row2][col2] = tmp;

    int ownerRowTmp = grid->lockOwnerRow[row1][col1];
    int ownerColTmp = grid->lockOwnerCol[row1][col1];
    grid->lockOwnerRow[row1][col1] = grid->lockOwnerRow[row2][col2];
    grid->lockOwnerCol[row1][col1] = grid->lockOwnerCol[row2][col2];
    grid->lockOwnerRow[row2][col2] = ownerRowTmp;
    grid->lockOwnerCol[row2][col2] = ownerColTmp;

    grid->stackScore += (cellValue(grid, row1, col1) - before1) + (cellValue(grid, row2, col2) - before2);
}

bool memorygrid_toggleAxisMode(MemoryGrid *grid)
{
    if (grid->diagonalModeForced) return false;
    if (grid->diagonalModeFrozenTurns > 0) return false;
    grid->diagonalMode = !grid->diagonalMode;
    grid->diagonalModeFrozenTurns = KING_FLIP_FREEZE_TURNS;
    return true;
}

void memorygrid_setDiagonalModeForced(MemoryGrid *grid, bool forced)
{
    grid->diagonalModeForced = forced;
    if (forced) grid->diagonalMode = true;
}

void memorygrid_tickTurn(MemoryGrid *grid)
{
    if (grid->diagonalModeFrozenTurns > 0) grid->diagonalModeFrozenTurns--;

}

int memorygrid_queenNeighbors(int row, int col, int size, bool includeDiagonals, int outNeighbors[8][2])
{
    static const int ORTHO_OFFSETS[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
    static const int DIAG_OFFSETS[4][2]  = { {-1,-1}, {-1,1}, {1,-1}, {1,1} };
    int count = 0;
    for (int n = 0; n < 4; n++)
    {
        int nr = row + ORTHO_OFFSETS[n][0];
        int nc = col + ORTHO_OFFSETS[n][1];
        if (nr < 0 || nr >= size || nc < 0 || nc >= size) continue;
        outNeighbors[count][0] = nr;
        outNeighbors[count][1] = nc;
        count++;
    }
    if (!includeDiagonals) return count;
    for (int n = 0; n < 4; n++)
    {
        int nr = row + DIAG_OFFSETS[n][0];
        int nc = col + DIAG_OFFSETS[n][1];
        if (nr < 0 || nr >= size || nc < 0 || nc >= size) continue;
        outNeighbors[count][0] = nr;
        outNeighbors[count][1] = nc;
        count++;
    }
    return count;
}

void memorygrid_queenLock(MemoryGrid *grid, int queenRow, int queenCol,
                            int lockRow1, int lockCol1, int lockRow2, int lockCol2)
{
    int targets[2][2] = { { lockRow1, lockCol1 }, { lockRow2, lockCol2 } };
    for (int i = 0; i < 2; i++)
    {
        int row = targets[i][0], col = targets[i][1];
        if (row < 0 || col < 0) continue;
        if (grid->cards[row][col].isLocked) continue;
        int before = cellValue(grid, row, col);
        grid->cards[row][col].isLocked = true;
        grid->lockOwnerRow[row][col] = queenRow;
        grid->lockOwnerCol[row][col] = queenCol;
        grid->stackScore += cellValue(grid, row, col) - before;
    }
}

void memorygrid_setRedundantColorLevels(MemoryGrid *grid, int warmLevel, int coolLevel)
{
    grid->redundantWarmLevel = warmLevel;
    grid->redundantCoolLevel = coolLevel;
}

void memorygrid_setBankerChipLevel(MemoryGrid *grid, int level)
{
    grid->bankerChipLevel = level;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setCacheBoostLevel(MemoryGrid *grid, int level)
{
    grid->cacheBoostLevel = level;
}

void memorygrid_setFaceValueBoostLevel(MemoryGrid *grid, int level)
{
    grid->faceValueBoostLevel = level;
}

void memorygrid_setOverclockLevel(MemoryGrid *grid, int level)
{
    grid->overclockLevel = level;
}

void memorygrid_setJitCompilerLevel(MemoryGrid *grid, int level)
{
    grid->jitCompilerLevel = level;
}

void memorygrid_setClubBonusLevel(MemoryGrid *grid, int level)
{
    grid->clubBonusLevel = level;
}

void memorygrid_setGlitchExploitLevel(MemoryGrid *grid, int level)
{
    grid->glitchExploitLevel = level;
}

void memorygrid_setGarbageCollectorLevel(MemoryGrid *grid, int level)
{
    grid->garbageCollectorLevel = level;
}

void memorygrid_setCoreDumpLevel(MemoryGrid *grid, int level)
{
    grid->coreDumpLevel = level;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setStackCanaryLevel(MemoryGrid *grid, int level)
{
    grid->stackCanaryLevel = level;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setAmortizationLevel(MemoryGrid *grid, int level)
{
    grid->amortizationLevel = level;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setDiagonalCacheLevel(MemoryGrid *grid, int level)
{
    grid->diagonalCacheLevel = level;
}

void memorygrid_setCompressionLevel(MemoryGrid *grid, int level)
{
    grid->compressionLevel = level;
}

void memorygrid_setDeallocatorLevel(MemoryGrid *grid, int level)
{
    grid->deallocatorLevel = level;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setDeallocatorSuit(MemoryGrid *grid, int suit)
{
    grid->deallocatorSuit = suit;
}

void memorygrid_setColumnLeak(MemoryGrid *grid, int col, int amount)
{
    grid->leakColumn = col;
    grid->leakAmount = amount;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_clearColumnLeak(MemoryGrid *grid)
{
    grid->leakColumn = -1;
    grid->leakAmount = 0;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

int memorygrid_freeCellCount(const MemoryGrid *grid)
{
    int count = 0;
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            if (!grid->cards[row][col].isLocked) count++;
    return count;
}

void memorygrid_clearAllRot(MemoryGrid *grid)
{
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            card_clearRot(&grid->cards[row][col]);
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setDisabledCombo(MemoryGrid *grid, ComboType disabled)
{
    grid->disabledComboType = disabled;
}

void memorygrid_setTrapCell(MemoryGrid *grid, int row, int col)
{
    grid->trapRow = row;
    grid->trapCol = col;
}

void memorygrid_setBannedAxis(MemoryGrid *grid, int axis)
{
    grid->bannedAxis = axis;
}

void memorygrid_setScoreThresholdActive(MemoryGrid *grid, bool active)
{
    grid->scoreThresholdActive = active;
}

void memorygrid_memoryFlush(MemoryGrid *grid, Deck *deck, int row, int col)
{
    releaseQueenLocks(grid, row, col);
    grid->cards[row][col].isLocked = false;
    grid->lockOwnerRow[row][col] = -1;
    grid->lockOwnerCol[row][col] = -1;
    if (!deck_isEmpty(deck))
    {
        int oldValue = cellValue(grid, row, col);
        deck_discard(deck, grid->cards[row][col]);
        grid->cards[row][col] = deck_drawCard(deck);
        grid->stackScore += cellValue(grid, row, col) - oldValue;
    }
}

void memorygrid_resolveAceValues(MemoryGrid *grid, int stackLimit)
{
    (void)stackLimit;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

static bool gridHasActiveMatch(const MemoryGrid *grid)
{
    LineClassification lines[LINE_COUNT_MAX];
    int n = memorygrid_classifyAllLines(grid, lines);
    for (int i = 0; i < n; i++)
        if (lines[i].active && lines[i].type != COMBO_NONE) return true;
    return false;
}

static bool tryBestMatchBreakingSwap(MemoryGrid *grid)
{
    LineClassification before[LINE_COUNT_MAX];
    int beforeCount = memorygrid_classifyAllLines(grid, before);
    int currentMatches = 0;
    for (int i = 0; i < beforeCount; i++)
        if (before[i].active && before[i].type != COMBO_NONE) currentMatches++;
    if (currentMatches == 0) return false;

    int bestMatches = currentMatches;
    int bestR1 = -1, bestC1 = -1, bestR2 = -1, bestC2 = -1;

    for (int r1 = 0; r1 < grid->size; r1++)
        for (int c1 = 0; c1 < grid->size; c1++)
        {
            if (grid->cards[r1][c1].isLocked) continue;
            for (int r2 = 0; r2 < grid->size; r2++)
                for (int c2 = 0; c2 < grid->size; c2++)
                {
                    if (r1 == r2 && c1 == c2) continue;
                    if (grid->cards[r2][c2].isLocked) continue;

                    MemoryGrid probe = *grid;
                    memorygrid_swapCells(&probe, r1, c1, r2, c2);
                    LineClassification lines[LINE_COUNT_MAX];
                    int n = memorygrid_classifyAllLines(&probe, lines);
                    int matches = 0;
                    for (int i = 0; i < n; i++)
                        if (lines[i].active && lines[i].type != COMBO_NONE) matches++;

                    if (matches < bestMatches)
                    {
                        bestMatches = matches;
                        bestR1 = r1; bestC1 = c1; bestR2 = r2; bestC2 = c2;
                        if (matches == 0) goto found;
                    }
                }
        }
found:
    if (bestR1 == -1) return false;
    memorygrid_swapCells(grid, bestR1, bestC1, bestR2, bestC2);
    return true;
}

void memorygrid_ensureFairDeal(MemoryGrid *grid, Deck *deck, int stackLimit,
                                 bool avoidPreexistingMatches, int maxAttempts)
{
    for (int attempt = 0; attempt < maxAttempts && grid->stackScore > stackLimit; attempt++)
    {
        if (deck_isEmpty(deck)) break;

        int row = -1, col = -1, bestValue = -1;
        for (int r = 0; r < grid->size; r++)
            for (int c = 0; c < grid->size; c++)
            {
                if (grid->cards[r][c].isLocked) continue;
                int value = card_getEffectiveValue(&grid->cards[r][c]);
                if (value > bestValue) { bestValue = value; row = r; col = c; }
            }
        if (row == -1) break;

        if (!deck_hasCardBelow(deck, bestValue)) break;

        deck_injectCard(deck, grid->cards[row][col]);
        memorygrid_placeCard(grid, row, col, deck_drawCardBelow(deck, bestValue));
    }

    if (avoidPreexistingMatches)
        for (int i = 0; i < grid->size * grid->size && gridHasActiveMatch(grid); i++)
            if (!tryBestMatchBreakingSwap(grid)) break;
}

ComboResult memorygrid_resolveAlignments(MemoryGrid *grid, Deck *deck, int stackLimit)
{
    ComboResult result = { 0 };
    int lines[LINE_COUNT_MAX][LINE_LEN_MAX][2];
    int lineCount = buildLines(grid->size, lines);
    ComboType lineTypes[LINE_COUNT_MAX];
    bool anyMatch = false;
    int matchedLineCount = 0;
    int straightLineCount = 0;

    int gridGlitchedCount = 0;
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            if (grid->cards[row][col].isRotted || grid->cards[row][col].isGlitched) gridGlitchedCount++;

    for (int l = 0; l < lineCount; l++)
    {
        lineTypes[l] = lineIsActive(l, grid->size, grid->diagonalMode, grid->bannedAxis) ? classifyLine(grid, lines[l], grid->size) : COMBO_NONE;

        bool matched = (lineTypes[l] != COMBO_NONE);
        if (matched)
        {
            anyMatch = true;

            int baseChips = memorygrid_comboBasePoints(lineTypes[l]);

            int faceValueSum = 0;
            int glitchedCount = 0;
            bool containsClub = false;
            for (int i = 0; i < grid->size; i++)
            {
                const Card *c = &grid->cards[lines[l][i][0]][lines[l][i][1]];
                faceValueSum += card_getChipValue(c);
                if (c->isRotted || c->isGlitched) glitchedCount++;
                if (c->suit == SUIT_CLUB) containsClub = true;
            }
            int faceValueBonus = faceValueSum - 2 * grid->size;
            float faceValueScale = grid->faceValueBoostLevel > 0
                ? FACE_VALUE_SCALE * (1.25f + 0.25f * (float)grid->faceValueBoostLevel) : FACE_VALUE_SCALE;
            int cardBonus = (faceValueBonus > 0) ? (int)(faceValueBonus * faceValueScale) : 0;

            if (grid->overclockLevel > 0 && lineTypes[l] == COMBO_BRELAN)
                baseChips += OVERCLOCK_BRELAN_CHIP_BONUS * grid->overclockLevel;

            float multiplier = 1.0f;

            if (grid->clubBonusLevel > 0 && containsClub)
            {
                multiplier *= CLUB_BONUS_MULTIPLIER + 0.5f * (float)(grid->clubBonusLevel - 1);
                result.triggeredClubBonus = true;
            }

            if (grid->glitchExploitLevel > 0 && glitchedCount > 0)
            {
                multiplier *= GLITCH_EXPLOIT_MULTIPLIER + 1.0f * (float)(grid->glitchExploitLevel - 1);
                result.triggeredGlitchExploit = true;
            }

            if (lineTypes[l] == COMBO_SAME_SUIT && (grid->redundantWarmLevel > 0 || grid->redundantCoolLevel > 0) &&
                !lineIsExactSameSuit(grid, lines[l], grid->size))
            {
                static const float REDUNDANT_PENALTY_BY_LEVEL[MODULE_MAX_LEVEL_LOCAL + 1] = { 1.0f, 0.5f, 0.65f, 0.8f };
                int group = suitGroup(grid, grid->cards[lines[l][0][0]][lines[l][0][1]].suit);
                int mergeLevel = (group == 0) ? grid->redundantWarmLevel : (group == 1 ? grid->redundantCoolLevel : 0);
                if (mergeLevel < 1) mergeLevel = 1;
                if (mergeLevel > MODULE_MAX_LEVEL_LOCAL) mergeLevel = MODULE_MAX_LEVEL_LOCAL;
                multiplier *= REDUNDANT_PENALTY_BY_LEVEL[mergeLevel];
            }

            if (grid->diagonalMode && !grid->diagonalModeForced && lineIndex_isDiagonal(l, grid->size))
            {
                multiplier *= KING_DIAGONAL_SCORE_MULTIPLIER;
                if (grid->diagonalCacheLevel > 0)
                {
                    multiplier *= 1.0f + 0.25f * (float)grid->diagonalCacheLevel;
                    result.triggeredDiagonalCache = true;
                }
            }

            if (grid->size == 3 && lineContainsCell(lines[l], grid->size, L1_CACHE_ROW, L1_CACHE_COL))
            {
                if (grid->cacheBoostLevel > 0)
                {
                    multiplier *= 1.75f + 0.25f * (float)grid->cacheBoostLevel;
                    result.triggeredCacheBoost = true;
                }
                else
                {
                    multiplier *= L1_CACHE_BONUS_MULTIPLIER;
                }
            }

            if (grid->scoreThresholdActive)
            {
                int strongCount = 0;
                for (int i = 0; i < grid->size; i++)
                    if (card_getEffectiveValue(&grid->cards[lines[l][i][0]][lines[l][i][1]]) > 3) strongCount++;
                multiplier *= (float)strongCount / (float)grid->size;
            }

            if (grid->jitCompilerLevel > 0 && gridGlitchedCount > 0)
            {
                multiplier += JIT_COMPILER_BONUS_PER_GLITCH * (float)grid->jitCompilerLevel * (float)gridGlitchedCount;
                result.triggeredJitCompiler = true;
            }

            if (grid->garbageCollectorMultiplier > 0.0f)
                multiplier *= 1.0f + grid->garbageCollectorMultiplier;

            if (grid->trapRow >= 0 && lineContainsCell(lines[l], grid->size, grid->trapRow, grid->trapCol))
                multiplier = 0.0f;

            result.totalScoreGained += (int)((baseChips + cardBonus) * multiplier);
            if (multiplier > result.bestMultiplier) result.bestMultiplier = multiplier;
            for (int i = 0; i < grid->size; i++)
                result.cellInvolved[lines[l][i][0]][lines[l][i][1]] = true;
            matchedLineCount++;
            if (lineTypes[l] == COMBO_STRAIGHT || lineTypes[l] == COMBO_STRAIGHT_FLUSH) straightLineCount++;
        }

        switch (lineTypes[l])
        {
            case COMBO_SAME_SUIT:      result.sameSuitMatches++;      break;
            case COMBO_STRAIGHT:       result.straightMatches++;      break;
            case COMBO_BRELAN:         result.brelanMatches++;        break;
            case COMBO_STRAIGHT_FLUSH: result.straightFlushMatches++; break;
            default: break;
        }
    }

    if (!anyMatch) return result;

    if (grid->garbageCollectorLevel > 0 && straightLineCount > 0)
    {
        grid->garbageCollectorMultiplier +=
            GARBAGE_COLLECTOR_BONUS_PER_COMBO * (float)grid->garbageCollectorLevel * (float)straightLineCount;
        result.triggeredGarbageCollector = true;
    }

    if (result.straightFlushMatches > 0)
    {
        for (int row = 0; row < grid->size; row++)
        {
            for (int col = 0; col < grid->size; col++)
            {
                if (grid->cards[row][col].isLocked) continue;
                releaseQueenLocks(grid, row, col);
                if (grid->cards[row][col].isGlitched)
                    result.totalScoreGained +=
                        card_getChipValue(&grid->cards[row][col]) * GLITCH_CLEAR_BONUS_MULTIPLIER;
                if (deck_isEmpty(deck)) continue;
                deck_discard(deck, grid->cards[row][col]);
                grid->cards[row][col] = deck_drawCard(deck);
            }
        }
        grid->stackScore = memorygrid_calculateStackScore(grid);
        memorygrid_ensureFairDeal(grid, deck, stackLimit, true, FAIR_DEAL_MAX_ATTEMPTS);
        result.gridWasWiped = true;
        return result;
    }

    bool anyHalving = false;
    for (int l = 0; l < lineCount; l++)
        if (lineTypes[l] == COMBO_STRAIGHT || lineTypes[l] == COMBO_BRELAN)
            anyHalving = true;

    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            if (result.cellInvolved[row][col])
                result.totalScoreGained += refillCell(grid, deck, row, col, stackLimit, anyHalving);

    if (grid->compressionLevel > 0)
    {
        float cutFraction = 0.05f * (float)grid->compressionLevel * (float)matchedLineCount;
        if (cutFraction > 0.9f) cutFraction = 0.9f;
        grid->stackScore -= (int)((float)grid->stackScore * cutFraction);
        if (grid->stackScore < 0) grid->stackScore = 0;
    }

    if (grid->stackScore > stackLimit)
    {
        memorygrid_ensureFairDeal(grid, deck, stackLimit, false, FAIR_DEAL_MAX_ATTEMPTS);
        result.fairDealApplied = (grid->stackScore <= stackLimit);
    }

    return result;
}
