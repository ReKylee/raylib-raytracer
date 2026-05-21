#pragma once

#include "raylib.h"

#include "scene/scene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>

namespace raytracer::render {

namespace shader_uniforms {

constexpr const char *resolution = "iResolution";
constexpr const char *time = "iTime";

constexpr const char *cameraToWorld = "uCameraToWorld";
constexpr const char *cameraViewportScale = "uCameraViewportScale";

constexpr const char *sphereCount = "uSphereCount";
constexpr const char *sphereData = "uSphereData";
constexpr const char *sphereColor = "uSphereColor";

constexpr const char *planeCount = "uPlaneCount";
constexpr const char *planeData = "uPlaneData";
constexpr const char *planeColor = "uPlaneColor";

constexpr const char *ambientIntensity = "uAmbientIntensity";

constexpr const char *directionalLightCount = "uDirectionalLightCount";
constexpr const char *directionalLightDirection = "uDirectionalLightDirection";
constexpr const char *directionalLightIntensity = "uDirectionalLightIntensity";

constexpr const char *spotlightCount = "uSpotlightCount";
constexpr const char *spotlightPosition = "uSpotlightPosition";
constexpr const char *spotlightDirectionCutoff = "uSpotlightDirectionCutoff";
constexpr const char *spotlightIntensity = "uSpotlightIntensity";

} // namespace shader_uniforms

struct ShaderLocations {
  int resolution = -1;
  int time = -1;

  int cameraToWorld = -1;
  int cameraViewportScale = -1;

  int sphereCount = -1;
  int sphereData = -1;
  int sphereColor = -1;

  int planeCount = -1;
  int planeData = -1;
  int planeColor = -1;

  int ambientIntensity = -1;

  int directionalLightCount = -1;
  int directionalLightDirection = -1;
  int directionalLightIntensity = -1;

  int spotlightCount = -1;
  int spotlightPosition = -1;
  int spotlightDirectionCutoff = -1;
  int spotlightIntensity = -1;
};

struct UniformUpload {
  int location = -1;
  const void *value = nullptr;
  int type = 0;
};

struct UniformArrayUpload {
  int location = -1;
  const void *values = nullptr;
  int type = 0;
  int count = 0;
};

void LoadShaderLocations(Shader shader, ShaderLocations &locations);

Vector4 PackSphereData(const scene::Sphere &sphere);
Vector4 PackSphereColor(const scene::Sphere &sphere);
Vector4 PackPlaneData(const scene::Plane &plane);
Vector4 PackPlaneColor(const scene::Plane &plane);
Vector4 PackSpotlightDirectionCutoff(const scene::Spotlight &light);

inline void UploadUniforms(Shader shader,
                           std::span<const UniformUpload> uploads) {
  std::ranges::for_each(uploads, [shader](const UniformUpload &upload) {
    SetShaderValue(shader, upload.location, upload.value, upload.type);
  });
}

inline void UploadUniformArrays(Shader shader,
                                std::span<const UniformArrayUpload> uploads) {
  auto activeUploads =
      uploads | std::views::filter(
                    [](const UniformArrayUpload &upload) {
                      return upload.count > 0;
                    });

  std::ranges::for_each(
      activeUploads, [shader](const UniformArrayUpload &upload) {
        SetShaderValueV(shader, upload.location, upload.values, upload.type,
                        upload.count);
      });
}

template <std::size_t MaxCount, std::ranges::sized_range Range>
std::size_t ClampedCount(const Range &range) {
  return std::min(std::ranges::size(range), MaxCount);
}

template <typename Packed, std::size_t Capacity, std::ranges::random_access_range SourceRange,
          typename Projection>
std::array<Packed, Capacity> PackArray(const SourceRange &source,
                                       std::size_t count,
                                       Projection projection) {
  std::array<Packed, Capacity> packed{};

  std::ranges::transform(source | std::views::take(count), packed.begin(),
                         projection);

  return packed;
}

} // namespace raytracer::render
