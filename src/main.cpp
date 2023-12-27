#include "global.hpp"

int main() {
  init_globals();

  int index = 0;

  auto meshes = gfx::LoadGLTFModel(global.renderer->GetAllocator(),
                                   "./res/models/DamagedHelmet.glb");

  global.renderer->AddGeometry(meshes.value());

  while (!global.window->ShouldClose()) {
    global.window->PollEvents();

    global.camera->Update();

    global.imgui->AddPanel([&]() {
      ImGui::Begin("Control Panel");
      ImGui::SliderInt("Model", &index, 0, meshes.value().size() - 1);

      ImGui::SliderFloat("X", &global.camera->angleX, 0.0f, 360.0f);
      ImGui::SliderFloat("Y", &global.camera->angleY, 0.0f, 360.0f);
      ImGui::SliderFloat("Z", &global.camera->angleZ, 0.0f, 360.0f);

      ImGui::End();
    });

    global.renderer->SetMeshIndex(index);

    global.imgui->Draw();
    global.renderer->Draw();
  }

  return 0;
}
