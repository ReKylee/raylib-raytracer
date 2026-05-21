#pragma once

#include "scene/scene.hpp"

namespace raytracer::app::camera {

void Initialize(scene::CameraData &camera, Vector4 &orientation);
void UpdateFreeCamera(scene::CameraData &camera, Vector4 &orientation);

} // namespace raytracer::app::camera
