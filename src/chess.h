#ifndef CHESS_H
#define CHESS_H

#include <stdbool.h>

#define CHESS_BOARD_SIZE 5
#define CHESS_MAX_PIECES 20 

typedef enum ChessPieceType {
    CHESS_PAWN,
    CHESS_KNIGHT,
    CHESS_BISHOP,
    CHESS_ROOK,
    CHESS_QUEEN,
    CHESS_KING,
    CHESS_PIECE_TYPE_COUNT
} ChessPieceType;

typedef enum ChessSide {
    CHESS_SIDE_PLAYER,
    CHESS_SIDE_AI
} ChessSide;

typedef struct ChessPiece {
    ChessPieceType type;
    ChessSide side;
    bool alive;
    int row, col;
} ChessPiece;

typedef struct ChessBoard {
    ChessPiece pieces[CHESS_MAX_PIECES];
    int count;
} ChessBoard;

typedef struct ChessMoveRecord {
    bool moved;         
    int fromRow, fromCol;
    int toRow, toCol;
    ChessPieceType movedType;
    ChessSide side;
    bool wasCapture;
    ChessPieceType capturedType;
} ChessMoveRecord;

void chess_boardClear(ChessBoard *b);

bool chess_placePiece(ChessBoard *b, ChessSide side, ChessPieceType type, int row, int col);

bool chess_removePieceAt(ChessBoard *b, ChessSide side, int row, int col, ChessPieceType *outType);

int  chess_pieceCost(ChessPieceType type);
const char *chess_pieceName(ChessPieceType type);
char chess_pieceGlyph(ChessPieceType type);

void chess_buildAiArmy(ChessBoard *b, int roundNumber, bool isBossRound);

bool chess_stepOneMove(ChessBoard *b, ChessSide side, ChessMoveRecord *outMove);

bool chess_sideEliminated(const ChessBoard *b, ChessSide side);

void chess_returnSurvivorsAndClear(ChessBoard *b, ChessSide side, int rosterCounts[CHESS_PIECE_TYPE_COUNT]);

#endif
