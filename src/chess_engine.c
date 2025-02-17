#include <stdbool.h>
#include <stdlib.h>

#include "chess_engine.h"
#include "utils.h"

struct chess_engine_t {
	Board8x8 board;
	ChessSquare lastSelected;
	ChessBoard *chessBoard;
};

void basicSetBoard(ChessEngine self, Board8x8 board);

static inline bool hasLastSelected(ChessEngine self)
{
	return self->lastSelected < CHESS_SQUARE_COUNT;
}

void createChessEngine(ChessEngine *chessEngine, ChessBoard *chessBoard)
{
	*chessEngine = malloc(sizeof(**chessEngine));

	ChessEngine self = *chessEngine;

	self->chessBoard = chessBoard;

	Board8x8 initialSetup = {
		BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK,
		BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN,
		WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK
	};
	basicSetBoard(self, initialSetup);

	self->lastSelected = CHESS_SQUARE_COUNT;
}

void chessEngineSquareSelected(ChessEngine self, ChessSquare square)
{
	char **error;

	if (self->lastSelected == square) {
		self->lastSelected = CHESS_SQUARE_COUNT;
		chessBoardSetSelected(*self->chessBoard, self->lastSelected);
	} else if (hasLastSelected(self)) {
		self->board[square] = self->board[self->lastSelected];
		self->board[self->lastSelected] = EMPTY;

		LastMove lastMove = {
			.from = self->lastSelected,
			.to = square
		};

		self->lastSelected = CHESS_SQUARE_COUNT;

		chessBoardSetBoard(*self->chessBoard, self->board);
		chessBoardSetLastMove(*self->chessBoard, lastMove);
		chessBoardSetSelected(*self->chessBoard, self->lastSelected);
	} else if (self->board[square] != EMPTY) {
		self->lastSelected = square;
		chessBoardSetSelected(*self->chessBoard, self->lastSelected);
	}

	if (!updateChessBoard(*self->chessBoard, error)) {
		asprintf(error, "Failed to update chess board.\n");
		// return false;
	}
}

void basicSetBoard(ChessEngine self, Board8x8 board)
{
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->board[i] = board[i];
	}
}

void chessEngineSetBoard(ChessEngine self, Board8x8 board)
{
	char **error;

	basicSetBoard(self, board);

	chessBoardSetBoard(*self->chessBoard, self->board);

	if (!updateChessBoard(*self->chessBoard, error)) {
		asprintf(error, "Failed to update chess board.\n");
		// return false;
	}
}

void chessEngineReset(ChessEngine self)
{
	self->lastSelected = CHESS_SQUARE_COUNT;
	chessBoardSetSelected(*self->chessBoard, self->lastSelected);

	LastMove lastMove = {
		.from = CHESS_SQUARE_COUNT,
		.to = CHESS_SQUARE_COUNT
	};
	chessBoardSetLastMove(*self->chessBoard, lastMove);

	Board8x8 initialSetup = {
		BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK,
		BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
		WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN,
		WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK
	};
	chessEngineSetBoard(self, initialSetup);
}
