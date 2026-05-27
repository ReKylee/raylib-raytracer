#include "app/Application.hpp"

namespace {

constexpr int DEFAULT_WINDOW_WIDTH = 1920;
constexpr int DEFAULT_WINDOW_HEIGHT = 1080;
constexpr const char *WINDOW_TITLE = "raytracer";

} // namespace

int main() {
  raytracer::app::Application app(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                                  WINDOW_TITLE);
  app.run();

  return 0;
}
