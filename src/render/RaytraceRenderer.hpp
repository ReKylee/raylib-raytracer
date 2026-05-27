#pragma once

#include "raylib.h"

#include "render/ShaderUniforms.hpp"
#include "scene/scene.hpp"

namespace raytracer::render {

/// Default tone mapper selector used before debug controls change it.
constexpr int DEFAULT_TONE_MAP_MODE = 1;

/// Default light filter selector used before debug controls change it.
constexpr int DEFAULT_LIGHT_MODE = 0;

/// Default display gamma used by renderer debug options.
constexpr float DEFAULT_RENDER_GAMMA = 1.0f;

/// Per-frame shader controls used for visual debugging and presentation.
struct RenderDebugOptions {
  /// Tone mapper selector. Matches the TONE_MAP_* constants in the shader.
  int toneMapMode = DEFAULT_TONE_MAP_MODE;

  /// Light filter selector. Matches the LIGHT_MODE_* constants in the shader.
  int lightMode = DEFAULT_LIGHT_MODE;

  /// Gamma value used during final color correction.
  float gamma = DEFAULT_RENDER_GAMMA;
};

/// Uploads scene data to a ray tracing fragment shader and renders it full-screen.
class RaytraceRenderer {
public:
  /// Loads the fragment shader and caches all uniform locations.
  explicit RaytraceRenderer(const char *shaderPath);

  /// Unloads the raylib shader resource.
  ~RaytraceRenderer();

  RaytraceRenderer(const RaytraceRenderer &) = delete;
  RaytraceRenderer &operator=(const RaytraceRenderer &) = delete;

  /// Uploads values that can change every frame, including camera state.
  void updateFrame(int width, int height, const scene::CameraData &camera,
                   RenderDebugOptions debugOptions);

  /// Uploads static scene geometry and lighting data.
  void uploadScene(const scene::Scene &scene);

  /// Draws the ray traced image into the current render target.
  void render(int width, int height) const;

private:
  /// Converts the camera basis and projection size into shader uniforms.
  void uploadCamera(int width, int height, const scene::CameraData &camera);

private:
  Shader m_shader{};
  ShaderLocations m_locs{};
};

} // namespace raytracer::render
