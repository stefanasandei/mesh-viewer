//
// Created by Stefan on 12/26/2023.
//

#include "gfx/mesh.hpp"

#include "global.hpp"

namespace gfx {

GPUMeshBuffers UploadMesh(VmaAllocator allocator, std::span<uint32_t> indices,
                          std::span<Vertex> vertices) {
  const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
  const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

  GPUMeshBuffers new_mesh;

  new_mesh.vertexBuffer =
      CreateBuffer(allocator, vertex_buffer_size,
                   vk::BufferUsageFlagBits::eStorageBuffer |
                       vk::BufferUsageFlagBits::eTransferDst |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress,
                   VMA_MEMORY_USAGE_GPU_ONLY);

  vk::BufferDeviceAddressInfo info;
  info.buffer = new_mesh.vertexBuffer.Buffer;
  new_mesh.vertexBufferAddress = global.context->Device.getBufferAddress(&info);

  new_mesh.indexBuffer = CreateBuffer(allocator, index_buffer_size,
                                      vk::BufferUsageFlagBits::eIndexBuffer |
                                          vk::BufferUsageFlagBits::eTransferDst,
                                      VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging = CreateBuffer(
      allocator, vertex_buffer_size + index_buffer_size,
      vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

  void* data;
  vmaMapMemory(allocator, staging.Allocation, &data);

  memcpy(data, vertices.data(), vertex_buffer_size);
  memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);

  global.context->DeletionQueue.Push([=]() {
    vmaDestroyBuffer(allocator, new_mesh.vertexBuffer.Buffer,
                     new_mesh.vertexBuffer.Allocation);
    vmaDestroyBuffer(allocator, new_mesh.indexBuffer.Buffer,
                     new_mesh.indexBuffer.Allocation);
    vmaDestroyBuffer(allocator, staging.Buffer,
                     staging.Allocation);
  });

  global.renderer->ImmediateSubmit([&](VkCommandBuffer cmd) {
    VkBufferCopy vertexCopy{0};
    vertexCopy.dstOffset = 0;
    vertexCopy.srcOffset = 0;
    vertexCopy.size = vertex_buffer_size;

    vkCmdCopyBuffer(cmd, staging.Buffer, new_mesh.vertexBuffer.Buffer, 1,
                    &vertexCopy);

    VkBufferCopy indexCopy{0};
    indexCopy.dstOffset = 0;
    indexCopy.srcOffset = vertex_buffer_size;
    indexCopy.size = index_buffer_size;

    vkCmdCopyBuffer(cmd, staging.Buffer, new_mesh.indexBuffer.Buffer, 1,
                    &indexCopy);

    vmaUnmapMemory(allocator, staging.Allocation);
  });

  return new_mesh;
}

}  // namespace gfx