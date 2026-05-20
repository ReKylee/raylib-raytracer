#include "render/RaytraceRenderer.hpp"

#include <array>
#include <cstddef>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <inplace_vector>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
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

template <typename Upload>
using UniformUploadList = std::inplace_vector<Upload, 8>;

Matrix CreateCameraToWorld(const scene::CameraData &camera) {
  return Matrix{
      camera.right.x,   camera.up.x,   camera.forward.x,   camera.position.x,
      camera.right.y,   camera.up.y,   camera.forward.y,   camera.position.y,
      camera.right.z,   camera.up.z,   camera.forward.z,   camera.position.z,
      0.0f,             0.0f,          0.0f,               1.0f,
  };
}

} // namespace

RaytraceRenderer::RaytraceRenderer(const char *shaderPath) {
  m_shader = LoadFragmentShaderWithDefines(shaderPath);
  LoadShaderLocations(m_shader, m_locs);
}

RaytraceRenderer::~RaytraceRenderer() { UnloadShader(m_shader); }

void RaytraceRenderer::updateFrame(int width, int height,
                                   const scene::CameraData &camera) {
  const float time = static_cast<float>(GetTime());
  const float resolution[2] = {
      static_cast<float>(width),
      static_cast<float>(height),
  };

  UniformUploadList<UniformUpload> frameUniforms;
  frameUniforms.push_back({m_locs.resolution, resolution, SHADER_UNIFORM_VEC2});
  frameUniforms.push_back({m_locs.time, &time, SHADER_UNIFORM_FLOAT});

  UploadUniforms(m_shader, frameUniforms);
  uploadCamera(camera);
}

void RaytraceRenderer::uploadCamera(const scene::CameraData &camera) {
  const Matrix cameraToWorld = CreateCameraToWorld(camera);

  SetShaderValueMatrix(m_shader, m_locs.cameraToWorld, cameraToWorld);

  UniformUploadList<UniformUpload> cameraUniforms;
  cameraUniforms.push_back(
      {m_locs.cameraFovY, &camera.fovY, SHADER_UNIFORM_FLOAT});

  UploadUniforms(m_shader, cameraUniforms);
}

void RaytraceRenderer::uploadScene(const scene::Scene &scene) {
  uploadCamera(scene.camera);

  const std::size_t sphereCount = ClampedCount<MAX_SPHERES>(scene.spheres);
  const std::size_t planeCount = ClampedCount<MAX_PLANES>(scene.planes);
  const std::size_t dirLightCount = ClampedCount<MAX_LIGHTS>(scene.dirlights);
  const std::size_t spotlightCount = ClampedCount<MAX_LIGHTS>(scene.spotlights);

  const int sphereCountGpu = static_cast<int>(sphereCount);
  const int planeCountGpu = static_cast<int>(planeCount);
  const int dirLightCountGpu = static_cast<int>(dirLightCount);
  const int spotlightCountGpu = static_cast<int>(spotlightCount);

  UniformUploadList<UniformUpload> sceneUniforms;
  sceneUniforms.push_back(
      {m_locs.ambientIntensity, &scene.ambient.intensity, SHADER_UNIFORM_VEC3});
  sceneUniforms.push_back(
      {m_locs.sphereCount, &sphereCountGpu, SHADER_UNIFORM_INT});
  sceneUniforms.push_back(
      {m_locs.planeCount, &planeCountGpu, SHADER_UNIFORM_INT});
  sceneUniforms.push_back(
      {m_locs.dirLightCount, &dirLightCountGpu, SHADER_UNIFORM_INT});
  sceneUniforms.push_back(
      {m_locs.spotlightCount, &spotlightCountGpu, SHADER_UNIFORM_INT});

  UploadUniforms(m_shader, sceneUniforms);

  const auto sphereData = PackArray<Vector4, MAX_SPHERES>(
      scene.spheres, sphereCount, PackSphereData);
  const auto sphereColor = PackArray<Vector4, MAX_SPHERES>(
      scene.spheres, sphereCount, PackSphereColor);

  UniformUploadList<UniformArrayUpload> sphereUploads;
  sphereUploads.push_back({m_locs.sphereData, sphereData.data(),
                           SHADER_UNIFORM_VEC4, sphereCountGpu});
  sphereUploads.push_back({m_locs.sphereColor, sphereColor.data(),
                           SHADER_UNIFORM_VEC4, sphereCountGpu});

  UploadUniformArrays(m_shader, sphereUploads);

  const auto planeData =
      PackArray<Vector4, MAX_PLANES>(scene.planes, planeCount, PackPlaneData);
  const auto planeColor =
      PackArray<Vector4, MAX_PLANES>(scene.planes, planeCount, PackPlaneColor);

  UniformUploadList<UniformArrayUpload> planeUploads;
  planeUploads.push_back(
      {m_locs.planeData, planeData.data(), SHADER_UNIFORM_VEC4, planeCountGpu});
  planeUploads.push_back({m_locs.planeColor, planeColor.data(),
                          SHADER_UNIFORM_VEC4, planeCountGpu});

  UploadUniformArrays(m_shader, planeUploads);

  const auto dirLightDirection = PackArray<Vector3, MAX_LIGHTS>(
      scene.dirlights, dirLightCount, &DirectionalLight::direction);
  const auto dirLightIntensity = PackArray<Vector3, MAX_LIGHTS>(
      scene.dirlights, dirLightCount, &DirectionalLight::intensity);

  UniformUploadList<UniformArrayUpload> dirLightUploads;
  dirLightUploads.push_back({m_locs.dirLightDirection, dirLightDirection.data(),
                             SHADER_UNIFORM_VEC3, dirLightCountGpu});
  dirLightUploads.push_back({m_locs.dirLightIntensity, dirLightIntensity.data(),
                             SHADER_UNIFORM_VEC3, dirLightCountGpu});

  UploadUniformArrays(m_shader, dirLightUploads);

  const auto spotlightPosition = PackArray<Vector3, MAX_LIGHTS>(
      scene.spotlights, spotlightCount, &Spotlight::position);
  const auto spotlightDirectionCutoff = PackArray<Vector4, MAX_LIGHTS>(
      scene.spotlights, spotlightCount, PackSpotlightDirectionCutoff);
  const auto spotlightIntensity = PackArray<Vector3, MAX_LIGHTS>(
      scene.spotlights, spotlightCount, &Spotlight::intensity);

  UniformUploadList<UniformArrayUpload> spotlightUploads;
  spotlightUploads.push_back({m_locs.spotlightPosition,
                              spotlightPosition.data(), SHADER_UNIFORM_VEC3,
                              spotlightCountGpu});
  spotlightUploads.push_back({m_locs.spotlightDirectionCutoff,
                              spotlightDirectionCutoff.data(),
                              SHADER_UNIFORM_VEC4, spotlightCountGpu});
  spotlightUploads.push_back({m_locs.spotlightIntensity,
                              spotlightIntensity.data(), SHADER_UNIFORM_VEC3,
                              spotlightCountGpu});

  UploadUniformArrays(m_shader, spotlightUploads);
}

void RaytraceRenderer::render(int width, int height) const {
  BeginShaderMode(m_shader);
  DrawRectangle(0, 0, width, height, WHITE);
  EndShaderMode();
}

} // namespace raytracer::render
