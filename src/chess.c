#include "chess.h"

#include <stdlib.h>
#include <string.h>

#include "i18n.h"

#define CHESS_MAX_DESTS 20

static bool inBounds(int r, int c)
{
    return r >= 0 && r < CHESS_BOARD_SIZE && c >= 0 && c < CHESS_BOARD_SIZE;
}

static ChessPiece *pieceAt(ChessBoard *b, int r, int c)
{
    for (int i = 0; i < b->count; i++)
        if (b->pieces[i].alive && b->pieces[i].row == r && b->pieces[i].col == c)
            return &b->pieces[i];
    return NULL;
}

void chess_boardClear(ChessBoard *b)
{
    b->count = 0;
}

bool chess_placePiece(ChessBoard *b, ChessSide side, ChessPieceType type, int row, int col)
{
    if (col < 0 || col >= CHESS_BOARD_SIZE) return false;
    if (side == CHESS_SIDE_PLAYER)
    {
        if (row != CHESS_BOARD_SIZE - 2 && row != CHESS_BOARD_SIZE - 1) return false;
    }
    else
    {
        if (row != 0 && row != 1) return false;
    }
    if (pieceAt(b, row, col) != NULL) return false;
    if (b->count >= CHESS_MAX_PIECES) return false;

    ChessPiece *p = &b->pieces[b->count++];
    p->type = type;
    p->side = side;
    p->alive = true;
    p->row = row;
    p->col = col;
    return true;
}

bool chess_removePieceAt(ChessBoard *b, ChessSide side, int row, int col, ChessPieceType *outType)
{
    for (int i = 0; i < b->count; i++)
    {
        ChessPiece *p = &b->pieces[i];
        if (p->alive && p->side == side && p->row == row && p->col == col)
        {
            if (outType) *outType = p->type;
            for (int j = i; j < b->count - 1; j++) b->pieces[j] = b->pieces[j + 1];
            b->count--;
            return true;
        }
    }
    return false;
}

int chess_pieceCost(ChessPieceType type)
{
    switch (type)
    {
        case CHESS_PAWN:   return 2;
        case CHESS_KNIGHT: return 4;
        case CHESS_BISHOP: return 4;
        case CHESS_ROOK:   return 6;
        case CHESS_QUEEN:  return 9;
        case CHESS_KING:   return 3;
        default:           return 0;
    }
}

const char *chess_pieceName(ChessPieceType type)
{
    switch (type)
    {
        case CHESS_PAWN:   return tr(STR_PIECE_PAWN);
        case CHESS_KNIGHT: return tr(STR_PIECE_KNIGHT);
        case CHESS_BISHOP: return tr(STR_PIECE_BISHOP);
        case CHESS_ROOK:   return tr(STR_PIECE_ROOK);
        case CHESS_QUEEN:  return tr(STR_PIECE_QUEEN);
        case CHESS_KING:   return tr(STR_PIECE_KING);
        default:           return "?";
    }
}

char chess_pieceGlyph(ChessPieceType type)
{
    switch (type)
    {
        case CHESS_PAWN:   return 'P';
        case CHESS_KNIGHT: return 'N';
        case CHESS_BISHOP: return 'B';
        case CHESS_ROOK:   return 'R';
        case CHESS_QUEEN:  return 'Q';
        case CHESS_KING:   return 'K';
        default:           return '?';
    }
}

void chess_buildAiArmy(ChessBoard *b, int matchesPlayed, bool isBossRound)
{

    int armySize = 3 + matchesPlayed / 2;
    if (armySize > 10) armySize = 10;
    if (isBossRound && armySize < 10) armySize++;

    int nonPawn = 0;
    if (matchesPlayed >= 3)  nonPawn++;
    if (matchesPlayed >= 6)  nonPawn++;
    if (matchesPlayed >= 10) nonPawn++;
    if (nonPawn > armySize) nonPawn = armySize;
    int pawnCount = armySize - nonPawn;

    static const ChessPieceType TIER1[3] = { CHESS_KNIGHT, CHESS_BISHOP, CHESS_KING };

    ChessPieceType types[10];
    int n = 0;
    for (int i = 0; i < pawnCount; i++) types[n++] = CHESS_PAWN;
    if (matchesPlayed >= 3  && n < armySize) types[n++] = TIER1[rand() % 3];
    if (matchesPlayed >= 6  && n < armySize) types[n++] = CHESS_ROOK;
    if (matchesPlayed >= 10 && n < armySize) types[n++] = CHESS_QUEEN;

    int coordR[10], coordC[10], coordCount = 0;
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < CHESS_BOARD_SIZE; c++)
            if (pieceAt(b, r, c) == NULL)
            {
                coordR[coordCount] = r;
                coordC[coordCount] = c;
                coordCount++;
            }

    for (int i = coordCount - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tr = coordR[i]; coordR[i] = coordR[j]; coordR[j] = tr;
        int tc = coordC[i]; coordC[i] = coordC[j]; coordC[j] = tc;
    }

    for (int i = 0; i < n && i < coordCount; i++)
        chess_placePiece(b, CHESS_SIDE_AI, types[i], coordR[i], coordC[i]);
}

bool chess_reinforceAi(ChessBoard *b)
{
    int pawnIdx = -1, aiCount = 0;
    for (int i = 0; i < b->count; i++)
    {
        if (!b->pieces[i].alive || b->pieces[i].side != CHESS_SIDE_AI) continue;
        aiCount++;
        if (b->pieces[i].type == CHESS_PAWN && b->pieces[i].row == CHESS_BOARD_SIZE - 1 && pawnIdx < 0)
            pawnIdx = i;
    }

    if (pawnIdx >= 0 && (aiCount >= 5 || rand() % 2 == 0))
    {
        static const ChessPieceType PROMOTIONS[3] = { CHESS_KNIGHT, CHESS_BISHOP, CHESS_ROOK };
        b->pieces[pawnIdx].type = PROMOTIONS[rand() % 3];
        return true;
    }

    if (aiCount >= 10) return false;

    int emptyR[10], emptyC[10], emptyCount = 0;
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < CHESS_BOARD_SIZE; c++)
            if (pieceAt(b, r, c) == NULL) { emptyR[emptyCount] = r; emptyC[emptyCount] = c; emptyCount++; }
    if (emptyCount == 0) return false;

    int pick = rand() % emptyCount;
    static const ChessPieceType REINFORCE_TYPES[4] = { CHESS_PAWN, CHESS_PAWN, CHESS_KNIGHT, CHESS_BISHOP };
    return chess_placePiece(b, CHESS_SIDE_AI, REINFORCE_TYPES[rand() % 4], emptyR[pick], emptyC[pick]);
}

static void genSlide(ChessBoard *b, const ChessPiece *p, const int dirs[][2], int dirCount,
                      int destR[], int destC[], bool destIsCapture[], int *outCount)
{
    int n = 0;
    for (int d = 0; d < dirCount; d++)
    {
        int r = p->row + dirs[d][0], c = p->col + dirs[d][1];
        while (inBounds(r, c) && n < CHESS_MAX_DESTS)
        {
            ChessPiece *occ = pieceAt(b, r, c);
            if (occ == NULL)
            {
                destR[n] = r; destC[n] = c; destIsCapture[n] = false; n++;
            }
            else
            {
                if (occ->side != p->side)
                {
                    destR[n] = r; destC[n] = c; destIsCapture[n] = true; n++;
                }
                break;
            }
            r += dirs[d][0]; c += dirs[d][1];
        }
    }
    *outCount = n;
}

static void genSteps(ChessBoard *b, const ChessPiece *p, const int offs[][2], int offCount,
                      int destR[], int destC[], bool destIsCapture[], int *outCount)
{
    int n = 0;
    for (int i = 0; i < offCount && n < CHESS_MAX_DESTS; i++)
    {
        int r = p->row + offs[i][0], c = p->col + offs[i][1];
        if (!inBounds(r, c)) continue;
        ChessPiece *occ = pieceAt(b, r, c);
        if (occ == NULL)
        {
            destR[n] = r; destC[n] = c; destIsCapture[n] = false; n++;
        }
        else if (occ->side != p->side)
        {
            destR[n] = r; destC[n] = c; destIsCapture[n] = true; n++;
        }
    }
    *outCount = n;
}

static void genPawn(ChessBoard *b, const ChessPiece *p, int destR[], int destC[], bool destIsCapture[], int *outCount)
{
    int n = 0;
    int dir = (p->side == CHESS_SIDE_PLAYER) ? -1 : 1;
    int fr = p->row + dir;

    if (inBounds(fr, p->col) && pieceAt(b, fr, p->col) == NULL)
    {
        destR[n] = fr; destC[n] = p->col; destIsCapture[n] = false; n++;
    }
    int diagCols[2] = { p->col - 1, p->col + 1 };
    for (int k = 0; k < 2; k++)
    {
        int c = diagCols[k];
        if (!inBounds(fr, c)) continue;
        ChessPiece *occ = pieceAt(b, fr, c);
        if (occ != NULL && occ->side != p->side)
        {
            destR[n] = fr; destC[n] = c; destIsCapture[n] = true; n++;
        }
    }
    *outCount = n;
}

static void generateMoves(ChessBoard *b, const ChessPiece *p, int destR[], int destC[], bool destIsCapture[], int *outCount)
{
    static const int KNIGHT_OFFS[8][2] = { {-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1} };
    static const int KING_OFFS[8][2]   = { {-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1} };
    static const int BISHOP_DIRS[4][2] = { {-1,-1},{-1,1},{1,-1},{1,1} };
    static const int ROOK_DIRS[4][2]   = { {-1,0},{1,0},{0,-1},{0,1} };
    static const int QUEEN_DIRS[8][2]  = { {-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1} };

    switch (p->type)
    {
        case CHESS_PAWN:   genPawn(b, p, destR, destC, destIsCapture, outCount); break;
        case CHESS_KNIGHT: genSteps(b, p, KNIGHT_OFFS, 8, destR, destC, destIsCapture, outCount); break;
        case CHESS_KING:   genSteps(b, p, KING_OFFS, 8, destR, destC, destIsCapture, outCount); break;
        case CHESS_BISHOP: genSlide(b, p, BISHOP_DIRS, 4, destR, destC, destIsCapture, outCount); break;
        case CHESS_ROOK:   genSlide(b, p, ROOK_DIRS, 4, destR, destC, destIsCapture, outCount); break;
        case CHESS_QUEEN:  genSlide(b, p, QUEEN_DIRS, 8, destR, destC, destIsCapture, outCount); break;
        default:           *outCount = 0; break;
    }
}

static int chebyshev(int r1, int c1, int r2, int c2)
{
    int dr = r1 > r2 ? r1 - r2 : r2 - r1;
    int dc = c1 > c2 ? c1 - c2 : c2 - c1;
    return dr > dc ? dr : dc;
}

static int nearestEnemyDist(const ChessBoard *b, ChessSide side, int r, int c)
{
    int best = 1000;
    for (int i = 0; i < b->count; i++)
    {
        const ChessPiece *e = &b->pieces[i];
        if (!e->alive || e->side == side) continue;
        int d = chebyshev(r, c, e->row, e->col);
        if (d < best) best = d;
    }
    return best;
}

bool chess_stepOneMove(ChessBoard *b, ChessSide side, ChessMoveRecord *outMove)
{
    memset(outMove, 0, sizeof(*outMove));
    outMove->side = side;

    bool anyAlive = false;
    for (int i = 0; i < b->count; i++)
        if (b->pieces[i].alive && b->pieces[i].side == side) { anyAlive = true; break; }
    if (!anyAlive) return false;

    int bestIdx = -1, bestR = 0, bestC = 0, bestCaptureValue = -1;
    for (int i = 0; i < b->count; i++)
    {
        ChessPiece *p = &b->pieces[i];
        if (!p->alive || p->side != side) continue;
        int dr[CHESS_MAX_DESTS], dc[CHESS_MAX_DESTS], n;
        bool cap[CHESS_MAX_DESTS];
        generateMoves(b, p, dr, dc, cap, &n);
        for (int k = 0; k < n; k++)
        {
            if (!cap[k]) continue;
            ChessPiece *target = pieceAt(b, dr[k], dc[k]);
            int val = chess_pieceCost(target->type);
            if (val > bestCaptureValue) { bestCaptureValue = val; bestIdx = i; bestR = dr[k]; bestC = dc[k]; }
        }
    }

    if (bestIdx >= 0)
    {
        ChessPiece *p = &b->pieces[bestIdx];
        ChessPiece *target = pieceAt(b, bestR, bestC);
        outMove->moved = true;
        outMove->fromRow = p->row; outMove->fromCol = p->col;
        outMove->toRow = bestR; outMove->toCol = bestC;
        outMove->movedType = p->type;
        outMove->wasCapture = true;
        outMove->capturedType = target->type;
        target->alive = false;
        p->row = bestR; p->col = bestC;
        return true;
    }

    int bestDist = 1000;
    for (int i = 0; i < b->count; i++)
    {
        ChessPiece *p = &b->pieces[i];
        if (!p->alive || p->side != side) continue;
        int dr[CHESS_MAX_DESTS], dc[CHESS_MAX_DESTS], n;
        bool cap[CHESS_MAX_DESTS];
        generateMoves(b, p, dr, dc, cap, &n);
        for (int k = 0; k < n; k++)
        {
            if (cap[k]) continue;
            int d = nearestEnemyDist(b, side, dr[k], dc[k]);
            if (d < bestDist) { bestDist = d; bestIdx = i; bestR = dr[k]; bestC = dc[k]; }
        }
    }

    if (bestIdx >= 0)
    {
        ChessPiece *p = &b->pieces[bestIdx];
        outMove->moved = true;
        outMove->fromRow = p->row; outMove->fromCol = p->col;
        outMove->toRow = bestR; outMove->toCol = bestC;
        outMove->movedType = p->type;
        outMove->wasCapture = false;
        p->row = bestR; p->col = bestC;
        return true;
    }

    return true;
}

bool chess_sideEliminated(const ChessBoard *b, ChessSide side)
{
    for (int i = 0; i < b->count; i++)
        if (b->pieces[i].alive && b->pieces[i].side == side) return false;
    return true;
}

void chess_returnSurvivorsAndClear(ChessBoard *b, ChessSide side, int rosterCounts[CHESS_PIECE_TYPE_COUNT])
{
    for (int i = 0; i < b->count; i++)
        if (b->pieces[i].alive && b->pieces[i].side == side)
            rosterCounts[b->pieces[i].type]++;

    int w = 0;
    for (int i = 0; i < b->count; i++)
        if (b->pieces[i].side != side)
            b->pieces[w++] = b->pieces[i];
    b->count = w;
}
