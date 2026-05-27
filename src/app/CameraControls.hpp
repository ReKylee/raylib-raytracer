#pragma once

#include "scene/scene.hpp"

namespace raytracer::app::camera {

/// Initializes the camera orientation quaternion from the current camera basis.
void Initialize(scene::CameraData &camera, Vector4 &orientation);

/// Applies mouse-look and keyboard movement to a free-flying camera.
void UpdateFreeCamera(scene::CameraData &camera, Vector4 &orientation);

} // namespace raytracer::app::camera
