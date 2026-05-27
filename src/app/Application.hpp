#pragma once

#include "app/DebugControls.hpp"
#include "render/RaytraceRenderer.hpp"
#include "scene/scene.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace raytracer::app {

class Application {
public:
  Application(int width, int height, std::string title);
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  void run();

private:
  void scanScenes();
  void loadScene(std::size_t index);
  void drawLoadError() const;

  void update();
  void render();

private:
  int m_width = 0;
  int m_height = 0;
  std::string m_title;

  std::optional<render::RaytraceRenderer> m_renderer;
  scene::Scene m_scene{};
  Vector4 m_cameraOrientation{};
  debug::State m_debugState{};

  std::vector<std::string> m_scenePaths;
  std::size_t m_currentScene = 0;
  // Non-empty when the last loadScene attempt failed; rendered as a banner.
  std::string m_loadError;
};

} // namespace raytracer::app
