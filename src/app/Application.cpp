#include "app/Application.hpp"

#include <cmath>
#include <utility>

namespace raytracer::app {

using scene::Plane;
using scene::Scene;
using scene::Sphere;
using scene::DirectionalLight;
using scene::Spotlight;

Application::Application(int width, int height, std::string title)
    : m_width(width), m_height(height), m_title(std::move(title)) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(m_width, m_height, m_title.c_str());
  SetTargetFPS(60);

  m_renderer.emplace("assets/shaders/raytrace.fs");
  createTestScene();

  // Static scene data only needs to be uploaded once, unless the scene changes.
  m_renderer->uploadScene(m_scene);
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

void Application::createTestScene() {
  m_scene = Scene{
      .camera =
          {
              .position = {0.0f, 0.5f, 4.0f},
              .forward = {0.0f, 0.0f, -1.0f},
              .right = {1.0f, 0.0f, 0.0f},
              .up = {0.0f, 1.0f, 0.0f},
              .fovY = 60.0f,
          },
      .ambient = {.intensity = {0.08f, 0.08f, 0.1f}},
      .spheres =
          {
              Sphere{
                  .position = {0.0f, 0.0f, 0.0f},
                  .radius = 1.0f,
                  .color = {1.0f, 0.2f, 0.2f},
                  .shininess = 32.0f,
              },
              Sphere{
                  .position = {-1.5f, -0.2f, -1.0f},
                  .radius = 0.6f,
                  .color = {0.2f, 0.8f, 1.0f},
                  .shininess = 64.0f,
              },
              Sphere{
                  .position = {1.5f, -0.3f, -1.2f},
                  .radius = 0.7f,
                  .color = {0.3f, 1.0f, 0.4f},
                  .shininess = 24.0f,
              },
          },
      .planes =
          {
              Plane{
                  .normal = {0.0f, 1.0f, 0.0f},
                  .offset = 1.0f,
                  .color = {0.7f, 0.7f, 0.7f},
                  .shininess = 8.0f,
              },
          },
      .dirlights =
          {
              DirectionalLight{
                  .direction = {-0.5f, -1.0f, -0.4f},
                  .intensity = {0.7f, 0.65f, 0.55f},
              },
          },
      .spotlights =
          {
              Spotlight{
                  .position = {2.0f, 3.0f, 3.0f},
                  .direction = {-0.45f, -0.75f, -0.5f},
                  .intensity = {3.0f, 2.8f, 2.4f},
                  .cosine_cutoff =
                      static_cast<float>(std::cos(25.0f * DEG2RAD)),
              },
          },
  };
}

void Application::update() {
  m_width = GetScreenWidth();
  m_height = GetScreenHeight();

  m_renderer->updateFrame(m_width, m_height, m_scene.camera);
}

void Application::render() {
  BeginDrawing();
  ClearBackground(BLACK);

  m_renderer->render(m_width, m_height);

  DrawFPS(10, 10);

  EndDrawing();
}

} // namespace raytracer::app
