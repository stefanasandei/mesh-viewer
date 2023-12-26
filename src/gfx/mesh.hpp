//
// Created by Stefan on 12/26/2023.
//

#pragma once

#include "gfx/vulkan_utils.hpp"
#include "gfx/allocated_buffer.hpp"

namespace gfx {

struct Vertex {
  glm::vec3 position;
  float uv_x;
  glm::vec3 normal;
  float uv_y;
  glm::vec4 color;
};

struct GPUMeshBuffers {
  AllocatedBuffer indexBuffer;
  AllocatedBuffer vertexBuffer;
  VkDeviceAddress vertexBufferAddress;
};

struct GPUDrawPushConstants {
  glm::mat4 worldMatrix;
  VkDeviceAddress vertexBuffer;
};

GPUMeshBuffers UploadMesh(VmaAllocator allocator, std::span<uint32_t> indices, std::span<Vertex> vertices);

}