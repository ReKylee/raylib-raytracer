#include "app/Application.hpp"
#include "app/CameraControls.hpp"
#include "sceneParser/SceneParser.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace raytracer::app {

Application::Application(int width, int height, std::string title)
    : m_width(width), m_height(height), m_title(std::move(title)) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(m_width, m_height, m_title.c_str());
  SetExitKey(KEY_F10);
  SetTargetFPS(60);

  m_renderer.emplace("assets/shaders/raytrace.fs");

  scanScenes();
  loadScene(0);

  DisableCursor();
}

Application::~Application() {
  m_renderer.reset();
  CloseWindow();
}

void Application::run() {
  while (!WindowShouldClose()) {
    update();
    render();
  }
}

void Application::scanScenes() {
  namespace fs = std::filesystem;

  const fs::path scenesDir{"Scenes"};
  if (!fs::is_directory(scenesDir)) {
    throw std::runtime_error("Application: 'Scenes' directory not found");
  }

  m_scenePaths.clear();
  for (const auto &entry : fs::directory_iterator{scenesDir}) {
    if (entry.is_regular_file() && entry.path().extension() == ".txt") {
      m_scenePaths.push_back(entry.path().string());
    }
  }

  std::sort(m_scenePaths.begin(), m_scenePaths.end());

  if (m_scenePaths.empty()) {
    throw std::runtime_error("Application: no .txt scene files in Scenes/");
  }
}

void Application::loadScene(std::size_t index) {
  m_currentScene = index % m_scenePaths.size();
  try {
    m_scene = sceneParser::parseScene(m_scenePaths[m_currentScene]);
    m_renderer->uploadScene(m_scene);
    camera::Initialize(m_scene.camera, m_cameraOrientation);
    m_loadError.clear();
  } catch (const std::exception &e) {
    // Keep the previously-loaded scene rendering; show the error in the UI.
    m_loadError = e.what();
  }
}

void Application::drawLoadError() const {
  if (m_loadError.empty()) {
    return;
  }

  namespace fs = std::filesystem;
  const std::string fileName =
      fs::path(m_scenePaths[m_currentScene]).filename().string();
  const std::string title = "Failed to load: " + fileName;

  constexpr int fontSize = 18;
  constexpr int lineHeight = fontSize + 8;
  constexpr int padding = 14;
  constexpr int margin = 14;

  const int textWidth = std::max(MeasureText(title.c_str(), fontSize),
                                 MeasureText(m_loadError.c_str(), fontSize));
  const int bannerWidth =
      std::min(textWidth + padding * 2, GetScreenWidth() - margin * 2);
  const int bannerHeight = padding * 2 + lineHeight * 2;
  const int bannerX = margin;
  const int bannerY = GetScreenHeight() - margin - bannerHeight;

  DrawRectangle(bannerX, bannerY, bannerWidth, bannerHeight,
                Color{60, 12, 12, 235});
  DrawRectangleLines(bannerX, bannerY, bannerWidth, bannerHeight,
                     Color{220, 80, 80, 230});
  DrawText(title.c_str(), bannerX + padding, bannerY + padding, fontSize,
           Color{255, 200, 200, 255});
  DrawText(m_loadError.c_str(), bannerX + padding,
           bannerY + padding + lineHeight, fontSize,
           Color{255, 235, 235, 255});
}

void Application::update() {
  m_width = GetScreenWidth();
  m_height = GetScreenHeight();

  // Space cycles to the next scene; loadScene wraps around the list.
  if (IsKeyPressed(KEY_SPACE)) {
    loadScene(m_currentScene + 1);
  }

  debug::Update(m_debugState);
  camera::UpdateFreeCamera(m_scene.camera, m_cameraOrientation);
  m_renderer->updateFrame(m_width, m_height, m_scene.camera,
                          debug::ToRenderOptions(m_debugState));
}

void Application::render() {
  BeginDrawing();
  ClearBackground(BLACK);

  m_renderer->render(m_width, m_height);

  DrawFPS(10, 10);
  debug::DrawOverlay(m_debugState);
  drawLoadError();

  EndDrawing();
}

} // namespace raytracer::app
