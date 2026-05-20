#include "render/ShaderUniforms.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace {

int FindShaderLocation(Shader shader, const char *name) {
  return GetShaderLocation(shader, name);
}

int FindShaderArrayLocation(Shader shader, const char *name) {
  int loc = FindShaderLocation(shader, name);

  if (loc == -1) {
    const std::string firstElementName = std::string{name} + "[0]";
    loc = GetShaderLocation(shader, firstElementName.c_str());
  }

  return loc;
}

} // namespace

namespace raytracer::render {

void LoadShaderLocations(Shader shader, ShaderLocations &locations) {
  struct UniformLocation {
    int ShaderLocations::*location = nullptr;
    const char *name = nullptr;
  };

  constexpr auto scalarUniforms = std::to_array<UniformLocation>({
      {&ShaderLocations::resolution, shader_uniforms::resolution},
      {&ShaderLocations::time, shader_uniforms::time},
      {&ShaderLocations::cameraPosition, shader_uniforms::cameraPosition},
      {&ShaderLocations::cameraForward, shader_uniforms::cameraForward},
      {&ShaderLocations::cameraRight, shader_uniforms::cameraRight},
      {&ShaderLocations::cameraUp, shader_uniforms::cameraUp},
      {&ShaderLocations::cameraFovY, shader_uniforms::cameraFovY},
      {&ShaderLocations::sphereCount, shader_uniforms::sphereCount},
      {&ShaderLocations::planeCount, shader_uniforms::planeCount},
      {&ShaderLocations::ambientIntensity, shader_uniforms::ambientIntensity},
      {&ShaderLocations::dirLightCount, shader_uniforms::dirLightCount},
      {&ShaderLocations::spotlightCount, shader_uniforms::spotlightCount},
  });

  constexpr auto arrayUniforms = std::to_array<UniformLocation>({
      {&ShaderLocations::sphereData, shader_uniforms::sphereData},
      {&ShaderLocations::sphereColor, shader_uniforms::sphereColor},
      {&ShaderLocations::planeData, shader_uniforms::planeData},
      {&ShaderLocations::planeColor, shader_uniforms::planeColor},
      {&ShaderLocations::dirLightDirection,
       shader_uniforms::dirLightDirection},
      {&ShaderLocations::dirLightIntensity,
       shader_uniforms::dirLightIntensity},
      {&ShaderLocations::spotlightPosition,
       shader_uniforms::spotlightPosition},
      {&ShaderLocations::spotlightDirectionCutoff,
       shader_uniforms::spotlightDirectionCutoff},
      {&ShaderLocations::spotlightIntensity,
       shader_uniforms::spotlightIntensity},
  });

  std::ranges::for_each(scalarUniforms, [&](const UniformLocation &uniform) {
    locations.*uniform.location = FindShaderLocation(shader, uniform.name);
  });

  std::ranges::for_each(arrayUniforms, [&](const UniformLocation &uniform) {
    locations.*uniform.location = FindShaderArrayLocation(shader, uniform.name);
  });
}

Vector4 PackSphereData(const scene::Sphere &sphere) {
  return {
      sphere.position.x,
      sphere.position.y,
      sphere.position.z,
      sphere.radius,
  };
}

Vector4 PackSphereColor(const scene::Sphere &sphere) {
  return {
      sphere.color.x,
      sphere.color.y,
      sphere.color.z,
      sphere.shininess,
  };
}

Vector4 PackPlaneData(const scene::Plane &plane) {
  return {
      plane.normal.x,
      plane.normal.y,
      plane.normal.z,
      plane.offset,
  };
}

Vector4 PackPlaneColor(const scene::Plane &plane) {
  return {
      plane.color.x,
      plane.color.y,
      plane.color.z,
      plane.shininess,
  };
}

Vector4 PackSpotlightDirectionCutoff(const scene::Spotlight &light) {
  return {
      light.direction.x,
      light.direction.y,
      light.direction.z,
      light.cosine_cutoff,
  };
}

} // namespace raytracer::render
