#include "app/DebugControls.hpp"

namespace raytracer::app::debug {
namespace {

constexpr float GAMMA_STEP = 0.1f;
constexpr float MIN_GAMMA = 0.5f;
constexpr float MAX_GAMMA = 4.0f;

constexpr int BASE_PANEL_MARGIN = 14;
constexpr int BASE_PANEL_TOP = 40;
constexpr int BASE_PANEL_WIDTH = 500;
constexpr int BASE_PANEL_PADDING = 14;

constexpr int BASE_FPS_LEFT = 10;
constexpr int BASE_FPS_TOP = 10;
constexpr int FPS_BACKING_WIDTH = 88;
constexpr int FPS_BACKING_HEIGHT = 26;

constexpr int BASE_HEADER_HEIGHT = 30;
constexpr int BASE_LINE_HEIGHT = 28;
constexpr int BASE_FONT_SIZE = 18;
constexpr int BASE_TITLE_FONT_SIZE = 18;
constexpr int BODY_LINES = 5;

constexpr float REFERENCE_SCREEN_HEIGHT = 1080.0f;
constexpr float MIN_UI_SCALE = 0.9f;
constexpr float MAX_UI_SCALE = 1.25f;
constexpr float SCALE_ROUNDING_OFFSET = 0.5f;

constexpr int PANEL_EXTRA_HEIGHT = 4;
constexpr int COMPACT_PANEL_WIDTH = 430;
constexpr int COMPACT_VALUE_COLUMN_OFFSET = 92;
constexpr int NORMAL_VALUE_COLUMN_OFFSET = 120;
constexpr int COMPACT_KEY_COLUMN_MARGIN = 70;
constexpr int NORMAL_KEY_COLUMN_MARGIN = 86;
constexpr int ACCENT_BAR_HEIGHT = 2;
constexpr int TITLE_TEXT_TOP = 6;

constexpr int TONE_MAP_ROW = 0;
constexpr int LIGHTS_ROW = 1;
constexpr int GAMMA_ROW = 2;
constexpr int CAMERA_ROW = 3;
constexpr int SYSTEM_ROW = 4;

constexpr Color BG_COLOR{6, 8, 12, 220};
constexpr Color HEADER_COLOR{28, 34, 48, 240};
constexpr Color BORDER_COLOR{255, 255, 255, 85};
constexpr Color ACCENT_COLOR{120, 170, 255, 235};

constexpr Color TITLE_COLOR{230, 235, 255, 255};
constexpr Color LABEL_COLOR{150, 160, 175, 255};
constexpr Color KEY_COLOR{125, 180, 255, 255};
constexpr Color HELP_COLOR{180, 185, 195, 255};
constexpr Color VALUE_COLOR{235, 238, 245, 255};

float UiScale(int screenHeight) {
  const auto clamp = [](float value, float minValue, float maxValue) {
    return value < minValue ? minValue : value > maxValue ? maxValue : value;
  };

  return clamp(static_cast<float>(screenHeight) / REFERENCE_SCREEN_HEIGHT,
               MIN_UI_SCALE, MAX_UI_SCALE);
}

int Scale(int value, float uiScale) {
  return static_cast<int>(static_cast<float>(value) * uiScale +
                          SCALE_ROUNDING_OFFSET);
}

const char *ToneMapModeName(ToneMapMode mode) {
  switch (mode) {
  case ToneMapMode::Raw:
    return "Raw";
  case ToneMapMode::Aces:
    return "ACES";
  }

  return "Unknown";
}

const char *LightModeName(LightMode mode) {
  switch (mode) {
  case LightMode::All:
    return "All lights";
  case LightMode::DirectionalOnly:
    return "Directional only";
  case LightMode::SpotlightsOnly:
    return "Spotlights only";
  }

  return "Unknown";
}

ToneMapMode NextToneMapMode(ToneMapMode mode) {
  return mode == ToneMapMode::Raw ? ToneMapMode::Aces : ToneMapMode::Raw;
}

LightMode NextLightMode(LightMode mode) {
  switch (mode) {
  case LightMode::All:
    return LightMode::DirectionalOnly;
  case LightMode::DirectionalOnly:
    return LightMode::SpotlightsOnly;
  case LightMode::SpotlightsOnly:
    return LightMode::All;
  }

  return LightMode::All;
}

void DrawFpsCounter(float uiScale) {
  const int x = Scale(BASE_FPS_LEFT, uiScale);
  const int y = Scale(BASE_FPS_TOP, uiScale);
  const int width = Scale(FPS_BACKING_WIDTH, uiScale);
  const int height = Scale(FPS_BACKING_HEIGHT, uiScale);

  DrawRectangle(x, y, width, height, BG_COLOR);
  DrawRectangleLines(x, y, width, height, BORDER_COLOR);
  DrawFPS(x + 5, y + 5);
}

} // namespace

void Update(State &state) {
  using std::max;
  using std::min;

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
  const int screenWidth = GetScreenWidth();
  const int screenHeight = GetScreenHeight();
  const float uiScale = UiScale(screenHeight);

  DrawFpsCounter(uiScale);

  if (!state.overlayVisible) {
    return;
  }

  // Base dimensions are authored for 1080p and scaled within a narrow range so
  // the overlay remains readable without taking over small windows.
  const int margin = Scale(BASE_PANEL_MARGIN, uiScale);
  const int panelX = margin;
  const int panelY = Scale(BASE_PANEL_TOP, uiScale);
  const int panelWidth = std::min(Scale(BASE_PANEL_WIDTH, uiScale),
                                  screenWidth - margin * 2);

  const int padding = Scale(BASE_PANEL_PADDING, uiScale);
  const int headerHeight = Scale(BASE_HEADER_HEIGHT, uiScale);
  const int lineHeight = Scale(BASE_LINE_HEIGHT, uiScale);
  const int fontSize = Scale(BASE_FONT_SIZE, uiScale);
  const int titleFontSize = Scale(BASE_TITLE_FONT_SIZE, uiScale);

  const int panelHeight =
      padding * 2 + headerHeight + BODY_LINES * lineHeight +
      Scale(PANEL_EXTRA_HEIGHT, uiScale);

  const bool compact = panelWidth < Scale(COMPACT_PANEL_WIDTH, uiScale);

  // Column positions collapse on narrow windows while preserving the same row
  // layout and shortcut alignment.
  const int labelX = panelX + padding;
  const int valueX =
      labelX + (compact ? Scale(COMPACT_VALUE_COLUMN_OFFSET, uiScale)
                        : Scale(NORMAL_VALUE_COLUMN_OFFSET, uiScale));
  const int keyX = panelX + panelWidth -
                   (compact ? Scale(COMPACT_KEY_COLUMN_MARGIN, uiScale)
                            : Scale(NORMAL_KEY_COLUMN_MARGIN, uiScale));

  const auto drawText = [fontSize](int x, int y, const char *text,
                                   Color color) {
    DrawText(text, x, y, fontSize, color);
  };

  const auto drawRow = [&](int row, const char *label, const char *value,
                           const char *key = nullptr,
                           Color rowValueColor = VALUE_COLOR) {
    const int y = panelY + headerHeight + padding + row * lineHeight;

    drawText(labelX, y, label, LABEL_COLOR);
    drawText(valueX, y, value, rowValueColor);

    if (key != nullptr) {
      drawText(keyX, y, key, KEY_COLOR);
    }
  };

  DrawRectangle(panelX, panelY, panelWidth, panelHeight, BG_COLOR);
  DrawRectangle(panelX, panelY, panelWidth, headerHeight, HEADER_COLOR);
  DrawRectangleLines(panelX, panelY, panelWidth, panelHeight, BORDER_COLOR);
  DrawRectangle(panelX, panelY, panelWidth,
                Scale(ACCENT_BAR_HEIGHT, uiScale), ACCENT_COLOR);

  DrawText("DEBUG", labelX, panelY + Scale(TITLE_TEXT_TOP, uiScale),
           titleFontSize, TITLE_COLOR);

  drawRow(TONE_MAP_ROW, "Tone map", ToneMapModeName(state.toneMapMode), "[T]");
  drawRow(LIGHTS_ROW, "Lights", LightModeName(state.lightMode), "[L]");
  drawRow(GAMMA_ROW, "Gamma", TextFormat("%.1f", state.gamma), "[+/-]");

  drawRow(CAMERA_ROW, "Camera",
          compact ? "WASD + mouse" : "Mouse + WASD, Q/E, Shift", nullptr,
          HELP_COLOR);

  drawRow(SYSTEM_ROW, "System",
          compact ? "F1  Esc  F10" : "F1 overlay  Esc cursor  F10 exit",
          nullptr, HELP_COLOR);
}

render::RenderDebugOptions ToRenderOptions(const State &state) {
  return {
      .toneMapMode = static_cast<int>(state.toneMapMode),
      .lightMode = static_cast<int>(state.lightMode),
      .gamma = state.gamma,
  };
}

} // namespace raytracer::app::debug
