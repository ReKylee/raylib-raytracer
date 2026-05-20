#pragma once

#include "raylib.h"

#include "render/ShaderUniforms.hpp"
#include "scene/scene.hpp"

namespace raytracer::render {

class RaytraceRenderer {
public:
  explicit RaytraceRenderer(const char *shaderPath);
  ~RaytraceRenderer();

  RaytraceRenderer(const RaytraceRenderer &) = delete;
  RaytraceRenderer &operator=(const RaytraceRenderer &) = delete;

  void updateFrame(int width, int height, const scene::CameraData &camera);
  void uploadScene(const scene::Scene &scene);
  void render(int width, int height) const;

private:
  void uploadCamera(int width, int height, const scene::CameraData &camera);

private:
  Shader m_shader{};
  ShaderLocations m_locs{};
};

} // namespace raytracer::render
