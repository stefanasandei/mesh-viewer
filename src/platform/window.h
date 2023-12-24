//
// Created by Stefan on 12/24/2023.
//

#pragma once

#include "util/util.hpp"

namespace platform {

    class Window {
    public:
        Window(glm::ivec2 size, const std::string& title);
        ~Window();

        void PollEvents() const;
        bool ShouldClose() const;

        GLFWwindow* GetNative();
        glm::ivec2 GetSize();

    private:
        GLFWwindow* m_Window;
    };

}
