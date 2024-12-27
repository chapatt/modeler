#ifndef MODELER_CHESS_ENGINE_H
#define MODELER_CHESS_ENGINE_H

typedef struct chess_engine_t *ChessEngine;

#include "chess.h"
#include "chess_board.h"

void createChessEngine(ChessEngine *chessEngine, ChessBoard *chessBoard);
void chessEngineSquareSelected(ChessEngine self, ChessSquare square);

#endif /* MODELER_CHESS_ENGINE_H */
