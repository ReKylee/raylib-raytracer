#pragma once

#include "scene/scene.hpp"

#include <string_view>

namespace raytracer::scene {

/// Parses an Assignment 2 scene definition file into renderer scene data.
Scene ParseScene(std::string_view path);

} // namespace raytracer::scene
