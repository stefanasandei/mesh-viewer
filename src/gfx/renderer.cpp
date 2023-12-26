//
// Created by Stefan on 12/24/2023.
//

#include "gfx/renderer.hpp"

#include "global.hpp"

namespace gfx {

Renderer::Renderer() {
  m_FramesCount = 0;

  InitCommands();
  InitSyncStructures();
  InitVma();
  InitDrawTarget();
}

Renderer::~Renderer() {
  global.context->Device.waitIdle();

  global.context->DeletionQueue.Flush();

  for (std::size_t i = 0; i < FRAME_COUNT; i++) {
    global.context->Device.destroyCommandPool(m_Frames[i].CmdPool);

    global.context->Device.destroyFence(m_Frames[i].RenderFence);
    global.context->Device.destroySemaphore(m_Frames[i].RenderSemaphore);
    global.context->Device.destroySemaphore(m_Frames[i].SwapchainSemaphore);
  }
}

void Renderer::Draw() {
  // wait & reset the fence
  VK_CHECK(global.context->Device.waitForFences(1, &GetFrame().RenderFence,
                                                true, 1000000000));

  GetFrame().DeletionQueue.Flush();

  VK_CHECK(global.context->Device.resetFences(1, &GetFrame().RenderFence));

  // grab the swapchain image
  uint32_t swapchain_image_index = 0;
  vk::SwapchainKHR native_swapchain =
      *static_cast<vk::SwapchainKHR*>(&global.swapchain->NativeSwapchain);
  VK_CHECK(global.context->Device.acquireNextImageKHR(
      native_swapchain, 1000000000, GetFrame().SwapchainSemaphore, nullptr,
      &swapchain_image_index));

  vk::Image current_img = global.swapchain->Images[swapchain_image_index];

  vk::ImageView current_img_view =
      global.swapchain->ImageViews[swapchain_image_index];

  // reset the main cmd buffer
  GetFrame().MainCmdBuffer.reset();

  vk::CommandBufferBeginInfo cmd_begin_info;
  cmd_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  VK_CHECK(GetFrame().MainCmdBuffer.begin(&cmd_begin_info));

  // start drawing
  TransitionImage(GetFrame().MainCmdBuffer, m_DrawImage.Image,
                  vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

  DrawBackground(GetFrame().MainCmdBuffer, m_DrawImage.Image);

  TransitionImage(GetFrame().MainCmdBuffer, m_DrawImage.Image,
                  vk::ImageLayout::eGeneral,
                  vk::ImageLayout::eTransferSrcOptimal);

  TransitionImage(GetFrame().MainCmdBuffer, current_img,
                  vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eTransferDstOptimal);

  CopyImageToImage(GetFrame().MainCmdBuffer, m_DrawImage.Image, current_img,
                   m_DrawImage.Extent);

  TransitionImage(GetFrame().MainCmdBuffer, current_img,
                  vk::ImageLayout::eTransferDstOptimal,
                  vk::ImageLayout::ePresentSrcKHR);

  // close the command buffer
  GetFrame().MainCmdBuffer.end();

  // prepare the submission to the queue.
  vk::CommandBufferSubmitInfo cmd_info;
  cmd_info.setCommandBuffer(GetFrame().MainCmdBuffer);
  cmd_info.setDeviceMask(0);

  vk::SemaphoreSubmitInfo wait_info;
  wait_info.setSemaphore(GetFrame().SwapchainSemaphore);
  wait_info.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
  wait_info.setDeviceIndex(0);
  wait_info.setValue(1);

  vk::SemaphoreSubmitInfo signal_info;
  signal_info.setSemaphore(GetFrame().RenderSemaphore);
  signal_info.setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);
  signal_info.setDeviceIndex(0);
  signal_info.setValue(1);

  vk::SubmitInfo2 submit;
  submit.setWaitSemaphoreInfoCount(1);
  submit.setPWaitSemaphoreInfos(&wait_info);
  submit.setSignalSemaphoreInfoCount(1);
  submit.setPSignalSemaphoreInfos(&signal_info);
  submit.setCommandBufferInfoCount(1);
  submit.setPCommandBufferInfos(&cmd_info);

  VK_CHECK(global.context->GraphicsQueue.submit2(1, &submit,
                                                 GetFrame().RenderFence));

  // present the image
  vk::PresentInfoKHR present_info;
  present_info.setSwapchainCount(1);
  present_info.setPSwapchains(&native_swapchain);
  present_info.setWaitSemaphoreCount(1);
  present_info.setPWaitSemaphores(&GetFrame().RenderSemaphore);
  present_info.setPImageIndices(&swapchain_image_index);

  VK_CHECK(global.context->GraphicsQueue.presentKHR(&present_info));

  m_FramesCount++;
}

void Renderer::DrawBackground(vk::CommandBuffer cmd, vk::Image target) {
  // make a clear-color from frame number. This will flash with a 120 frame
  // period.
  vk::ClearColorValue clearValue;
  float flash = abs(sin(m_FramesCount / 120.f));
  clearValue.setFloat32({flash, 0.8f, 1.0f, 1.0f});

  vk::ImageSubresourceRange clearRange =
      ImageSubresourceRange(vk::ImageAspectFlagBits::eColor);

  // clear image
  cmd.clearColorImage(target, vk::ImageLayout::eGeneral, &clearValue, 1,
                      &clearRange);
}

void Renderer::InitCommands() {
  vk::CommandPoolCreateInfo cmd_pool_info;
  cmd_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  cmd_pool_info.setQueueFamilyIndex(global.context->GraphicsQueueFamily);

  for (std::size_t i = 0; i < FRAME_COUNT; i++) {
    VK_CHECK(global.context->Device.createCommandPool(&cmd_pool_info, nullptr,
                                                      &m_Frames[i].CmdPool));

    vk::CommandBufferAllocateInfo cmd_alloc_info;
    cmd_alloc_info.setCommandPool(m_Frames[i].CmdPool);
    cmd_alloc_info.setCommandBufferCount(1);
    cmd_alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);

    VK_CHECK(global.context->Device.allocateCommandBuffers(
        &cmd_alloc_info, &m_Frames[i].MainCmdBuffer));
  }
}

void Renderer::InitSyncStructures() {
  vk::FenceCreateInfo fence_info;
  fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

  vk::SemaphoreCreateInfo semaphore_info;

  for (std::size_t i = 0; i < FRAME_COUNT; i++) {
    VK_CHECK(global.context->Device.createFence(&fence_info, nullptr,
                                                &m_Frames[i].RenderFence));

    VK_CHECK(global.context->Device.createSemaphore(
        &semaphore_info, nullptr, &m_Frames[i].SwapchainSemaphore));
    VK_CHECK(global.context->Device.createSemaphore(
        &semaphore_info, nullptr, &m_Frames[i].RenderSemaphore));
  }
}

void Renderer::InitVma() {
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = global.context->PhysicalDevice;
  allocator_info.device = global.context->Device;
  allocator_info.instance = global.context->Instance;
  allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  vmaCreateAllocator(&allocator_info, &m_Allocator);

  global.context->DeletionQueue.Push(
      [&]() { vmaDestroyAllocator(m_Allocator); });
}

void Renderer::InitDrawTarget() {
  VkImageUsageFlags usages{};
  usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  usages |= VK_IMAGE_USAGE_STORAGE_BIT;
  usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  m_DrawImage.SetAllocator(m_Allocator, global.context->Device);

  m_DrawImage.Create(global.window->GetSize(), vk::Format::eR16G16B16A16Sfloat,
                     usages);

  global.context->DeletionQueue.Push([&]() { m_DrawImage.Destroy(); });
}

FrameData& Renderer::GetFrame() {
  return m_Frames[m_FramesCount % FRAME_COUNT];
}

}  // namespace gfx