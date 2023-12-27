//
// Created by Stefan on 12/26/2023.
//

#include "gfx/pipeline_builder.hpp"

#include "global.hpp"

namespace gfx {

PipelineBuilder::PipelineBuilder() { clear(); }

PipelineBuilder::~PipelineBuilder() {}

void PipelineBuilder::clear() { shader_stages.clear(); }

vk::Pipeline PipelineBuilder::build(vk::Device device) {
  vk::PipelineViewportStateCreateInfo viewport_state;
  viewport_state.setViewportCount(1);
  viewport_state.setScissorCount(1);

  vk::PipelineColorBlendStateCreateInfo color_blending;
  color_blending.setLogicOpEnable(VK_FALSE);
  color_blending.setLogicOp(vk::LogicOp::eCopy);
  color_blending.setAttachmentCount(1);
  color_blending.setPAttachments(&color_blend_attachment);

  vk::PipelineVertexInputStateCreateInfo vertex_input_info;

  vk::GraphicsPipelineCreateInfo pipeline_info;
  pipeline_info.setPNext(&render_info);
  pipeline_info.setStageCount(shader_stages.size());
  pipeline_info.setPStages(shader_stages.data());
  pipeline_info.setPVertexInputState(&vertex_input_info);
  pipeline_info.setPInputAssemblyState(&input_assembly);
  pipeline_info.setPViewportState(&viewport_state);
  pipeline_info.setPRasterizationState(&rasterizer);
  pipeline_info.setPMultisampleState(&multisampling);
  pipeline_info.setPColorBlendState(&color_blending);
  pipeline_info.setPDepthStencilState(&depth_stencil);
  pipeline_info.setLayout(pipeline_layout);

  vk::DynamicState state[] = {vk::DynamicState::eViewport,
                              vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamic_info;
  dynamic_info.setDynamicStateCount(2);
  dynamic_info.setPDynamicStates(&state[0]);

  pipeline_info.setPDynamicState(&dynamic_info);

  vk::Pipeline pipeline;
  VK_CHECK(device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info,
                                          nullptr, &pipeline));

  return pipeline;
}
void PipelineBuilder::SetShaders(vk::ShaderModule vertex_shader,
                                 vk::ShaderModule fragment_shader) {
  shader_stages.clear();

  vk::PipelineShaderStageCreateInfo vertex_stage_info;
  vertex_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
  vertex_stage_info.setModule(vertex_shader);
  vertex_stage_info.setPName("main");

  shader_stages.push_back(vertex_stage_info);

  vk::PipelineShaderStageCreateInfo fragment_stage_info;
  fragment_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
  fragment_stage_info.setModule(fragment_shader);
  fragment_stage_info.setPName("main");

  shader_stages.push_back(fragment_stage_info);
}

void PipelineBuilder::SetInputTopology(vk::PrimitiveTopology topology) {
  input_assembly.setTopology(topology);
  input_assembly.setPrimitiveRestartEnable(VK_FALSE);
}

void PipelineBuilder::SetPolygonMode(vk::PolygonMode mode) {
  rasterizer.setPolygonMode(mode);
  rasterizer.setLineWidth(1.0f);
}

void PipelineBuilder::SetCullMode(vk::CullModeFlags cull_mode,
                                  vk::FrontFace front_face) {
  rasterizer.setCullMode(cull_mode);
  rasterizer.setFrontFace(front_face);
}

void PipelineBuilder::SetMultisamplingNone() {
  multisampling.setSampleShadingEnable(VK_FALSE);
  multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);
  multisampling.setMinSampleShading(1.0f);
  multisampling.setPSampleMask(nullptr);
  multisampling.setAlphaToCoverageEnable(VK_FALSE);
  multisampling.setAlphaToOneEnable(VK_FALSE);
}

void PipelineBuilder::DisableBlending() {
  color_blend_attachment.setColorWriteMask(
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  color_blend_attachment.setBlendEnable(VK_FALSE);
}

void PipelineBuilder::SetColorAttachmentFormat(vk::Format format) {
  color_attachment_format = format;
  render_info.setColorAttachmentCount(1);
  render_info.setPColorAttachmentFormats(&color_attachment_format);
}

void PipelineBuilder::SetDepthFormat(vk::Format format) {
  render_info.setDepthAttachmentFormat(format);
}

void PipelineBuilder::DisableDepthTest() {
  depth_stencil.setDepthTestEnable(VK_FALSE);
  depth_stencil.setDepthWriteEnable(VK_FALSE);
  depth_stencil.setDepthCompareOp(vk::CompareOp::eNever);
  depth_stencil.setDepthBoundsTestEnable(VK_FALSE);
  depth_stencil.setStencilTestEnable(VK_FALSE);
  depth_stencil.setFront({});
  depth_stencil.setBack({});
  depth_stencil.setMinDepthBounds(0.0f);
  depth_stencil.setMaxDepthBounds(1.0f);
}

void PipelineBuilder::EnableDepthTest(bool depth_write_enable,
                                      vk::CompareOp op) {
  depth_stencil.setDepthTestEnable(VK_TRUE);
  depth_stencil.setDepthWriteEnable(depth_write_enable);
  depth_stencil.setDepthCompareOp(op);
  depth_stencil.setDepthBoundsTestEnable(VK_FALSE);
  depth_stencil.setStencilTestEnable(VK_FALSE);
  depth_stencil.setFront({});
  depth_stencil.setBack({});
  depth_stencil.setMinDepthBounds(0.0f);
  depth_stencil.setMaxDepthBounds(1.0f);
}

}  // namespace gfx
