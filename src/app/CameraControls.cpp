#include "app/CameraControls.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "raymath.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace raytracer::app::camera {
namespace {

constexpr float MOVE_SPEED = 4.0f;
constexpr float FAST_MOVE_MULTIPLIER = 3.0f;
constexpr float MOUSE_SENSITIVITY = 0.12f;
constexpr Vector3 DEFAULT_FORWARD{0.0f, 0.0f, -1.0f};
constexpr Vector3 DEFAULT_UP{0.0f, 1.0f, 0.0f};

Vector3 RotateVector(Vector3 vector, Quaternion orientation) {
  return Vector3Normalize(Vector3RotateByQuaternion(vector, orientation));
}

Vector3 CameraRight(const scene::CameraData &camera) {
  return Vector3Normalize(Vector3CrossProduct(camera.forward, camera.up));
}

Quaternion InitialOrientation(const scene::CameraData &camera) {
  return QuaternionFromVector3ToVector3(DEFAULT_FORWARD,
                                        Vector3Normalize(camera.forward));
}

// Keeps the camera basis derived from one quaternion so mouse rotations do not
// gradually skew the forward/up vectors.
void ApplyOrientation(scene::CameraData &camera, Quaternion orientation) {
  camera.forward = RotateVector(DEFAULT_FORWARD, orientation);
  camera.up = RotateVector(DEFAULT_UP, orientation);
}

void Rotate(scene::CameraData &camera, Quaternion &orientation,
            Vector2 mouseDelta) {
  if (mouseDelta.x == 0.0f && mouseDelta.y == 0.0f) {
    return;
  }

  // Yaw around the camera up axis, then pitch around the current right axis.
  const Quaternion yaw = QuaternionFromAxisAngle(
      camera.up, -mouseDelta.x * MOUSE_SENSITIVITY * DEG2RAD);
  const Quaternion pitch = QuaternionFromAxisAngle(
      CameraRight(camera), -mouseDelta.y * MOUSE_SENSITIVITY * DEG2RAD);

  orientation = QuaternionNormalize(
      QuaternionMultiply(pitch, QuaternionMultiply(yaw, orientation)));
  ApplyOrientation(camera, orientation);
}

Vector3 MovementDirection(const scene::CameraData &camera) {
  const Vector3 right = CameraRight(camera);
  Vector3 direction{};

  // WASD moves on the camera plane; Q/E move vertically along camera up.
  if (IsKeyDown(KEY_W)) {
    direction = Vector3Add(direction, camera.forward);
  }
  if (IsKeyDown(KEY_S)) {
    direction = Vector3Subtract(direction, camera.forward);
  }
  if (IsKeyDown(KEY_D)) {
    direction = Vector3Add(direction, right);
  }
  if (IsKeyDown(KEY_A)) {
    direction = Vector3Subtract(direction, right);
  }
  if (IsKeyDown(KEY_E)) {
    direction = Vector3Add(direction, camera.up);
  }
  if (IsKeyDown(KEY_Q)) {
    direction = Vector3Subtract(direction, camera.up);
  }

  return direction;
}

float MoveSpeed() {
  return MOVE_SPEED *
         (IsKeyDown(KEY_LEFT_SHIFT) ? FAST_MOVE_MULTIPLIER : 1.0f) *
         GetFrameTime();
}

void Move(scene::CameraData &camera, Vector3 direction) {
  if (Vector3LengthSqr(direction) == 0.0f) {
    return;
  }

  camera.position = Vector3Add(
      camera.position, Vector3Scale(Vector3Normalize(direction), MoveSpeed()));
}

void ToggleCursorCapture() {
  if (!IsKeyPressed(KEY_ESCAPE)) {
    return;
  }

  if (IsCursorHidden()) {
    EnableCursor();
  } else {
    DisableCursor();
  }
}

} // namespace

void Initialize(scene::CameraData &camera, Vector4 &orientation) {
  orientation = InitialOrientation(camera);
  ApplyOrientation(camera, orientation);
}

void UpdateFreeCamera(scene::CameraData &camera, Vector4 &orientation) {
  ToggleCursorCapture();

  if (!IsCursorHidden()) {
    return;
  }

  Rotate(camera, orientation, GetMouseDelta());
  Move(camera, MovementDirection(camera));
}

} // namespace raytracer::app::camera
