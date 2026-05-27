#pragma once

#include "scene/scene.hpp"

#include <string_view>

namespace raytracer::sceneParser {

// Parse a scene definition text file (Assignment 2 format) into a Scene.
//
// File format — each line is `<tag> v1 v2 v3 v4`:
//   e  x y z _      camera eye position           (4th coord ignored)
//   a  r g b _      global ambient intensity      (4th coord ignored)
//   o  x y z w      object: w > 0 -> Sphere(radius=w),
//                           w <= 0 -> Plane(offset=w)
//   c  r g b shin   material color + shininess; pairs with i-th 'o' line
//   d  x y z w      light direction: w == 0 directional, w == 1 spotlight
//   p  x y z cos    spotlight position + cone cutoff cosine; pairs with
//                   i-th *spotlight* 'd' line
//   i  r g b _      light intensity; pairs with i-th 'd' line
//
// Lines must appear in this exact order: e, a, o*, c*, d*, p*, i*. Blank
// lines and unknown tags are silently skipped. Throws std::runtime_error
// on missing file, out-of-order lines, malformed numbers, or count
// mismatches between paired groups.
scene::Scene parseScene(std::string_view path);

} // namespace raytracer::sceneParser
