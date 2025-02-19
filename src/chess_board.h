#ifndef MODELER_CHESS_BOARD_H
#define MODELER_CHESS_BOARD_H

#include <stdbool.h>

typedef struct chess_board_t *ChessBoard;

#include "chess.h"
#include "chess_engine.h"
#include "input_event.h"
#include "window.h"
#include "buffer.h"
#include "vulkan_utils.h"
#include "vk_mem_alloc.h"

typedef enum projection_t {
	ORTHOGRAPHIC,
	PERSPECTIVE
} Projection;

bool createChessBoard(ChessBoard *chessBoard, ChessEngine engine, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, float width, float originX, float originY, Orientation orientation, bool enable3d, Projection projection, char **error);
bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, char **error);
void destroyChessBoard(ChessBoard self);
bool updateChessBoard(ChessBoard self, char **error);
void chessBoardHandleInputEvent(void *chessBoard, InputEvent *inputEvent);
void chessBoardSetDimensions(ChessBoard self, float width, float originX, float originY, Orientation orientation);
void chessBoardSetBoard(ChessBoard self, Board8x8 board);
void chessBoardSetMove(ChessBoard self, MoveBoard8x8 move);
void chessBoardSetSelected(ChessBoard self, ChessSquare selected);
void chessBoardSetLastMove(ChessBoard self, LastMove lastMove);
bool chessBoardGetEnable3d(ChessBoard self);
void chessBoardSetEnable3d(ChessBoard self, bool enable3d);
Projection chessBoardGetProjection(ChessBoard self);
void chessBoardSetProjection(ChessBoard self, Projection projection);

#endif /* MODELER_CHESS_BOARD_H */
