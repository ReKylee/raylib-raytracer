#include "app/Application.hpp"
#include "app/CameraControls.hpp"

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
  SetExitKey(KEY_F12);
  SetTargetFPS(60);

  m_renderer.emplace("assets/shaders/raytrace.fs");
  createTestScene();
  DisableCursor();

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
              .position = {0.0f, 0.75f, 8.4f},
              .forward = {0.0f, -0.1f, -1.0f},
              .up = {0.0f, 1.0f, 0.0f},
              .fovY = 50.0f,
          },

      .ambient = {.intensity = {0.035f, 0.04f, 0.055f}},

      .spheres =
          {
              // Center: glossy reflections, strong highlights, and
              // self-shadowing.
              Sphere{
                  .position = {0.0f, 0.0f, 0.0f},
                  .radius = 1.0f,
                  .color = {1.0f, 0.1f, 0.07f},
                  .shininess = 96.0f,
              },

              // Left bay: hard directional shadows and saturated diffuse color.
              Sphere{
                  .position = {-3.2f, -0.35f, -0.55f},
                  .radius = 0.65f,
                  .color = {0.05f, 0.52f, 1.0f},
                  .shininess = 72.0f,
              },

              // Right bay: warm spotlight and clear ground shadow.
              Sphere{
                  .position = {3.25f, -0.28f, -0.75f},
                  .radius = 0.72f,
                  .color = {0.18f, 1.0f, 0.28f},
                  .shininess = 28.0f,
              },

              // Foreground: tiny geometry for antialiasing and depth ordering.
              Sphere{
                  .position = {-0.55f, -0.73f, 2.25f},
                  .radius = 0.28f,
                  .color = {1.0f, 0.86f, 0.05f},
                  .shininess = 16.0f,
              },

              // Back center: visible in reflections and second directional
              // light.
              Sphere{
                  .position = {0.9f, 0.68f, -2.35f},
                  .radius = 0.42f,
                  .color = {0.78f, 0.22f, 1.0f},
                  .shininess = 160.0f,
              },

              // Left rear: bright specular reference.
              Sphere{
                  .position = {-2.25f, -0.72f, -2.15f},
                  .radius = 0.3f,
                  .color = {0.95f, 0.95f, 0.9f},
                  .shininess = 220.0f,
              },

              // Right foreground: overlap and reflection test.
              Sphere{
                  .position = {1.85f, -0.62f, 1.45f},
                  .radius = 0.38f,
                  .color = {1.0f, 0.38f, 0.88f},
                  .shininess = 120.0f,
              },

              // Far wall marker: makes spotlight cone edges easier to see.
              Sphere{
                  .position = {-3.65f, 0.18f, -3.25f},
                  .radius = 0.55f,
                  .color = {0.12f, 1.0f, 0.82f},
                  .shininess = 48.0f,
              },

              // Directional test: neutral caster for warm left-to-right
              // shadows.
              Sphere{
                  .position = {-4.1f, -0.55f, 1.25f},
                  .radius = 0.45f,
                  .color = {0.86f, 0.86f, 0.82f},
                  .shininess = 36.0f,
              },

              // Directional test: neutral caster for cool right-to-left
              // shadows.
              Sphere{
                  .position = {4.15f, -0.55f, 0.95f},
                  .radius = 0.45f,
                  .color = {0.82f, 0.86f, 0.9f},
                  .shininess = 36.0f,
              },
          },

      .planes =
          {
              // Floor: non-unit normal tests plane normalization and checker
              // UVs.
              Plane{
                  .normal = {0.0f, 2.0f, 0.0f},
                  .offset = 2.0f,
                  .color = {0.8f, 0.8f, 0.74f},
                  .shininess = 12.0f,
              },

              // Back wall catches both spotlight cones and reflected objects.
              Plane{
                  .normal = {0.0f, 0.0f, 1.0f},
                  .offset = 4.2f,
                  .color = {0.55f, 0.6f, 0.7f},
                  .shininess = 18.0f,
              },

              // Side walls make the gallery read as a small room.
              Plane{
                  .normal = {1.0f, 0.0f, 0.0f},
                  .offset = 5.2f,
                  .color = {0.55f, 0.58f, 0.64f},
                  .shininess = 10.0f,
              },

              Plane{
                  .normal = {-1.0f, 0.0f, 0.0f},
                  .offset = 5.2f,
                  .color = {0.64f, 0.58f, 0.55f},
                  .shininess = 10.0f,
              },

              // Ceiling gives upward views and reflections a bounded surface.
              Plane{
                  .normal = {0.0f, -1.0f, 0.0f},
                  .offset = 3.2f,
                  .color = {0.46f, 0.48f, 0.52f},
                  .shininess = 8.0f,
              },
          },

      .directionalLights =
          {
              // Warm key direction: hard colored shadows from the left/front.
              DirectionalLight{
                  .direction = {0.62f, -0.72f, -0.38f},
                  .intensity = {0.85f, 0.52f, 0.28f},
              },

              // Cool key direction: second directional light from the
              // right/front.
              DirectionalLight{
                  .direction = {-0.58f, -0.58f, -0.58f},
                  .intensity = {0.28f, 0.42f, 0.78f},
              },
          },

      .spotlights =
          {
              // Cool narrow cone: visible soft edge on the left floor/wall.
              Spotlight{
                  .position = {-3.8f, 2.7f, 2.5f},
                  .direction = {0.25f, -0.68f, -0.7f},
                  .intensity = {3.0f, 4.1f, 6.6f},
                  .cosineCutoff = static_cast<float>(std::cos(20.0f * DEG2RAD)),
              },

              // Warm wider cone: overlaps the center and right bay.
              Spotlight{
                  .position = {3.8f, 2.35f, 2.1f},
                  .direction = {-0.62f, -0.55f, -0.56f},
                  .intensity = {4.8f, 2.2f, 0.85f},
                  .cosineCutoff = static_cast<float>(std::cos(32.0f * DEG2RAD)),
              },
          },
  };

  camera::Initialize(m_scene.camera, m_cameraOrientation);
}

void Application::update() {
  m_width = GetScreenWidth();
  m_height = GetScreenHeight();

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

  EndDrawing();
}

} // namespace raytracer::app
