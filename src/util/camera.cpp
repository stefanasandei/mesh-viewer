//
// Created by Stefan on 12/27/2023.
//

#include "util/camera.hpp"

#include "global.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace util {

Camera::Camera() {
  projection = glm::mat4(1.0f);
  view = glm::mat4(1.0f);
  model = glm::mat4(1.0f);
}

Camera::~Camera() {}

void Camera::Update() {
  view = glm::translate(glm::mat4(1.0f), glm::vec3{0, 0, -4});

  projection = glm::perspective(
      glm::radians(45.f),
      (float)global.window->GetSize().x / (float)global.window->GetSize().y,
      0.1f, 10000.0f);
  //projection[1][1] *= -1.0f;

  model = glm::mat4(1.0f);
  model = glm::rotate(model, glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(angleZ), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 Camera::GetMVP() const { return projection * view * model; }

}
