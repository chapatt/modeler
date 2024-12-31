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

bool createChessBoard(ChessBoard *chessBoard, ChessEngine engine, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, const char *resourcePath, float aspectRatio, float width, float originX, float originY, char **error);
bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, char **error);
void destroyChessBoard(ChessBoard self);
void setSize(ChessBoard self, float aspectRatio, float width, float originX, float originY);
void setBoard(ChessBoard self, Board8x8 board);
void setMove(ChessBoard self, MoveBoard8x8 move);
bool updateChessBoard(ChessBoard self, char **error);
void chessBoardHandleInputEvent(void *chessBoard, InputEvent *inputEvent);
void setSelected(ChessBoard self, ChessSquare selected);
void setLastMove(ChessBoard self, LastMove lastMove);

#endif /* MODELER_CHESS_BOARD_H */
