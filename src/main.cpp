#include "platform/window.h"

int main() {
    platform::Window window({1600, 900}, "Mesh Viewer");

    while(!window.ShouldClose()) {
        window.PollEvents();
    }

    return 0;
}
