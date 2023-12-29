//
// Created by Stefan on 12/27/2023.
//

#pragma once

#include <glm/glm.hpp>

namespace util {

class Camera {
 public:
  Camera();
  ~Camera();

  void Update();

  [[nodiscard]] glm::mat4 GetMVP() const;

 public:
  float angleX = 270.0f, angleY = 0.0f, angleZ = 0.0f;
  glm::mat4 model, view, projection;
};

}