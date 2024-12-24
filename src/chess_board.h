#ifndef MODELER_CHESS_BOARD_H
#define MODELER_CHESS_BOARD_H

#include <stdbool.h>

#include "window.h"
#include "buffer.h"
#include "vulkan_utils.h"
#include "vk_mem_alloc.h"

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

typedef Piece Board8x8[64];

typedef struct chess_board_t *ChessBoard;

bool createChessBoard(ChessBoard *chessBoard, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, const char *resourcePath, float anisotropy, float aspectRatio, float width, float originX, float originY, char **error);
bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, WindowDimensions initialWindowDimensions, char **error);
void destroyChessBoard(ChessBoard self);
void setSize(ChessBoard self, float aspectRatio, float width, float originX, float originY);
void setBoard(ChessBoard self, Board8x8 board);
bool updateChessBoard(ChessBoard self, char **error);

#endif /* MODELER_CHESS_BOARD_H */
