#pragma once

#include "render/RaytraceRenderer.hpp"
#include "scene/scene.hpp"

#include <optional>
#include <string>

namespace raytracer::app {

class Application {
public:
  Application(int width, int height, std::string title);
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  void run();

private:
  void createTestScene();

  void update();
  void render();

private:
  int m_width = 0;
  int m_height = 0;
  std::string m_title;

  std::optional<render::RaytraceRenderer> m_renderer;
  scene::Scene m_scene{};
};

} // namespace raytracer::app
