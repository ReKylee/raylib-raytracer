#include "app/Application.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

Application::Application(int width, int height, std::string title)
    : m_width(width), m_height(height), m_title(std::move(title)) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(m_width, m_height, m_title.c_str());
  SetTargetFPS(60);

  m_shader = LoadShader(nullptr, "assets/shaders/raytrace.fs");

  loadShaderLocations();
  createTestScene();
}

Application::~Application() {
  UnloadShader(m_shader);
  CloseWindow();
}

void Application::run() {
  while (!WindowShouldClose()) {
    update();
    render();
  }
}

void Application::loadShaderLocations() {
  m_locs.resolution = GetShaderLocation(m_shader, "iResolution");
  m_locs.time = GetShaderLocation(m_shader, "iTime");

  m_locs.cameraPosition = GetShaderLocation(m_shader, "uCameraPosition");
  m_locs.cameraForward = GetShaderLocation(m_shader, "uCameraForward");
  m_locs.cameraRight = GetShaderLocation(m_shader, "uCameraRight");
  m_locs.cameraUp = GetShaderLocation(m_shader, "uCameraUp");
  m_locs.cameraFovY = GetShaderLocation(m_shader, "uCameraFovY");

  m_locs.sphereCount = GetShaderLocation(m_shader, "uSphereCount");
  m_locs.sphereData = GetShaderLocation(m_shader, "uSphereData");
  m_locs.sphereColor = GetShaderLocation(m_shader, "uSphereColor");

  m_locs.planeCount = GetShaderLocation(m_shader, "uPlaneCount");
  m_locs.planeData = GetShaderLocation(m_shader, "uPlaneData");
  m_locs.planeColor = GetShaderLocation(m_shader, "uPlaneColor");

  m_locs.ambientIntensity = GetShaderLocation(m_shader, "uAmbientIntensity");

  m_locs.dirLightCount = GetShaderLocation(m_shader, "uDirLightCount");
  m_locs.dirLightDirection = GetShaderLocation(m_shader, "uDirLightDirection");
  m_locs.dirLightIntensity = GetShaderLocation(m_shader, "uDirLightIntensity");

  m_locs.spotlightCount = GetShaderLocation(m_shader, "uSpotlightCount");
  m_locs.spotlightPosition = GetShaderLocation(m_shader, "uSpotlightPosition");
  m_locs.spotlightCosineCutoff =
      GetShaderLocation(m_shader, "uSpotlightCosineCutoff");
  m_locs.spotlightDirection =
      GetShaderLocation(m_shader, "uSpotlightDirection");
  m_locs.spotlightIntensity =
      GetShaderLocation(m_shader, "uSpotlightIntensity");
}

void Application::createTestScene() {
  m_scene = Scene{};

  m_scene.camera.position = {0.0f, 0.5f, 4.0f};
  m_scene.camera.forward = {0.0f, 0.0f, -1.0f};
  m_scene.camera.right = {1.0f, 0.0f, 0.0f};
  m_scene.camera.up = {0.0f, 1.0f, 0.0f};
  m_scene.camera.fovY = 60.0f;

  m_scene.ambientlight.intensity = {0.08f, 0.08f, 0.1f};

  Plane ground;
  ground.normal = {0.0f, 1.0f, 0.0f};
  ground.offset = 1.0f;
  ground.color = {0.7f, 0.7f, 0.7f};
  ground.shininess = 8.0f;
  m_scene.planes.push_back(ground);

  Sphere sphere;
  sphere.position = {0.0f, 0.0f, 0.0f};
  sphere.radius = 1.0f;
  sphere.color = {1.0f, 0.2f, 0.2f};
  sphere.shininess = 32.0f;
  m_scene.spheres.push_back(sphere);

  sphere.position = {-1.5f, -0.2f, -1.0f};
  sphere.radius = 0.6f;
  sphere.color = {0.2f, 0.8f, 1.0f};
  sphere.shininess = 64.0f;
  m_scene.spheres.push_back(sphere);

  sphere.position = {1.5f, -0.3f, -1.2f};
  sphere.radius = 0.7f;
  sphere.color = {0.3f, 1.0f, 0.4f};
  sphere.shininess = 24.0f;
  m_scene.spheres.push_back(sphere);

  DirectionalLight sun;
  sun.direction = {-0.5f, -1.0f, -0.4f};
  sun.intensity = {0.7f, 0.65f, 0.55f};
  m_scene.dirlights.push_back(sun);

  Spotlight spot;
  spot.position = {2.0f, 3.0f, 3.0f};
  spot.direction = {-0.45f, -0.75f, -0.5f};
  spot.intensity = {3.0f, 2.8f, 2.4f};
  spot.cosine_cutoff = std::cos(25.0f * DEG2RAD);
  m_scene.spotlights.push_back(spot);
}
void Application::update() {
  m_width = GetScreenWidth();
  m_height = GetScreenHeight();

  const float time = static_cast<float>(GetTime());
  const float resolution[2] = {static_cast<float>(m_width),
                               static_cast<float>(m_height)};

  SetShaderValue(m_shader, m_locs.resolution, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(m_shader, m_locs.time, &time, SHADER_UNIFORM_FLOAT);

  uploadSceneToShader();
}

void Application::uploadSceneToShader() {
  const CameraData &camera = m_scene.camera;

  SetShaderValue(m_shader, m_locs.cameraPosition, &camera.position,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(m_shader, m_locs.cameraForward, &camera.forward,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(m_shader, m_locs.cameraRight, &camera.right,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(m_shader, m_locs.cameraUp, &camera.up, SHADER_UNIFORM_VEC3);
  SetShaderValue(m_shader, m_locs.cameraFovY, &camera.fovY,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(m_shader, m_locs.ambientIntensity,
                 &m_scene.ambientlight.intensity, SHADER_UNIFORM_VEC3);

  const size_t sphereCount =
      std::min(m_scene.spheres.size(), static_cast<size_t>(MAX_SPHERES));

  const size_t planeCount =
      std::min(m_scene.planes.size(), static_cast<size_t>(MAX_PLANES));

  const size_t dirLightCount =
      std::min(m_scene.dirlights.size(), static_cast<size_t>(MAX_LIGHTS));

  const size_t spotlightCount =
      std::min(m_scene.spotlights.size(), static_cast<size_t>(MAX_LIGHTS));

  const int sphereCountGpu = static_cast<int>(sphereCount);
  const int planeCountGpu = static_cast<int>(planeCount);
  const int dirLightCountGpu = static_cast<int>(dirLightCount);
  const int spotlightCountGpu = static_cast<int>(spotlightCount);

  SetShaderValue(m_shader, m_locs.sphereCount, &sphereCountGpu,
                 SHADER_UNIFORM_INT);
  SetShaderValue(m_shader, m_locs.planeCount, &planeCountGpu,
                 SHADER_UNIFORM_INT);
  SetShaderValue(m_shader, m_locs.dirLightCount, &dirLightCountGpu,
                 SHADER_UNIFORM_INT);
  SetShaderValue(m_shader, m_locs.spotlightCount, &spotlightCountGpu,
                 SHADER_UNIFORM_INT);

  std::array<Vector4, MAX_SPHERES> sphereData{};
  std::array<Vector4, MAX_SPHERES> sphereColor{};

  for (size_t i = 0; i < sphereCount; ++i) {
    const Sphere &sphere = m_scene.spheres[i];

    sphereData[i] = {sphere.position.x, sphere.position.y, sphere.position.z,
                     sphere.radius};

    sphereColor[i] = {sphere.color.x, sphere.color.y, sphere.color.z,
                      sphere.shininess};
  }

  if (sphereCountGpu > 0) {
    SetShaderValueV(m_shader, m_locs.sphereData, sphereData.data(),
                    SHADER_UNIFORM_VEC4, sphereCountGpu);
    SetShaderValueV(m_shader, m_locs.sphereColor, sphereColor.data(),
                    SHADER_UNIFORM_VEC4, sphereCountGpu);
  }

  std::array<Vector4, MAX_PLANES> planeData{};
  std::array<Vector4, MAX_PLANES> planeColor{};

  for (size_t i = 0; i < planeCount; ++i) {
    const Plane &plane = m_scene.planes[i];

    planeData[i] = {plane.normal.x, plane.normal.y, plane.normal.z,
                    plane.offset};

    planeColor[i] = {plane.color.x, plane.color.y, plane.color.z,
                     plane.shininess};
  }

  if (planeCountGpu > 0) {
    SetShaderValueV(m_shader, m_locs.planeData, planeData.data(),
                    SHADER_UNIFORM_VEC4, planeCountGpu);
    SetShaderValueV(m_shader, m_locs.planeColor, planeColor.data(),
                    SHADER_UNIFORM_VEC4, planeCountGpu);
  }

  std::array<Vector3, MAX_LIGHTS> dirLightDirection{};
  std::array<Vector3, MAX_LIGHTS> dirLightIntensity{};

  for (size_t i = 0; i < dirLightCount; ++i) {
    const DirectionalLight &light = m_scene.dirlights[i];

    dirLightDirection[i] = light.direction;
    dirLightIntensity[i] = light.intensity;
  }

  if (dirLightCountGpu > 0) {
    SetShaderValueV(m_shader, m_locs.dirLightDirection,
                    dirLightDirection.data(), SHADER_UNIFORM_VEC3,
                    dirLightCountGpu);

    SetShaderValueV(m_shader, m_locs.dirLightIntensity,
                    dirLightIntensity.data(), SHADER_UNIFORM_VEC3,
                    dirLightCountGpu);
  }

  std::array<Vector3, MAX_LIGHTS> spotlightPosition{};
  std::array<Vector3, MAX_LIGHTS> spotlightDirection{};
  std::array<Vector3, MAX_LIGHTS> spotlightIntensity{};
  std::array<float, MAX_LIGHTS> spotlightCosineCutoff{};

  for (size_t i = 0; i < spotlightCount; ++i) {
    const Spotlight &light = m_scene.spotlights[i];

    spotlightPosition[i] = light.position;
    spotlightDirection[i] = light.direction;
    spotlightIntensity[i] = light.intensity;
    spotlightCosineCutoff[i] = light.cosine_cutoff;
  }

  if (spotlightCountGpu > 0) {
    SetShaderValueV(m_shader, m_locs.spotlightPosition,
                    spotlightPosition.data(), SHADER_UNIFORM_VEC3,
                    spotlightCountGpu);

    SetShaderValueV(m_shader, m_locs.spotlightDirection,
                    spotlightDirection.data(), SHADER_UNIFORM_VEC3,
                    spotlightCountGpu);

    SetShaderValueV(m_shader, m_locs.spotlightIntensity,
                    spotlightIntensity.data(), SHADER_UNIFORM_VEC3,
                    spotlightCountGpu);

    SetShaderValueV(m_shader, m_locs.spotlightCosineCutoff,
                    spotlightCosineCutoff.data(), SHADER_UNIFORM_FLOAT,
                    spotlightCountGpu);
  }
}
void Application::render() {
  BeginDrawing();
  ClearBackground(BLACK);

  BeginShaderMode(m_shader);
  DrawRectangle(0, 0, m_width, m_height, WHITE);
  EndShaderMode();

  DrawFPS(10, 10);

  EndDrawing();
}
