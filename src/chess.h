#ifndef MODELER_CHESS_H
#define MODELER_CHESS_H

#include <stddef.h>

#define CHESS_SQUARE_COUNT 8 * 8

typedef enum piece_t
{
	EMPTY,
	BLACK_PAWN,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,
	WHITE_PAWN,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING
} Piece;

typedef enum move_t
{
	ILLEGAL,
	OPEN,
	CAPTURE
} Move;

typedef Piece Board8x8[CHESS_SQUARE_COUNT];

typedef Move MoveBoard8x8[CHESS_SQUARE_COUNT];

typedef size_t ChessSquare;

#endif /* MODELER_CHESS_H */