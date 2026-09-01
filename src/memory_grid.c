#include "memory_grid.h"

#include <stdlib.h>

#define SPECIAL_CLEAR_BONUS_MULTIPLIER 20

#define L1_CACHE_BONUS_MULTIPLIER 1.5f

#define KING_FLIP_FREEZE_TURNS 2

#define FACE_VALUE_SCALE 8

#define KING_DIAGONAL_SCORE_MULTIPLIER 2.0f

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

static int cellValue(const MemoryGrid *grid, int row, int col)
{
    const Card *card = &grid->cards[row][col];
    if (card->isLocked) return 0;
    if (grid->bankerChipActive &&
        (card->rank == RANK_JACK || card->rank == RANK_QUEEN || card->rank == RANK_KING))
        return 0;
    int value = card_getEffectiveValue(card);
    if (card->isSpecial) value /= 2;
    if (card->isRotted) value *= 2;
    return value;
}

static int suitGroup(const MemoryGrid *grid, Suit suit)
{
    if (!grid->redundantColorActive) return (int)suit;
    return (suit == SUIT_HEART || suit == SUIT_DIAMOND) ? 0 : 1;
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

static void applySlotRot(MemoryGrid *grid, int row, int col)
{
    if (grid->rottenSlot[row][col]) grid->cards[row][col].isRotted = true;
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

static int refillCell(MemoryGrid *grid, Deck *deck, int row, int col)
{
    if (grid->cards[row][col].isLocked) return 0;
    releaseQueenLocks(grid, row, col); 
    int bonus = 0;
    if (grid->cards[row][col].isSpecial)
        bonus = card_getEffectiveValue(&grid->cards[row][col]) * SPECIAL_CLEAR_BONUS_MULTIPLIER;
    if (deck_isEmpty(deck)) return bonus;
    int oldValue = cellValue(grid, row, col);
    deck_discard(deck, grid->cards[row][col]);
    grid->cards[row][col] = deck_drawCard(deck);
    applySlotRot(grid, row, col);
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
    grid->redundantColorActive = false;
    grid->bankerChipActive = false;
    grid->segfaultHandlerActive = false;
    grid->cacheBoostActive = false;
    grid->faceValueBoostActive = false;
    grid->disabledComboType = COMBO_NONE;
    for (int row = 0; row < GRID_SIZE_MAX; row++)
        for (int col = 0; col < GRID_SIZE_MAX; col++)
        {
            grid->lockOwnerRow[row][col] = -1;
            grid->lockOwnerCol[row][col] = -1;
            grid->rottenSlot[row][col] = false;
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
            grid->rottenSlot[row][col] = false;
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
    int oldValue = cellValue(grid, row, col);
    grid->cards[row][col] = card;
    applySlotRot(grid, row, col);
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
    applySlotRot(grid, row1, col1);
    applySlotRot(grid, row2, col2);

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

int memorygrid_countRottenCandidates(const MemoryGrid *grid)
{
    int count = 0;
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            if (!grid->rottenSlot[row][col] && !grid->cards[row][col].isLocked)
                count++;
    return count;
}

bool memorygrid_addRottenSlotAtIndex(MemoryGrid *grid, int index)
{
    int i = 0;
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
        {
            if (grid->rottenSlot[row][col] || grid->cards[row][col].isLocked) continue;
            if (i == index)
            {
                grid->rottenSlot[row][col] = true;
                int before = cellValue(grid, row, col);
                applySlotRot(grid, row, col);
                grid->stackScore += cellValue(grid, row, col) - before;
                return true;
            }
            i++;
        }
    return false;
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

void memorygrid_setRedundantColorActive(MemoryGrid *grid, bool active)
{
    grid->redundantColorActive = active;
}

void memorygrid_setBankerChipActive(MemoryGrid *grid, bool active)
{
    grid->bankerChipActive = active;
    grid->stackScore = memorygrid_calculateStackScore(grid);
}

void memorygrid_setSegfaultHandlerActive(MemoryGrid *grid, bool active)
{
    grid->segfaultHandlerActive = active;
}

void memorygrid_setCacheBoostActive(MemoryGrid *grid, bool active)
{
    grid->cacheBoostActive = active;
}

void memorygrid_setFaceValueBoostActive(MemoryGrid *grid, bool active)
{
    grid->faceValueBoostActive = active;
}

void memorygrid_clearAllRot(MemoryGrid *grid)
{
    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
        {
            grid->cards[row][col].isRotted = false;
            grid->rottenSlot[row][col] = false;
        }
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
        applySlotRot(grid, row, col);
        grid->stackScore += cellValue(grid, row, col) - oldValue;
    }
}

void memorygrid_resolveAceValues(MemoryGrid *grid, int stackLimit)
{
    int lines[LINE_COUNT_MAX][LINE_LEN_MAX][2];
    int lineCount = buildLines(grid->size, lines);

    for (int row = 0; row < grid->size; row++)
    {
        for (int col = 0; col < grid->size; col++)
        {
            Card *card = &grid->cards[row][col];
            if (card->rank != RANK_ACE) continue;

            if (grid->segfaultHandlerActive)
            {
                card->aceAsEleven = true;
                int scoreHigh = memorygrid_calculateStackScore(grid);
                card->aceAsEleven = false;
                int scoreLow = memorygrid_calculateStackScore(grid);
                if (scoreHigh <= stackLimit)      card->aceAsEleven = true;
                else if (scoreLow <= stackLimit)  card->aceAsEleven = false;
                else                              card->aceAsEleven = true; 
                continue;
            }

            bool alignsHigh = false, alignsLow = false;

            card->aceAsEleven = true;
            for (int l = 0; l < lineCount && !alignsHigh; l++)
                if (lineIsActive(l, grid->size, grid->diagonalMode, grid->bannedAxis) && lineContainsCell(lines[l], grid->size, row, col) &&
                    classifyLine(grid, lines[l], grid->size) != COMBO_NONE)
                    alignsHigh = true;

            card->aceAsEleven = false;
            for (int l = 0; l < lineCount && !alignsLow; l++)
                if (lineIsActive(l, grid->size, grid->diagonalMode, grid->bannedAxis) && lineContainsCell(lines[l], grid->size, row, col) &&
                    classifyLine(grid, lines[l], grid->size) != COMBO_NONE)
                    alignsLow = true;

            if (alignsHigh && !alignsLow) { card->aceAsEleven = true; continue; }
            if (alignsLow && !alignsHigh) { card->aceAsEleven = false; continue; }

            card->aceAsEleven = true;
            int scoreHigh = memorygrid_calculateStackScore(grid);
            card->aceAsEleven = false;
            int scoreLow = memorygrid_calculateStackScore(grid);

            if (scoreHigh <= stackLimit)      card->aceAsEleven = true;
            else if (scoreLow <= stackLimit)  card->aceAsEleven = false;
            else                              card->aceAsEleven = true;
        }
    }
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

    for (int l = 0; l < lineCount; l++)
    {
        lineTypes[l] = lineIsActive(l, grid->size, grid->diagonalMode, grid->bannedAxis) ? classifyLine(grid, lines[l], grid->size) : COMBO_NONE;

        bool matched = (lineTypes[l] != COMBO_NONE);
        if (matched)
        {
            anyMatch = true;
            int base = memorygrid_comboBasePoints(lineTypes[l]);

            int faceValueSum = 0;
            for (int i = 0; i < grid->size; i++)
                faceValueSum += card_getEffectiveValue(&grid->cards[lines[l][i][0]][lines[l][i][1]]);
            int faceValueBonus = faceValueSum - 2 * grid->size;
            float faceValueScale = grid->faceValueBoostActive ? FACE_VALUE_SCALE * 1.5f : FACE_VALUE_SCALE;
            if (faceValueBonus > 0) base += (int)(faceValueBonus * faceValueScale);

            if (lineTypes[l] == COMBO_SAME_SUIT && grid->redundantColorActive &&
                !lineIsExactSameSuit(grid, lines[l], grid->size))
                base /= 2;

            if (grid->diagonalMode && lineIndex_isDiagonal(l, grid->size))
                base = (int)(base * KING_DIAGONAL_SCORE_MULTIPLIER);

            if (grid->size == 3 && lineContainsCell(lines[l], grid->size, L1_CACHE_ROW, L1_CACHE_COL))
            {
                float l1Mult = grid->cacheBoostActive ? 2.0f : L1_CACHE_BONUS_MULTIPLIER;
                base = (int)(base * l1Mult);
            }

            if (grid->trapRow >= 0 && lineContainsCell(lines[l], grid->size, grid->trapRow, grid->trapCol))
                base = 0;

            if (grid->scoreThresholdActive)
            {
                int strongCount = 0;
                for (int i = 0; i < grid->size; i++)
                    if (card_getEffectiveValue(&grid->cards[lines[l][i][0]][lines[l][i][1]]) > 3) strongCount++;
                base = base * strongCount / grid->size;
            }

            result.totalScoreGained += base;
            for (int i = 0; i < grid->size; i++)
                result.cellInvolved[lines[l][i][0]][lines[l][i][1]] = true;
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

    if (result.straightFlushMatches > 0)
    {
        for (int row = 0; row < grid->size; row++)
        {
            for (int col = 0; col < grid->size; col++)
            {
                if (grid->cards[row][col].isLocked) continue;
                releaseQueenLocks(grid, row, col); 
                if (grid->cards[row][col].isSpecial)
                    result.totalScoreGained +=
                        card_getEffectiveValue(&grid->cards[row][col]) * SPECIAL_CLEAR_BONUS_MULTIPLIER;
                if (deck_isEmpty(deck)) continue;
                deck_discard(deck, grid->cards[row][col]);
                grid->cards[row][col] = deck_drawCard(deck);
                applySlotRot(grid, row, col);
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
    if (anyHalving) grid->stackScore /= 2;

    for (int row = 0; row < grid->size; row++)
        for (int col = 0; col < grid->size; col++)
            if (result.cellInvolved[row][col])
                result.totalScoreGained += refillCell(grid, deck, row, col);

    return result;
}
