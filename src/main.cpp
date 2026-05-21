#include "app/Application.hpp"

int main() {
  raytracer::app::Application app(1920, 1080, "raytracer");
  app.run();

  return 0;
}
