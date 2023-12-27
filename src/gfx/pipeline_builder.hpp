//
// Created by Stefan on 12/26/2023.
//

#pragma once

#include "gfx/vulkan_utils.hpp"

namespace gfx {

class PipelineBuilder {
 public:
  PipelineBuilder();
  ~PipelineBuilder();

  void SetShaders(vk::ShaderModule vertex_shader,
                  vk::ShaderModule fragment_shader);
  void SetInputTopology(vk::PrimitiveTopology topology);
  void SetPolygonMode(vk::PolygonMode mode);
  void SetCullMode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);
  void SetMultisamplingNone();
  void DisableBlending();
  void SetColorAttachmentFormat(vk::Format format);
  void SetDepthFormat(vk::Format format);
  void DisableDepthTest();
  void EnableDepthTest(bool depth_write_enable, vk::CompareOp op);

  void clear();

  vk::Pipeline build(vk::Device device);

 public:
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  vk::PipelineRasterizationStateCreateInfo rasterizer;
  vk::PipelineColorBlendAttachmentState color_blend_attachment;
  vk::PipelineMultisampleStateCreateInfo multisampling;
  vk::PipelineLayout pipeline_layout;
  vk::PipelineDepthStencilStateCreateInfo depth_stencil;
  vk::PipelineRenderingCreateInfo render_info;
  vk::Format color_attachment_format;
};

}  // namespace gfx
