#ifndef MODELER_CHESS_BOARD_H
#define MODELER_CHESS_BOARD_H

#include <stdbool.h>

#include "window.h"
#include "buffer.h"
#include "vulkan_utils.h"
#include "vk_mem_alloc.h"

typedef struct chess_board_component_t *ChessBoard;

bool createChessBoard(ChessBoard *chessBoard, VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue queue, VkRenderPass renderPass, uint32_t subpass, VkExtent2D extent, const char *resourcePath, float aspectRatio, float width, float originX, float originY, char **error);
bool drawChessBoard(ChessBoard self, VkCommandBuffer commandBuffer, WindowDimensions initialWindowDimensions, char **error);
void destroyChessBoard(ChessBoard self);
bool updateChessBoard(ChessBoard self, float aspectRatio, float width, float originX, float originY, char **error);

#endif /* MODELER_CHESS_BOARD_H */
