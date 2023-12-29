//
// Created by Stefan on 12/26/2023.
//

#include "gfx/mesh.hpp"

#include "global.hpp"

#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

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
    vmaDestroyBuffer(allocator, staging.Buffer, staging.Allocation);
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

std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGLTFModel(
    VmaAllocator allocator, std::filesystem::path filePath) {
  std::cout << "Loading GLTF: " << filePath << std::endl;

  fastgltf::GltfDataBuffer data;
  data.loadFromFile(filePath);

  constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers |
                               fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};

  auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
  if (load) {
    gltf = std::move(load.get());
  } else {
    std::cerr << "Failed to load glTF: "
              << fastgltf::to_underlying(load.error()) << " \n";
    return {};
  }

  std::vector<std::shared_ptr<MeshAsset>> meshes;

  for(fastgltf::Image& img : gltf.images) {

  }

  for(fastgltf::Sampler sampler: gltf.samplers) {

  }

  for(fastgltf::Texture& tex : gltf.textures) {

  }

  for(fastgltf::Material& mat : gltf.materials) {

  }

  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;
  for (fastgltf::Mesh& mesh : gltf.meshes) {
    MeshAsset newmesh;

    newmesh.name = mesh.name;

    // clear the mesh arrays each mesh, we dont want to merge them by error
    indices.clear();
    vertices.clear();

    for (auto&& p : mesh.primitives) {
      GeoSurface newSurface{};
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count =
          (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initial_vtx = vertices.size();

      // load indexes
      {
        fastgltf::Accessor& indexaccessor =
            gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
            gltf, indexaccessor,
            [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
      }

      // load vertex positions
      {
        fastgltf::Accessor& posAccessor =
            gltf.accessors[p.findAttribute("POSITION")->second];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, posAccessor, [&](glm::vec3 v, size_t index) {
              Vertex newvtx;
              newvtx.position = v;
              newvtx.normal = {1, 0, 0};
              newvtx.color = glm::vec4{1.f};
              newvtx.uv_x = 0;
              newvtx.uv_y = 0;
              vertices[initial_vtx + index] = newvtx;
            });
      }

      // load vertex normals
      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            gltf, gltf.accessors[(*normals).second],
            [&](glm::vec3 v, size_t index) {
              vertices[initial_vtx + index].normal = v;
            });
      }

      // load UVs
      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            gltf, gltf.accessors[(*uv).second], [&](glm::vec2 v, size_t index) {
              vertices[initial_vtx + index].uv_x = v.x;
              vertices[initial_vtx + index].uv_y = v.y;
            });
      }

      // load vertex colors
      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            gltf, gltf.accessors[(*colors).second],
            [&](glm::vec4 v, size_t index) {
              vertices[initial_vtx + index].color = v;
            });
      }
      newmesh.surfaces.push_back(newSurface);
    }

    // display the vertex normals
    constexpr bool OverrideColors = true;
    if (OverrideColors) {
      for (Vertex& vtx : vertices) {
        vtx.color = glm::vec4(vtx.normal, 1.f);
      }
    }
    newmesh.meshBuffers = UploadMesh(allocator, indices, vertices);

    meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
  }

  return meshes;
}

}  // namespace gfx