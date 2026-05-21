#pragma once

#include "render/RaytraceRenderer.hpp"

namespace raytracer::app::debug {

enum class ToneMapMode {
  Raw = 0,
  Aces = 1,
};

enum class LightMode {
  All = 0,
  DirectionalOnly = 1,
  SpotlightsOnly = 2,
};

struct State {
  bool overlayVisible = true;
  ToneMapMode toneMapMode = ToneMapMode::Aces;
  LightMode lightMode = LightMode::All;
  float gamma = 2.2f;
};

void Update(State &state);
void DrawOverlay(const State &state);
render::RenderDebugOptions ToRenderOptions(const State &state);

} // namespace raytracer::app::debug
