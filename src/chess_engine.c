#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "chess_engine.h"
#include "utils.h"

struct chess_engine_t {
	Board8x8 board;
	ChessSquare lastSelected;
	ChessBoard *chessBoard;
};

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
	for (size_t i = 0; i < CHESS_SQUARE_COUNT; ++i) {
		self->board[i] = initialSetup[i];
	}

	self->lastSelected = CHESS_SQUARE_COUNT;
}

void chessEngineSquareSelected(ChessEngine self, ChessSquare square)
{
	char **error;

	if (self->lastSelected == square) {
		self->lastSelected = CHESS_SQUARE_COUNT;
		setSelected(*self->chessBoard, self->lastSelected);
	} else if (hasLastSelected(self)) {
		printf("taking %d with %d\n", self->board[square], self->board[self->lastSelected]);
		self->board[square] = self->board[self->lastSelected];
		self->board[self->lastSelected] = EMPTY;

		LastMove lastMove = {
			.from = self->lastSelected,
			.to = square
		};

		self->lastSelected = CHESS_SQUARE_COUNT;

		setBoard(*self->chessBoard, self->board);
		setLastMove(*self->chessBoard, lastMove);
		setSelected(*self->chessBoard, self->lastSelected);
	} else if (self->board[square] != EMPTY) {
		printf("selecting: %d\n", square);
		self->lastSelected = square;
		setSelected(*self->chessBoard, self->lastSelected);
	}

	if (!updateChessBoard(*self->chessBoard, error)) {
		asprintf(error, "Failed to update chess board.\n");
		// return false;
	}
}
