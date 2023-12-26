#include "global.hpp"

int main() {
  init_globals();

  while (!global.window->ShouldClose()) {
    global.window->PollEvents();

    global.renderer->Draw();
  }

  return 0;
}
