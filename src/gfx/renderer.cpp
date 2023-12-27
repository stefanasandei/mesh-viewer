//
// Created by Stefan on 12/24/2023.
//

#include "gfx/renderer.hpp"

#include "global.hpp"

#include "gfx/pipeline_builder.hpp"
#include "gfx/shader.hpp"

#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gfx {

Renderer::Renderer() {
  m_FramesCount = 0;
  m_MeshIndex = 0;

  InitCommands();
  InitSyncStructures();
  InitVma();
  InitDrawTarget();
  InitPipeline();
}

Renderer::~Renderer() {
  global.context->Device.waitIdle();

  m_DrawImage.Destroy();
  m_DepthImage.Destroy();

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
  vk::Result res = global.context->Device.acquireNextImageKHR(
      native_swapchain, 1000000000, GetFrame().SwapchainSemaphore, nullptr,
      &swapchain_image_index);
  if (res == vk::Result::eErrorOutOfDateKHR) {
    Resize();
    return;
  }

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
                  vk::ImageLayout::eColorAttachmentOptimal);

  TransitionImage(GetFrame().MainCmdBuffer, m_DepthImage.Image,
                  vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eDepthAttachmentOptimal);

  DrawGeometry(GetFrame().MainCmdBuffer);

  TransitionImage(GetFrame().MainCmdBuffer, m_DrawImage.Image,
                  vk::ImageLayout::eColorAttachmentOptimal,
                  vk::ImageLayout::eTransferSrcOptimal);

  TransitionImage(GetFrame().MainCmdBuffer, current_img,
                  vk::ImageLayout::eUndefined,
                  vk::ImageLayout::eTransferDstOptimal);

  CopyImageToImage(GetFrame().MainCmdBuffer, m_DrawImage.Image, current_img,
                   m_DrawImage.Extent);

  TransitionImage(GetFrame().MainCmdBuffer, current_img,
                  vk::ImageLayout::eTransferDstOptimal,
                  vk::ImageLayout::eColorAttachmentOptimal);

  DrawImGui(GetFrame().MainCmdBuffer, current_img_view);

  TransitionImage(GetFrame().MainCmdBuffer, current_img,
                  vk::ImageLayout::eColorAttachmentOptimal,
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

  vk::Result presentResult =
      global.context->GraphicsQueue.presentKHR(&present_info);
  if (presentResult == vk::Result::eErrorOutOfDateKHR) {
    Resize();
    return;
  }

  m_FramesCount++;
}

void Renderer::DrawBackground(vk::CommandBuffer cmd, vk::Image target) {
  vk::ClearColorValue clearValue;
  clearValue.setFloat32({0.1f, 0.1f, 0.1f, 1.0f});

  vk::ImageSubresourceRange clearRange =
      ImageSubresourceRange(vk::ImageAspectFlagBits::eColor);

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

  // allocate the command buffer for the immediate mode
  VK_CHECK(global.context->Device.createCommandPool(&cmd_pool_info, nullptr,
                                                    &m_Immediate.CmdPool));

  vk::CommandBufferAllocateInfo cmd_alloc_info;
  cmd_alloc_info.setCommandPool(m_Immediate.CmdPool);
  cmd_alloc_info.setCommandBufferCount(1);
  cmd_alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);

  VK_CHECK(global.context->Device.allocateCommandBuffers(
      &cmd_alloc_info, &m_Immediate.MainCmdBuffer));
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

  VK_CHECK(global.context->Device.createFence(&fence_info, nullptr,
                                              &m_Immediate.RenderFence));
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

  m_DepthImage.Format = vk::Format::eD32Sfloat;
  m_DepthImage.Extent = m_DrawImage.Extent;
  VkImageUsageFlags depthImageUsages{};
  depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  m_DepthImage.SetAllocator(m_Allocator, global.context->Device);

  m_DepthImage.Create(global.window->GetSize(), vk::Format::eD32Sfloat,
                      depthImageUsages);
}

FrameData& Renderer::GetFrame() {
  return m_Frames[m_FramesCount % FRAME_COUNT];
}

void Renderer::ImmediateSubmit(
    std::function<void(VkCommandBuffer cmd)>&& function) {
  VK_CHECK(global.context->Device.resetFences(1, &m_Immediate.RenderFence));
  m_Immediate.MainCmdBuffer.reset();

  vk::CommandBuffer cmd = m_Immediate.MainCmdBuffer;
  vk::CommandBufferBeginInfo cmd_begin_info;
  cmd_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  VK_CHECK(cmd.begin(&cmd_begin_info));

  function(cmd);

  cmd.end();

  vk::CommandBufferSubmitInfo cmd_info;
  cmd_info.setCommandBuffer(cmd);
  cmd_info.setDeviceMask(0);

  vk::SubmitInfo2 submit;
  submit.setCommandBufferInfoCount(1);
  submit.setPCommandBufferInfos(&cmd_info);

  VK_CHECK(global.context->GraphicsQueue.submit2(1, &submit,
                                                 m_Immediate.RenderFence));

  VK_CHECK(global.context->Device.waitForFences(1, &m_Immediate.RenderFence,
                                                true, 9999999999));
}

void Renderer::DrawImGui(vk::CommandBuffer cmd, vk::ImageView target) {
  vk::RenderingAttachmentInfo color_attachment;
  color_attachment.setImageView(target);
  color_attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
  color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
  color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);

  vk::Rect2D rect;
  rect.setExtent({m_DrawImage.Extent.width, m_DrawImage.Extent.height});

  vk::RenderingInfo render_info;
  render_info.setRenderArea(rect);
  render_info.setLayerCount(1);
  render_info.setViewMask(0);
  render_info.setColorAttachmentCount(1);
  render_info.setPColorAttachments(&color_attachment);

  cmd.beginRendering(&render_info);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

  cmd.endRendering();
}

void Renderer::InitPipeline() {
  vk::ShaderModule triangle_vertex_shader;
  util::error::ErrNDie(
      !LoadShaderModule("./res/shaders/compiled/basic_vert.spv",
                        global.context->Device, &triangle_vertex_shader),
      "Failed to load vertex shader.");

  vk::ShaderModule triangle_frag_shader;
  util::error::ErrNDie(
      !LoadShaderModule("./res/shaders/compiled/basic_frag.spv",
                        global.context->Device, &triangle_frag_shader),
      "Failed to load fragment shader.");

  vk::PushConstantRange buffer_range;
  buffer_range.setOffset(0);
  buffer_range.setSize(sizeof(GPUDrawPushConstants));
  buffer_range.setStageFlags(vk::ShaderStageFlagBits::eVertex);

  vk::PipelineLayoutCreateInfo layout_info;
  layout_info.setPushConstantRangeCount(1);
  layout_info.setPPushConstantRanges(&buffer_range);

  VK_CHECK(global.context->Device.createPipelineLayout(
      &layout_info, nullptr, &m_GeometryPipelineLayout));

  PipelineBuilder pipelineBuilder;

  pipelineBuilder.pipeline_layout = m_GeometryPipelineLayout;
  pipelineBuilder.SetShaders(triangle_vertex_shader, triangle_frag_shader);
  pipelineBuilder.SetInputTopology(vk::PrimitiveTopology::eTriangleList);
  pipelineBuilder.SetPolygonMode(vk::PolygonMode::eFill);
  pipelineBuilder.SetCullMode(vk::CullModeFlagBits::eNone,
                              vk::FrontFace::eClockwise);
  pipelineBuilder.SetMultisamplingNone();
  pipelineBuilder.DisableBlending();
  pipelineBuilder.EnableDepthTest(true, vk::CompareOp::eGreaterOrEqual);
  pipelineBuilder.SetColorAttachmentFormat(m_DrawImage.Format);
  pipelineBuilder.SetDepthFormat(m_DepthImage.Format);

  // finally build the pipeline
  m_GeometryPipeline = pipelineBuilder.build(global.context->Device);

  global.context->Device.destroyShaderModule(triangle_frag_shader);
  global.context->Device.destroyShaderModule(triangle_vertex_shader);

  global.context->DeletionQueue.Push([&]() {
    global.context->Device.destroyPipelineLayout(m_GeometryPipelineLayout);
    global.context->Device.destroyPipeline(m_GeometryPipeline);
  });
}

void Renderer::DrawGeometry(vk::CommandBuffer cmd) {
  vk::RenderingAttachmentInfo color_attachment;
  color_attachment.setImageView(m_DrawImage.View);
  color_attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
  color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
  color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);

  vk::ClearDepthStencilValue depth_clear;
  depth_clear.setDepth(0.0f);

  vk::RenderingAttachmentInfo depth_attachment;
  depth_attachment.setImageView(m_DepthImage.View);
  depth_attachment.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal);
  depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
  depth_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
  depth_attachment.setClearValue(depth_clear);

  vk::Offset2D offset;
  offset.setX(0);
  offset.setY(0);

  vk::Rect2D scissor;
  scissor.setOffset(offset);
  scissor.setExtent({m_DrawImage.Extent.width, m_DrawImage.Extent.height});

  vk::RenderingInfo render_info;
  render_info.setRenderArea(scissor);
  render_info.setLayerCount(1);
  render_info.setViewMask(0);
  render_info.setColorAttachmentCount(1);
  render_info.setPColorAttachments(&color_attachment);
  render_info.setPDepthAttachment(&depth_attachment);

  cmd.beginRendering(&render_info);

  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GeometryPipeline);

  GPUDrawPushConstants push_constants{};
  push_constants.worldMatrix = global.camera->GetMVP();
  push_constants.vertexBuffer =
      m_Meshes[m_MeshIndex]->meshBuffers.vertexBufferAddress;

  cmd.pushConstants(m_GeometryPipelineLayout, vk::ShaderStageFlagBits::eVertex,
                    0, sizeof(GPUDrawPushConstants), &push_constants);
  cmd.bindIndexBuffer(m_Meshes[m_MeshIndex]->meshBuffers.indexBuffer.Buffer,
                      m_Meshes[m_MeshIndex]->surfaces[0].startIndex,
                      vk::IndexType::eUint32);

  vk::Viewport viewport;
  viewport.setX(0);
  viewport.setY(0);
  viewport.setWidth(m_DrawImage.Extent.width);
  viewport.setHeight(m_DrawImage.Extent.height);
  viewport.setMinDepth(0.0f);
  viewport.setMaxDepth(1.0f);

  cmd.setViewport(0, 1, &viewport);

  cmd.setScissor(0, 1, &scissor);

  cmd.drawIndexed(m_Meshes[m_MeshIndex]->surfaces[0].count, 1,
                  m_Meshes[m_MeshIndex]->surfaces[0].startIndex, 0, 0);

  cmd.endRendering();
}

void Renderer::AddGeometry(
    const std::vector<std::shared_ptr<MeshAsset>>& geometry) {
  for (const auto& mesh : geometry) m_Meshes.push_back(mesh);
}

void Renderer::SetMeshIndex(int index) { m_MeshIndex = index; }

void Renderer::Resize() {
  global.context->Device.waitIdle();

  global.swapchain->Shutdown();
  global.swapchain->Init();

  m_DrawImage.Destroy();
  m_DepthImage.Destroy();

  InitDrawTarget();
}

}  // namespace gfx
