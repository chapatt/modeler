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
	PERSPECTIVE,
	ORTHOGRAPHIC
} Projection;

bool createChessBoard(ChessBoard *chessBoard, ChessEngine engine, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkSampleCountFlagBits sampleCount, const char *resourcePath, float width, float originX, float originY, Orientation orientation, bool enable3d, Projection projection, char **error);
bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, char **error);
void destroyChessBoard(ChessBoard self);
void setDimensions(ChessBoard self, float width, float originX, float originY, Orientation orientation);
void setBoard(ChessBoard self, Board8x8 board);
void setMove(ChessBoard self, MoveBoard8x8 move);
bool updateChessBoard(ChessBoard self, char **error);
void chessBoardHandleInputEvent(void *chessBoard, InputEvent *inputEvent);
void setSelected(ChessBoard self, ChessSquare selected);
void setLastMove(ChessBoard self, LastMove lastMove);

#endif /* MODELER_CHESS_BOARD_H */
