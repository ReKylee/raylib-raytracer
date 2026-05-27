#include "sceneParser/SceneParser.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "raymath.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace raytracer::sceneParser {

namespace {

constexpr std::string_view VALID_TAGS = "eaocdpi";

struct TaggedLine {
  char tag = 0;
  Vector4 values{};
};

struct TokenStream {
  std::vector<TaggedLine> lines;
  std::size_t cursor = 0;

  bool hasMore() const { return cursor < lines.size(); }
  const TaggedLine &peek() const { return lines[cursor]; }
  TaggedLine consume() { return lines[cursor++]; }
};

[[noreturn]] void throwError(const std::string &msg) {
  throw std::runtime_error("SceneParser: " + msg);
}

// Parse one raw line. Returns nullopt for blank lines or unknown tags
// (silently skipped). Throws if the line begins with a known tag but the
// numeric payload is malformed.
std::optional<TaggedLine> tokenizeLine(const std::string &line) {
  // Blank lines are the only thing silently tolerated.
  const auto start = line.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return std::nullopt;
  }

  const char tag = line[start];
  if (VALID_TAGS.find(tag) == std::string_view::npos) {
    throwError(std::string{"unknown tag '"} + tag + "' on line: " + line);
  }

  if (start + 1 >= line.size() ||
      !std::isspace(static_cast<unsigned char>(line[start + 1]))) {
    throwError(std::string{"tag '"} + tag +
               "' must be followed by whitespace: " + line);
  }

  std::istringstream iss{line.substr(start + 2)};
  float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  iss >> v[0] >> v[1] >> v[2] >> v[3];
  if (iss.fail()) {
    throwError(std::string{"malformed '"} + tag +
               "' line (expected 4 numbers): " + line);
  }

  std::string extra;
  if (iss >> extra) {
    throwError(std::string{"extra tokens on '"} + tag + "' line: " + line);
  }

  return TaggedLine{tag, Vector4{v[0], v[1], v[2], v[3]}};
}

TokenStream tokenizeFile(const std::string &path) {
  std::ifstream file{path};
  if (!file) {
    throwError("cannot open file: " + path);
  }

  TokenStream stream;
  std::string line;
  while (std::getline(file, line)) {
    if (auto tagged = tokenizeLine(line)) {
      stream.lines.push_back(*tagged);
    }
  }
  return stream;
}

Vector4 readOne(TokenStream &s, char expected) {
  if (!s.hasMore()) {
    throwError(std::string{"expected '"} + expected + "' tag, got end of file");
  }
  if (s.peek().tag != expected) {
    throwError(std::string{"expected '"} + expected + "' tag, got '" +
               s.peek().tag + "'");
  }
  return s.consume().values;
}

std::vector<Vector4> readMany(TokenStream &s, char expected) {
  std::vector<Vector4> result;
  while (s.hasMore() && s.peek().tag == expected) {
    result.push_back(s.consume().values);
  }
  return result;
}

std::vector<Vector4> readExactly(TokenStream &s, char expected,
                                 std::size_t n) {
  std::vector<Vector4> result;
  result.reserve(n);
  for (std::size_t k = 0; k < n; ++k) {
    result.push_back(readOne(s, expected));
  }
  return result;
}

// Assignment camera convention: screen is the [-1,1]x[-1,1] square at z=0,
// camera looks at the origin. All provided scenes have eye on the z-axis,
// so this collapses to forward=(0,0,-1), up=(0,1,0), fovY=2*atan(1/|eye|).
scene::CameraData cameraFromEye(Vector3 eye) {
  const float eyeDist = Vector3Length(eye);
  if (eyeDist < 1e-6f) {
    throwError("camera eye 'e' is at the origin");
  }

  return scene::CameraData{
      .position = eye,
      .forward = Vector3Scale(eye, -1.0f / eyeDist),
      .up = Vector3{0.0f, 1.0f, 0.0f},
      .fovY = 2.0f * std::atan(1.0f / eyeDist) * RAD2DEG,
  };
}

Vector3 xyz(const Vector4 &v) { return Vector3{v.x, v.y, v.z}; }

} // namespace

scene::Scene parseScene(std::string_view path) {
  TokenStream s = tokenizeFile(std::string{path});

  const Vector4 eVec = readOne(s, 'e');
  const Vector4 aVec = readOne(s, 'a');
  const auto oVecs = readMany(s, 'o');
  const auto cVecs = readExactly(s, 'c', oVecs.size());
  const auto dVecs = readMany(s, 'd');

  for (const Vector4 &d : dVecs) {
    if (d.w != 0.0f && d.w != 1.0f) {
      throwError("'d' 4th coord must be 0.0 (directional) or 1.0 (spotlight)");
    }
  }

  // Count spotlights (d.w == 1) so we know how many 'p' lines to expect.
  const std::size_t spotCount = static_cast<std::size_t>(
      std::count_if(dVecs.begin(), dVecs.end(),
                    [](const Vector4 &d) { return d.w == 1.0f; }));

  const auto pVecs = readExactly(s, 'p', spotCount);
  const auto iVecs = readExactly(s, 'i', dVecs.size());

  for (const Vector4 &p : pVecs) {
    if (p.w < -1.0f || p.w > 1.0f) {
      throwError("'p' cosine cutoff must be in [-1, 1]");
    }
  }

  // Any leftover tag means a section appeared out of order.
  if (s.hasMore()) {
    throwError(std::string{"unexpected '"} + s.peek().tag +
               "' tag after expected sections");
  }

  scene::Scene scene;
  scene.camera = cameraFromEye(xyz(eVec));
  scene.ambient.intensity = xyz(aVec);

  for (std::size_t k = 0; k < oVecs.size(); ++k) {
    const Vector4 &obj = oVecs[k];
    const Vector4 &col = cVecs[k];

    if (obj.w > 0.0f) {
      scene.spheres.push_back(scene::Sphere{
          .position = xyz(obj),
          .radius = obj.w,
          .color = xyz(col),
          .shininess = col.w,
      });
    } else {
      scene.planes.push_back(scene::Plane{
          .normal = xyz(obj),
          .offset = obj.w,
          .color = xyz(col),
          .shininess = col.w,
      });
    }
  }

  // 'p' only pairs with spotlight 'd's, so its cursor advances independently.
  std::size_t spotIdx = 0;
  for (std::size_t k = 0; k < dVecs.size(); ++k) {
    const Vector4 &dir = dVecs[k];
    const Vector4 &intensity = iVecs[k];

    if (dir.w == 0.0f) {
      scene.directionalLights.push_back(scene::DirectionalLight{
          .direction = xyz(dir),
          .intensity = xyz(intensity),
      });
    } else {
      const Vector4 &pos = pVecs[spotIdx++];
      scene.spotlights.push_back(scene::Spotlight{
          .position = xyz(pos),
          .direction = xyz(dir),
          .intensity = xyz(intensity),
          .cosineCutoff = pos.w,
      });
    }
  }

  return scene;
}

} // namespace raytracer::sceneParser
