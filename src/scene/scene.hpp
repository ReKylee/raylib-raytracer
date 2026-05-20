#pragma once

#include "raylib.h"

#include <variant>
#include <vector>

namespace raytracer::scene {

constexpr int MAX_SPHERES = 16;
constexpr int MAX_PLANES = 16;
constexpr int MAX_LIGHTS = 8;

struct Sphere {
  Vector3 position{};
  float radius = 1.0f;

  Vector3 color{1.0f, 1.0f, 1.0f};
  float shininess = 0.0f;
};

struct Plane {
  Vector3 normal{};
  float offset = 0.0f;

  Vector3 color{};
  float shininess = 0.0f;
};

// Temporary parser representation before objects are split into render arrays.
using SceneObject = std::variant<Sphere, Plane>;

struct AmbientLight {
  Vector3 intensity{};
};

struct DirectionalLight {
  Vector3 direction{};
  Vector3 intensity{};
};

struct Spotlight {
  Vector3 position{};
  Vector3 direction{};
  Vector3 intensity{};
  float cosine_cutoff = 0.0f;
};

struct CameraData {
  Vector3 position{0.0f, 0.5f, 4.0f};
  Vector3 forward{0.0f, 0.0f, -1.0f};
  Vector3 right{1.0f, 0.0f, 0.0f};
  Vector3 up{0.0f, 1.0f, 0.0f};
  float fovY = 60.0f;
};

struct Scene {
  CameraData camera{};
  AmbientLight ambient;
  std::vector<Sphere> spheres{};
  std::vector<Plane> planes{};
  std::vector<DirectionalLight> dirlights{};
  std::vector<Spotlight> spotlights{};
};

} // namespace raytracer::scene
