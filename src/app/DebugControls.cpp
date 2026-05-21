#include "app/DebugControls.hpp"

namespace {

constexpr int PANEL_X = 10;
constexpr int PANEL_Y = 36;
constexpr int PANEL_WIDTH = 390;
constexpr int PANEL_HEIGHT = 116;
constexpr int TEXT_X = PANEL_X + 12;
constexpr int TEXT_Y = PANEL_Y + 10;
constexpr int LINE_HEIGHT = 22;
constexpr int FONT_SIZE = 16;

const char *ToneMapModeName(raytracer::app::debug::ToneMapMode mode) {
  switch (mode) {
  case raytracer::app::debug::ToneMapMode::Raw:
    return "Raw";
  case raytracer::app::debug::ToneMapMode::Aces:
    return "ACES";
  }

  return "Unknown";
}

const char *LightModeName(raytracer::app::debug::LightMode mode) {
  switch (mode) {
  case raytracer::app::debug::LightMode::All:
    return "All lights";
  case raytracer::app::debug::LightMode::DirectionalOnly:
    return "Directional only";
  case raytracer::app::debug::LightMode::SpotlightsOnly:
    return "Spotlights only";
  }

  return "Unknown";
}

raytracer::app::debug::ToneMapMode
NextToneMapMode(raytracer::app::debug::ToneMapMode mode) {
  return mode == raytracer::app::debug::ToneMapMode::Raw
             ? raytracer::app::debug::ToneMapMode::Aces
             : raytracer::app::debug::ToneMapMode::Raw;
}

raytracer::app::debug::LightMode
NextLightMode(raytracer::app::debug::LightMode mode) {
  switch (mode) {
  case raytracer::app::debug::LightMode::All:
    return raytracer::app::debug::LightMode::DirectionalOnly;
  case raytracer::app::debug::LightMode::DirectionalOnly:
    return raytracer::app::debug::LightMode::SpotlightsOnly;
  case raytracer::app::debug::LightMode::SpotlightsOnly:
    return raytracer::app::debug::LightMode::All;
  }

  return raytracer::app::debug::LightMode::All;
}

void DrawLine(int lineIndex, const char *text) {
  DrawText(text, TEXT_X, TEXT_Y + lineIndex * LINE_HEIGHT, FONT_SIZE, RAYWHITE);
}

} // namespace

namespace raytracer::app::debug {

void Update(State &state) {
  if (IsKeyPressed(KEY_F1)) {
    state.overlayVisible = !state.overlayVisible;
  }

  if (IsKeyPressed(KEY_T)) {
    state.toneMapMode = NextToneMapMode(state.toneMapMode);
  }

  if (IsKeyPressed(KEY_L)) {
    state.lightMode = NextLightMode(state.lightMode);
  }
}

void DrawOverlay(const State &state) {
  if (!state.overlayVisible) {
    return;
  }

  DrawRectangle(PANEL_X, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT,
                Color{0, 0, 0, 170});
  DrawRectangleLines(PANEL_X, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT,
                     Color{255, 255, 255, 70});

  DrawLine(0,
           TextFormat("Tone map: %s [T]", ToneMapModeName(state.toneMapMode)));
  DrawLine(1, TextFormat("Lights: %s [L]", LightModeName(state.lightMode)));
  DrawLine(2, "Camera: mouse + WASD, Q/E vertical, Shift fast");
  DrawLine(3, "Esc: cursor capture   F1: overlay   F12: exit");
}

render::RenderDebugOptions ToRenderOptions(const State &state) {
  return {
      .toneMapMode = static_cast<int>(state.toneMapMode),
      .lightMode = static_cast<int>(state.lightMode),
  };
}

} // namespace raytracer::app::debug
