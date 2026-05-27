#include "scene/SceneParser.hpp"

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

namespace raytracer::scene {
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

[[noreturn]] void ThrowError(const std::string &message) {
  throw std::runtime_error("SceneParser: " + message);
}

std::optional<TaggedLine> TokenizeLine(const std::string &line) {
  const auto start = line.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return std::nullopt;
  }

  const char tag = line[start];
  if (VALID_TAGS.find(tag) == std::string_view::npos) {
    ThrowError(std::string{"unknown tag '"} + tag + "' on line: " + line);
  }

  if (start + 1 >= line.size() ||
      !std::isspace(static_cast<unsigned char>(line[start + 1]))) {
    ThrowError(std::string{"tag '"} + tag +
               "' must be followed by whitespace: " + line);
  }

  std::istringstream iss{line.substr(start + 2)};
  float values[4] = {};
  iss >> values[0] >> values[1] >> values[2] >> values[3];
  if (iss.fail()) {
    ThrowError(std::string{"malformed '"} + tag +
               "' line (expected 4 numbers): " + line);
  }

  std::string extra;
  if (iss >> extra) {
    ThrowError(std::string{"extra tokens on '"} + tag + "' line: " + line);
  }

  return TaggedLine{tag, Vector4{values[0], values[1], values[2], values[3]}};
}

TokenStream TokenizeFile(const std::string &path) {
  std::ifstream file{path};
  if (!file) {
    ThrowError("cannot open file: " + path);
  }

  TokenStream stream;
  std::string line;
  while (std::getline(file, line)) {
    if (auto tagged = TokenizeLine(line)) {
      stream.lines.push_back(*tagged);
    }
  }

  return stream;
}

Vector4 ReadOne(TokenStream &stream, char expected) {
  if (!stream.hasMore()) {
    ThrowError(std::string{"expected '"} + expected +
               "' tag, got end of file");
  }

  if (stream.peek().tag != expected) {
    ThrowError(std::string{"expected '"} + expected + "' tag, got '" +
               stream.peek().tag + "'");
  }

  return stream.consume().values;
}

std::vector<Vector4> ReadMany(TokenStream &stream, char expected) {
  std::vector<Vector4> result;
  while (stream.hasMore() && stream.peek().tag == expected) {
    result.push_back(stream.consume().values);
  }

  return result;
}

std::vector<Vector4> ReadExactly(TokenStream &stream, char expected,
                                 std::size_t count) {
  std::vector<Vector4> result;
  result.reserve(count);

  for (std::size_t index = 0; index < count; ++index) {
    result.push_back(ReadOne(stream, expected));
  }

  return result;
}

CameraData CameraFromEye(Vector3 eye) {
  const float eyeDistance = Vector3Length(eye);
  if (eyeDistance < 1e-6f) {
    ThrowError("camera eye 'e' is at the origin");
  }

  return CameraData{
      .position = eye,
      .forward = Vector3Scale(eye, -1.0f / eyeDistance),
      .up = Vector3{0.0f, 1.0f, 0.0f},
      .fovY = 2.0f * std::atan(1.0f / eyeDistance) * RAD2DEG,
  };
}

Vector3 Xyz(const Vector4 &value) {
  return Vector3{value.x, value.y, value.z};
}

} // namespace

Scene ParseScene(std::string_view path) {
  TokenStream stream = TokenizeFile(std::string{path});

  const Vector4 eye = ReadOne(stream, 'e');
  const Vector4 ambient = ReadOne(stream, 'a');
  const auto objects = ReadMany(stream, 'o');
  const auto colors = ReadExactly(stream, 'c', objects.size());
  const auto directions = ReadMany(stream, 'd');

  for (const Vector4 &direction : directions) {
    if (direction.w != 0.0f && direction.w != 1.0f) {
      ThrowError("'d' 4th coord must be 0.0 (directional) or 1.0 (spotlight)");
    }
  }

  const std::size_t spotlightCount = static_cast<std::size_t>(
      std::count_if(directions.begin(), directions.end(),
                    [](const Vector4 &direction) {
                      return direction.w == 1.0f;
                    }));

  const auto positions = ReadExactly(stream, 'p', spotlightCount);
  const auto intensities = ReadExactly(stream, 'i', directions.size());

  for (const Vector4 &position : positions) {
    if (position.w < -1.0f || position.w > 1.0f) {
      ThrowError("'p' cosine cutoff must be in [-1, 1]");
    }
  }

  if (stream.hasMore()) {
    ThrowError(std::string{"unexpected '"} + stream.peek().tag +
               "' tag after expected sections");
  }

  Scene scene;
  scene.camera = CameraFromEye(Xyz(eye));
  scene.ambient.intensity = Xyz(ambient);

  for (std::size_t index = 0; index < objects.size(); ++index) {
    const Vector4 &object = objects[index];
    const Vector4 &color = colors[index];

    if (object.w > 0.0f) {
      scene.spheres.push_back(Sphere{
          .position = Xyz(object),
          .radius = object.w,
          .color = Xyz(color),
          .shininess = color.w,
      });
    } else {
      scene.planes.push_back(Plane{
          .normal = Xyz(object),
          .offset = object.w,
          .color = Xyz(color),
          .shininess = color.w,
      });
    }
  }

  std::size_t spotlightIndex = 0;
  for (std::size_t index = 0; index < directions.size(); ++index) {
    const Vector4 &direction = directions[index];
    const Vector4 &intensity = intensities[index];

    if (direction.w == 0.0f) {
      scene.directionalLights.push_back(DirectionalLight{
          .direction = Xyz(direction),
          .intensity = Xyz(intensity),
      });
    } else {
      const Vector4 &position = positions[spotlightIndex++];
      scene.spotlights.push_back(Spotlight{
          .position = Xyz(position),
          .direction = Xyz(direction),
          .intensity = Xyz(intensity),
          .cosineCutoff = position.w,
      });
    }
  }

  return scene;
}

} // namespace raytracer::scene
