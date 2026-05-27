#pragma once

#include "render/RaytraceRenderer.hpp"

#include <string_view>

namespace raytracer::app::debug {

/// Default display gamma used by the debug controls.
constexpr float DEFAULT_GAMMA = 2.2f;

/// Tone mapping modes exposed by the debug overlay.
enum class ToneMapMode {
  Raw = 0,
  Aces = 1,
};

/// Light filters used to validate each light family independently.
enum class LightMode {
  All = 0,
  DirectionalOnly = 1,
  SpotlightsOnly = 2,
};

/// Runtime debug options controlled from the keyboard.
struct State {
  /// Whether the on-screen debug overlay is visible.
  bool overlayVisible = true;

  /// Active tone mapping mode.
  ToneMapMode toneMapMode = ToneMapMode::Aces;

  /// Active light-family filter.
  LightMode lightMode = LightMode::All;

  /// Display gamma used by the shader post-processing step.
  float gamma = DEFAULT_GAMMA;
};

/// Updates debug state from keyboard shortcuts.
void Update(State &state);

/// Draws the debug overlay when it is enabled.
void DrawOverlay(const State &state, std::string_view sceneName);

/// Draws the last scene-load error, if any.
void DrawSceneLoadError(std::string_view sceneName, std::string_view error);

/// Converts application debug state to the renderer-facing uniform payload.
render::RenderDebugOptions ToRenderOptions(const State &state);

} // namespace raytracer::app::debug
