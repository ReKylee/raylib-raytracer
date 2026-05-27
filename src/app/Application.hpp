#pragma once

#include "app/DebugControls.hpp"
#include "render/RaytraceRenderer.hpp"
#include "scene/scene.hpp"

#include <optional>
#include <string>

namespace raytracer::app {

/// Owns the window, demo scene, input updates, and render loop.
class Application {
public:
  /// Creates the application window and initializes the renderer.
  Application(int width, int height, std::string title);

  /// Releases renderer resources before closing the window.
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  /// Runs the main update/render loop until the window is closed.
  void run();

private:
  /// Builds the hard-coded validation scene used by the assignment.
  void createTestScene();

  /// Polls input and uploads per-frame render state.
  void update();

  /// Draws a full-screen shader pass and debug overlay.
  void render();

private:
  int m_width = 0;
  int m_height = 0;
  std::string m_title;

  std::optional<render::RaytraceRenderer> m_renderer;
  scene::Scene m_scene{};
  Vector4 m_cameraOrientation{};
  debug::State m_debugState{};
};

} // namespace raytracer::app
