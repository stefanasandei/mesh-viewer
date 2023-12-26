//
// Created by Stefan on 12/24/2023.
//

#pragma once

#include "gfx/context.hpp"
#include "gfx/allocated_image.hpp"

namespace gfx {

constexpr uint32_t FRAME_COUNT = 2;

struct FrameData {
  vk::CommandPool CmdPool;
  vk::CommandBuffer MainCmdBuffer;

  vk::Semaphore SwapchainSemaphore, RenderSemaphore;
  vk::Fence RenderFence;

  memory::DeletionQueue DeletionQueue;
};

class Renderer {
 public:
  Renderer();
  ~Renderer();

  void Draw();

 private:
  void InitCommands();
  void InitSyncStructures();
  void InitVma();
  void InitDrawTarget();

  void DrawBackground(vk::CommandBuffer cmd, vk::Image target);

  FrameData& GetFrame();

 private:
  uint64_t m_FramesCount;
  std::array<FrameData, FRAME_COUNT> m_Frames;

  VmaAllocator m_Allocator;

  AllocatedImage m_DrawImage;
};

}  // namespace gfx
