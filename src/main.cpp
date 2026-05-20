#include "app/Application.hpp"

int main() {
  Application app(1280, 720, "raytracer");
  app.run();

  return 0;
}
