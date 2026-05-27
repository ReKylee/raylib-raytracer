#pragma once

#include "raylib.h"

#include <variant>
#include <vector>

namespace raytracer::scene {

/// Maximum number of spheres that can be uploaded to the fragment shader.
constexpr int MAX_SPHERES = 16;

/// Maximum number of planes that can be uploaded to the fragment shader.
constexpr int MAX_PLANES = 16;

/// Maximum number of lights of each supported light type.
constexpr int MAX_LIGHTS = 8;

/// Sphere primitive with a Phong-style material.
struct Sphere {
  /// Center position in world space.
  Vector3 position{};

  /// Radius in world units.
  float radius = 1.0f;

  /// Diffuse RGB color in linear space.
  Vector3 color{1.0f, 1.0f, 1.0f};

  /// Specular exponent. Higher values create tighter highlights.
  float shininess = 0.0f;
};

/// Infinite plane represented by the equation dot(normal, p) + offset = 0.
struct Plane {
  /// Plane normal. It is normalized by the shader before intersection tests.
  Vector3 normal{};

  /// Plane offset in the implicit plane equation.
  float offset = 0.0f;

  /// Diffuse RGB color in linear space.
  Vector3 color{};

  /// Specular exponent. Higher values create tighter highlights.
  float shininess = 0.0f;
};

// Temporary parser representation before objects are split into render arrays.
using SceneObject = std::variant<Sphere, Plane>;

/// Ambient light applied uniformly to every visible surface.
struct AmbientLight {
  /// RGB intensity multiplier.
  Vector3 intensity{};
};

/// Directional light with parallel rays.
struct DirectionalLight {
  /// Direction the light travels in world space.
  Vector3 direction{};

  /// RGB light intensity.
  Vector3 intensity{};
};

/// Cone-shaped point light.
struct Spotlight {
  /// Light origin in world space.
  Vector3 position{};

  /// Direction the spotlight points in world space.
  Vector3 direction{};

  /// RGB light intensity before distance attenuation.
  Vector3 intensity{};

  /// Cosine of the outer cone angle.
  float cosineCutoff = 0.0f;
};

/// Camera basis used to generate primary rays in the shader.
struct CameraData {
  /// Camera position in world space.
  Vector3 position{0.0f, 0.5f, 4.0f};

  /// Forward view direction in world space.
  Vector3 forward{0.0f, 0.0f, -1.0f};

  /// Up vector used with forward to construct the camera basis.
  Vector3 up{0.0f, 1.0f, 0.0f};

  /// Vertical field of view in degrees.
  float fovY = 60.0f;
};

/// Complete CPU-side scene description consumed by the renderer.
struct Scene {
  /// Active camera.
  CameraData camera{};

  /// Global ambient term.
  AmbientLight ambient;

  /// Sphere primitives. Extra entries beyond MAX_SPHERES are ignored.
  std::vector<Sphere> spheres{};

  /// Plane primitives. Extra entries beyond MAX_PLANES are ignored.
  std::vector<Plane> planes{};

  /// Directional lights. Extra entries beyond MAX_LIGHTS are ignored.
  std::vector<DirectionalLight> directionalLights{};

  /// Spotlights. Extra entries beyond MAX_LIGHTS are ignored.
  std::vector<Spotlight> spotlights{};
};

} // namespace raytracer::scene
