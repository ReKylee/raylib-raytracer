#include "app/Application.hpp"

#include <cmath>
#include <utility>

namespace raytracer::app {

using scene::DirectionalLight;
using scene::Plane;
using scene::Scene;
using scene::Sphere;
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
              .position = {0.0f, 0.8f, 5.0f},
              .forward = {0.0f, -0.08f, -1.0f},
              .up = {0.0f, 1.0f, 0.0f},
              .fovY = 55.0f,
          },

      .ambient = {.intensity = {0.06f, 0.06f, 0.075f}},

      .spheres =
          {
              // Main center sphere
              Sphere{
                  .position = {0.0f, 0.0f, 0.0f},
                  .radius = 1.0f,
                  .color = {1.0f, 0.22f, 0.18f},
                  .shininess = 48.0f,
              },

              // Left small cool sphere
              Sphere{
                  .position = {-1.65f, -0.35f, -0.85f},
                  .radius = 0.65f,
                  .color = {0.12f, 0.55f, 1.0f},
                  .shininess = 96.0f,
              },

              // Right green sphere
              Sphere{
                  .position = {1.55f, -0.25f, -1.1f},
                  .radius = 0.75f,
                  .color = {0.25f, 1.0f, 0.38f},
                  .shininess = 32.0f,
              },

              // Small yellow sphere in front, good for testing depth sorting
              Sphere{
                  .position = {-0.55f, -0.72f, 1.35f},
                  .radius = 0.28f,
                  .color = {1.0f, 0.82f, 0.18f},
                  .shininess = 16.0f,
              },

              // Small purple sphere farther back
              Sphere{
                  .position = {0.85f, 0.55f, -1.75f},
                  .radius = 0.38f,
                  .color = {0.75f, 0.35f, 1.0f},
                  .shininess = 128.0f,
              },
          },

      .planes =
          {
              // Ground plane.
              Plane{
                  .normal = {0.0f, -0.5f, -1.0f},
                  .offset = -3.5f,
                  .color = {0.72f, 0.72f, 0.68f},
                  .shininess = 12.0f,
              },
          },

      .directionalLights =
          {
              // Warm sunlight from upper-left/front
              DirectionalLight{
                  .direction = {-0.45f, -0.9f, -0.35f},
                  .intensity = {0.75f, 0.68f, 0.55f},
              },
          },

      .spotlights =
          {
              // Cool-ish spotlight from camera/right side
              Spotlight{
                  .position = {2.8f, 3.5f, 3.2f},
                  .direction = {-0.55f, -0.75f, -0.55f},
                  .intensity = {2.8f, 3.1f, 3.8f},
                  .cosineCutoff =
                      static_cast<float>(std::cos(22.0f * DEG2RAD)),
              },

              // Soft warm fill from left/back
              Spotlight{
                  .position = {-3.0f, 2.2f, 1.0f},
                  .direction = {0.7f, -0.45f, -0.25f},
                  .intensity = {1.4f, 0.95f, 0.65f},
                  .cosineCutoff =
                      static_cast<float>(std::cos(35.0f * DEG2RAD)),
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
