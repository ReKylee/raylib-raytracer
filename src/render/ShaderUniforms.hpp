#pragma once

#include "raylib.h"

#include "scene/scene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>

namespace raytracer::render {

/// Shader uniform names shared by location lookup and upload code.
namespace shader_uniforms {

constexpr const char *resolution = "iResolution";
constexpr const char *time = "iTime";
constexpr const char *toneMapMode = "uToneMapMode";
constexpr const char *lightMode = "uLightMode";

constexpr const char *cameraToWorld = "uCameraToWorld";
constexpr const char *cameraViewportScale = "uCameraViewportScale";

constexpr const char *sphereCount = "uSphereCount";
constexpr const char *sphereData = "uSphereData";
constexpr const char *sphereColor = "uSphereColor";

constexpr const char *planeCount = "uPlaneCount";
constexpr const char *planeData = "uPlaneData";
constexpr const char *planeColor = "uPlaneColor";

constexpr const char *ambientIntensity = "uAmbientIntensity";
constexpr const char *gamma = "uGamma";

constexpr const char *directionalLightCount = "uDirectionalLightCount";
constexpr const char *directionalLightDirection = "uDirectionalLightDirection";
constexpr const char *directionalLightIntensity = "uDirectionalLightIntensity";

constexpr const char *spotlightCount = "uSpotlightCount";
constexpr const char *spotlightPosition = "uSpotlightPosition";
constexpr const char *spotlightDirectionCutoff = "uSpotlightDirectionCutoff";
constexpr const char *spotlightIntensity = "uSpotlightIntensity";

} // namespace shader_uniforms

/// Cached locations for all uniforms consumed by the ray tracing shader.
struct ShaderLocations {
  int resolution = -1;
  int time = -1;
  int toneMapMode = -1;
  int lightMode = -1;
  int gamma = -1;

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

/// Single-value uniform upload descriptor.
struct UniformUpload {
  /// Shader location returned by GetShaderLocation.
  int location = -1;

  /// Pointer to the value passed to raylib.
  const void *value = nullptr;

  /// raylib SHADER_UNIFORM_* type.
  int type = 0;
};

/// Array uniform upload descriptor.
struct UniformArrayUpload {
  /// Shader location returned by GetShaderLocation.
  int location = -1;

  /// Pointer to the first array element passed to raylib.
  const void *values = nullptr;

  /// raylib SHADER_UNIFORM_* element type.
  int type = 0;

  /// Number of elements to upload.
  int count = 0;
};

/// Finds and stores all shader uniform locations used by the renderer.
void LoadShaderLocations(Shader shader, ShaderLocations &locations);

/// Packs sphere position and radius into a vec4-compatible value.
Vector4 PackSphereData(const scene::Sphere &sphere);

/// Packs sphere color and shininess into a vec4-compatible value.
Vector4 PackSphereColor(const scene::Sphere &sphere);

/// Packs plane normal and offset into a vec4-compatible value.
Vector4 PackPlaneData(const scene::Plane &plane);

/// Packs plane color and shininess into a vec4-compatible value.
Vector4 PackPlaneColor(const scene::Plane &plane);

/// Packs spotlight direction and cutoff cosine into a vec4-compatible value.
Vector4 PackSpotlightDirectionCutoff(const scene::Spotlight &light);

/// Uploads a group of scalar/vector/matrix uniforms.
inline void UploadUniforms(Shader shader,
                           std::span<const UniformUpload> uploads) {
  std::ranges::for_each(uploads, [shader](const UniformUpload &upload) {
    SetShaderValue(shader, upload.location, upload.value, upload.type);
  });
}

/// Uploads non-empty uniform arrays.
inline void UploadUniformArrays(Shader shader,
                                std::span<const UniformArrayUpload> uploads) {
  auto activeUploads =
      uploads | std::views::filter([](const UniformArrayUpload &upload) {
        return upload.count > 0;
      });

  std::ranges::for_each(
      activeUploads, [shader](const UniformArrayUpload &upload) {
        SetShaderValueV(shader, upload.location, upload.values, upload.type,
                        upload.count);
      });
}

/// Returns a range size clamped to the shader-side capacity.
template <std::size_t MaxCount, std::ranges::sized_range Range>
std::size_t ClampedCount(const Range &range) {
  return std::min(std::ranges::size(range), MaxCount);
}

/// Packs the first count source elements into a fixed-size GPU upload array.
template <typename Packed, std::size_t Capacity,
          std::ranges::random_access_range SourceRange, typename Projection>
std::array<Packed, Capacity>
PackArray(const SourceRange &source, std::size_t count, Projection projection) {
  std::array<Packed, Capacity> packed{};

  std::ranges::transform(source | std::views::take(count), packed.begin(),
                         projection);

  return packed;
}

} // namespace raytracer::render
