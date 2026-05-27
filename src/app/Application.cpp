#include "app/Application.hpp"
#include "app/CameraControls.hpp"

#include <filesystem>
#include <utility>

namespace raytracer::app {

namespace {

constexpr int TARGET_FPS = 60;
constexpr const char *SHADER_PATH = "assets/shaders/raytrace.fs";
constexpr const char *SCENES_DIRECTORY_NAME = "Scenes";

std::filesystem::path RuntimeScenesDirectory() {
  const std::filesystem::path applicationDirectory{GetApplicationDirectory()};
  const std::filesystem::path nextToExecutable =
      applicationDirectory / SCENES_DIRECTORY_NAME;

  if (std::filesystem::is_directory(nextToExecutable)) {
    return nextToExecutable;
  }

  return SCENES_DIRECTORY_NAME;
}

} // namespace

Application::Application(int width, int height, std::string title)
    : m_width(width), m_height(height), m_title(std::move(title)) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(m_width, m_height, m_title.c_str());
  SetExitKey(KEY_F10);
  SetTargetFPS(TARGET_FPS);

  m_renderer.emplace(SHADER_PATH);
  m_sceneLibrary.emplace(RuntimeScenesDirectory());
  DisableCursor();

  m_renderer->uploadScene(m_sceneLibrary->scene());
  camera::Initialize(m_sceneLibrary->scene().camera, m_cameraOrientation);
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

void Application::update() {
  m_width = GetScreenWidth();
  m_height = GetScreenHeight();

  if (IsKeyPressed(KEY_SPACE) && m_sceneLibrary->loadNext()) {
    m_renderer->uploadScene(m_sceneLibrary->scene());
    camera::Initialize(m_sceneLibrary->scene().camera, m_cameraOrientation);
  }

  debug::Update(m_debugState);
  camera::UpdateFreeCamera(m_sceneLibrary->scene().camera, m_cameraOrientation);
  m_renderer->updateFrame(m_width, m_height, m_sceneLibrary->scene().camera,
                          debug::ToRenderOptions(m_debugState));
}

void Application::render() {
  BeginDrawing();
  ClearBackground(BLACK);

  m_renderer->render(m_width, m_height);

  debug::DrawOverlay(m_debugState, m_sceneLibrary->currentSceneName());
  debug::DrawSceneLoadError(m_sceneLibrary->currentSceneName(),
                            m_sceneLibrary->loadError());

  EndDrawing();
}

} // namespace raytracer::app
