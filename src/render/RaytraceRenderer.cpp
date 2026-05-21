#include "render/RaytraceRenderer.hpp"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "raymath.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

namespace {

std::string BuildShaderDefines() {
  return TextFormat("#define MAX_SPHERES %d\n"
                    "#define MAX_PLANES %d\n"
                    "#define MAX_LIGHTS %d\n",
                    raytracer::scene::MAX_SPHERES, raytracer::scene::MAX_PLANES,
                    raytracer::scene::MAX_LIGHTS);
}

std::string InjectShaderDefines(std::string_view shaderSource,
                                std::string_view defines) {
  constexpr std::string_view versionDirective = "#version";

  if (!shaderSource.starts_with(versionDirective)) {
    std::string source;
    source.reserve(defines.size() + shaderSource.size());
    source.append(defines);
    source.append(shaderSource);
    return source;
  }

  const std::size_t lineEnd = shaderSource.find('\n');
  const std::size_t insertOffset =
      lineEnd == std::string_view::npos ? shaderSource.size() : lineEnd + 1;

  std::string source;
  source.reserve(shaderSource.size() + defines.size());
  source.append(shaderSource.substr(0, insertOffset));
  source.append(defines);
  source.append(shaderSource.substr(insertOffset));
  return source;
}

Shader LoadFragmentShaderWithDefines(const char *shaderPath) {
  char *fragmentShaderText = LoadFileText(shaderPath);

  if (fragmentShaderText == nullptr) {
    return LoadShader(nullptr, shaderPath);
  }

  const std::string source =
      InjectShaderDefines(fragmentShaderText, BuildShaderDefines());
  UnloadFileText(fragmentShaderText);

  return LoadShaderFromMemory(nullptr, source.c_str());
}

} // namespace

namespace raytracer::render {

using scene::DirectionalLight;
using scene::MAX_LIGHTS;
using scene::MAX_PLANES;
using scene::MAX_SPHERES;
using scene::Spotlight;

namespace {

Matrix CreateCameraToWorld(const scene::CameraData &camera) {
  const Vector3 forward = Vector3Normalize(camera.forward);
  const Vector3 right =
      Vector3Normalize(Vector3CrossProduct(forward, camera.up));
  const Vector3 up = Vector3CrossProduct(right, forward);

  Matrix cameraToWorld = MatrixIdentity();
  cameraToWorld.m0 = right.x;
  cameraToWorld.m1 = right.y;
  cameraToWorld.m2 = right.z;

  cameraToWorld.m4 = up.x;
  cameraToWorld.m5 = up.y;
  cameraToWorld.m6 = up.z;

  cameraToWorld.m8 = -forward.x;
  cameraToWorld.m9 = -forward.y;
  cameraToWorld.m10 = -forward.z;

  cameraToWorld.m12 = camera.position.x;
  cameraToWorld.m13 = camera.position.y;
  cameraToWorld.m14 = camera.position.z;

  return cameraToWorld;
}

Vector2 CreateViewportScale(int width, int height, float fovYDegrees) {
  const float safeHeight = static_cast<float>(height > 0 ? height : 1);
  const float aspect = static_cast<float>(width) / safeHeight;
  const float verticalScale = std::tan(fovYDegrees * DEG2RAD * 0.5f);

  return {aspect * verticalScale, verticalScale};
}

} // namespace

RaytraceRenderer::RaytraceRenderer(const char *shaderPath) {
  m_shader = LoadFragmentShaderWithDefines(shaderPath);
  LoadShaderLocations(m_shader, m_locs);
}

RaytraceRenderer::~RaytraceRenderer() { UnloadShader(m_shader); }

void RaytraceRenderer::updateFrame(int width, int height,
                                   const scene::CameraData &camera,
                                   RenderDebugOptions debugOptions) {
  const float time = static_cast<float>(GetTime());
  const float resolution[2] = {
      static_cast<float>(width),
      static_cast<float>(height),
  };

  const auto frameUniforms = std::to_array<UniformUpload>({
      {m_locs.resolution, resolution, SHADER_UNIFORM_VEC2},
      {m_locs.time, &time, SHADER_UNIFORM_FLOAT},
      {m_locs.toneMapMode, &debugOptions.toneMapMode, SHADER_UNIFORM_INT},
      {m_locs.lightMode, &debugOptions.lightMode, SHADER_UNIFORM_INT},
  });

  UploadUniforms(m_shader, frameUniforms);
  uploadCamera(width, height, camera);
}

void RaytraceRenderer::uploadCamera(int width, int height,
                                    const scene::CameraData &camera) {
  const Matrix cameraToWorld = CreateCameraToWorld(camera);
  const Vector2 viewportScale = CreateViewportScale(width, height, camera.fovY);

  SetShaderValueMatrix(m_shader, m_locs.cameraToWorld, cameraToWorld);

  const auto cameraUniforms = std::to_array<UniformUpload>({
      {m_locs.cameraViewportScale, &viewportScale, SHADER_UNIFORM_VEC2},
  });

  UploadUniforms(m_shader, cameraUniforms);
}

void RaytraceRenderer::uploadScene(const scene::Scene &scene) {
  const std::size_t sphereCount = ClampedCount<MAX_SPHERES>(scene.spheres);
  const std::size_t planeCount = ClampedCount<MAX_PLANES>(scene.planes);
  const std::size_t directionalLightCount =
      ClampedCount<MAX_LIGHTS>(scene.directionalLights);
  const std::size_t spotlightCount = ClampedCount<MAX_LIGHTS>(scene.spotlights);

  const int sphereCountGpu = static_cast<int>(sphereCount);
  const int planeCountGpu = static_cast<int>(planeCount);
  const int directionalLightCountGpu = static_cast<int>(directionalLightCount);
  const int spotlightCountGpu = static_cast<int>(spotlightCount);

  const auto sceneUniforms = std::to_array<UniformUpload>({
      {m_locs.ambientIntensity, &scene.ambient.intensity, SHADER_UNIFORM_VEC3},
      {m_locs.sphereCount, &sphereCountGpu, SHADER_UNIFORM_INT},
      {m_locs.planeCount, &planeCountGpu, SHADER_UNIFORM_INT},
      {m_locs.directionalLightCount, &directionalLightCountGpu,
       SHADER_UNIFORM_INT},
      {m_locs.spotlightCount, &spotlightCountGpu, SHADER_UNIFORM_INT},
  });

  UploadUniforms(m_shader, sceneUniforms);

  const auto sphereData = PackArray<Vector4, MAX_SPHERES>(
      scene.spheres, sphereCount, PackSphereData);
  const auto sphereColor = PackArray<Vector4, MAX_SPHERES>(
      scene.spheres, sphereCount, PackSphereColor);

  const auto sphereUploads = std::to_array<UniformArrayUpload>({
      {m_locs.sphereData, sphereData.data(), SHADER_UNIFORM_VEC4,
       sphereCountGpu},
      {m_locs.sphereColor, sphereColor.data(), SHADER_UNIFORM_VEC4,
       sphereCountGpu},
  });

  UploadUniformArrays(m_shader, sphereUploads);

  const auto planeData =
      PackArray<Vector4, MAX_PLANES>(scene.planes, planeCount, PackPlaneData);
  const auto planeColor =
      PackArray<Vector4, MAX_PLANES>(scene.planes, planeCount, PackPlaneColor);

  const auto planeUploads = std::to_array<UniformArrayUpload>({
      {m_locs.planeData, planeData.data(), SHADER_UNIFORM_VEC4, planeCountGpu},
      {m_locs.planeColor, planeColor.data(), SHADER_UNIFORM_VEC4,
       planeCountGpu},
  });

  UploadUniformArrays(m_shader, planeUploads);

  const auto directionalLightDirection = PackArray<Vector3, MAX_LIGHTS>(
      scene.directionalLights, directionalLightCount,
      &DirectionalLight::direction);
  const auto directionalLightIntensity = PackArray<Vector3, MAX_LIGHTS>(
      scene.directionalLights, directionalLightCount,
      &DirectionalLight::intensity);

  const auto directionalLightUploads = std::to_array<UniformArrayUpload>({
      {m_locs.directionalLightDirection, directionalLightDirection.data(),
       SHADER_UNIFORM_VEC3, directionalLightCountGpu},
      {m_locs.directionalLightIntensity, directionalLightIntensity.data(),
       SHADER_UNIFORM_VEC3, directionalLightCountGpu},
  });

  UploadUniformArrays(m_shader, directionalLightUploads);

  const auto spotlightPosition = PackArray<Vector3, MAX_LIGHTS>(
      scene.spotlights, spotlightCount, &Spotlight::position);
  const auto spotlightDirectionCutoff = PackArray<Vector4, MAX_LIGHTS>(
      scene.spotlights, spotlightCount, PackSpotlightDirectionCutoff);
  const auto spotlightIntensity = PackArray<Vector3, MAX_LIGHTS>(
      scene.spotlights, spotlightCount, &Spotlight::intensity);

  const auto spotlightUploads = std::to_array<UniformArrayUpload>({
      {m_locs.spotlightPosition, spotlightPosition.data(), SHADER_UNIFORM_VEC3,
       spotlightCountGpu},
      {m_locs.spotlightDirectionCutoff, spotlightDirectionCutoff.data(),
       SHADER_UNIFORM_VEC4, spotlightCountGpu},
      {m_locs.spotlightIntensity, spotlightIntensity.data(),
       SHADER_UNIFORM_VEC3, spotlightCountGpu},
  });

  UploadUniformArrays(m_shader, spotlightUploads);
}

void RaytraceRenderer::render(int width, int height) const {
  BeginShaderMode(m_shader);
  DrawRectangle(0, 0, width, height, WHITE);
  EndShaderMode();
}

} // namespace raytracer::render
