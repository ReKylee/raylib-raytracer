#include "app/DebugControls.hpp"
namespace {

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

} // namespace

namespace raytracer::app::debug {

void Update(State &state) {
  using std::max;
  using std::min;
  constexpr float GAMMA_STEP = 0.1f;
  constexpr float MIN_GAMMA = 0.5f;
  constexpr float MAX_GAMMA = 4.0f;
  if (IsKeyPressed(KEY_F1)) {
    state.overlayVisible = !state.overlayVisible;
  }

  if (IsKeyPressed(KEY_T)) {
    state.toneMapMode = NextToneMapMode(state.toneMapMode);
  }

  if (IsKeyPressed(KEY_L)) {
    state.lightMode = NextLightMode(state.lightMode);
  }
  if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
    state.gamma = min(MAX_GAMMA, state.gamma + GAMMA_STEP);
  }

  if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
    state.gamma = max(MIN_GAMMA, state.gamma - GAMMA_STEP);
  }
}

void DrawOverlay(const State &state) {
  if (!state.overlayVisible) {
    return;
  }

  constexpr int BASE_PANEL_MARGIN = 14;
  constexpr int BASE_PANEL_TOP = 40;
  constexpr int BASE_PANEL_WIDTH = 500;
  constexpr int BASE_PANEL_PADDING = 14;

  constexpr int BASE_HEADER_HEIGHT = 30;
  constexpr int BASE_LINE_HEIGHT = 28;
  constexpr int BASE_FONT_SIZE = 18;
  constexpr int BASE_TITLE_FONT_SIZE = 18;

  constexpr int BODY_LINES = 5;

  const int screenWidth = GetScreenWidth();
  const int screenHeight = GetScreenHeight();

  const auto clamp = [](float value, float minValue, float maxValue) {
    return value < minValue ? minValue : value > maxValue ? maxValue : value;
  };

  const float uiScale =
      clamp(static_cast<float>(screenHeight) / 1080.0f, 0.9f, 1.25f);

  const auto s = [uiScale](int value) {
    return static_cast<int>(static_cast<float>(value) * uiScale + 0.5f);
  };

  const int margin = s(BASE_PANEL_MARGIN);
  const int panelX = margin;
  const int panelY = s(BASE_PANEL_TOP);
  const int panelWidth =
      std::min(s(BASE_PANEL_WIDTH), screenWidth - margin * 2);

  const int padding = s(BASE_PANEL_PADDING);
  const int headerHeight = s(BASE_HEADER_HEIGHT);
  const int lineHeight = s(BASE_LINE_HEIGHT);
  const int fontSize = s(BASE_FONT_SIZE);
  const int titleFontSize = s(BASE_TITLE_FONT_SIZE);

  const int panelHeight =
      padding * 2 + headerHeight + BODY_LINES * lineHeight + s(4);

  const bool compact = panelWidth < s(430);

  const int labelX = panelX + padding;
  const int valueX = labelX + (compact ? s(92) : s(120));
  const int keyX = panelX + panelWidth - (compact ? s(70) : s(86));

  const Color bgColor = Color{6, 8, 12, 220};
  const Color headerColor = Color{28, 34, 48, 240};
  const Color borderColor = Color{255, 255, 255, 85};
  const Color accentColor = Color{120, 170, 255, 235};

  const Color titleColor = Color{230, 235, 255, 255};
  const Color labelColor = Color{150, 160, 175, 255};
  const Color keyColor = Color{125, 180, 255, 255};
  const Color helpColor = Color{180, 185, 195, 255};

  const auto drawText = [fontSize](int x, int y, const char *text,
                                   Color color) {
    DrawText(text, x, y, fontSize, color);
  };

  const auto drawRow = [&](int row, const char *label, const char *value,
                           const char *key = nullptr,
                           Color rowValueColor = Color{235, 238, 245, 255}) {
    const int y = panelY + headerHeight + padding + row * lineHeight;

    drawText(labelX, y, label, labelColor);
    drawText(valueX, y, value, rowValueColor);

    if (key != nullptr) {
      drawText(keyX, y, key, keyColor);
    }
  };

  DrawRectangle(panelX, panelY, panelWidth, panelHeight, bgColor);
  DrawRectangle(panelX, panelY, panelWidth, headerHeight, headerColor);
  DrawRectangleLines(panelX, panelY, panelWidth, panelHeight, borderColor);
  DrawRectangle(panelX, panelY, panelWidth, s(2), accentColor);

  DrawText("DEBUG", labelX, panelY + s(6), titleFontSize, titleColor);

  drawRow(0, "Tone map", ToneMapModeName(state.toneMapMode), "[T]");
  drawRow(1, "Lights", LightModeName(state.lightMode), "[L]");
  drawRow(2, "Gamma", TextFormat("%.1f", state.gamma), "[+/-]");

  drawRow(3, "Camera", compact ? "WASD + mouse" : "Mouse + WASD, Q/E, Shift",
          nullptr, helpColor);

  drawRow(4, "System",
          compact ? "F1  Esc  F10" : "F1 overlay  Esc cursor  F10 exit",
          nullptr, helpColor);
}

render::RenderDebugOptions ToRenderOptions(const State &state) {
  return {
      .toneMapMode = static_cast<int>(state.toneMapMode),
      .lightMode = static_cast<int>(state.lightMode),
      .gamma = state.gamma,
  };
}

} // namespace raytracer::app::debug
