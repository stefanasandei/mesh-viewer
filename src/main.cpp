#include "global.hpp"

int main() {
  init_globals();

  int index = 1;

  auto meshes = gfx::LoadGLTFModel(global.renderer->GetAllocator(),
                                   "./res/models/smooth_monke.glb");

  global.renderer->AddGeometry(meshes.value());

  while (!global.window->ShouldClose()) {
    global.window->PollEvents();

    global.imgui->AddPanel([&]() {
      ImGui::Begin("Control Panel");
      ImGui::SliderInt("Model", &index, 0, 2);
      ImGui::End();
    });

    global.renderer->SetMeshIndex(index);

    global.imgui->Draw();
    global.renderer->Draw();
  }

  return 0;
}
