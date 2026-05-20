#pragma once

#include "raylib.h"

#include "scene/scene.hpp"

#include <string>

class Application {
public:
  Application(int width, int height, std::string title);
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  void run();

private:
  struct ShaderLocations {
    int resolution = -1;
    int time = -1;

    int cameraPosition = -1;
    int cameraForward = -1;
    int cameraRight = -1;
    int cameraUp = -1;
    int cameraFovY = -1;

    int sphereCount = -1;
    int sphereData = -1;
    int sphereColor = -1;

    int planeCount = -1;
    int planeData = -1;
    int planeColor = -1;

    int ambientIntensity = -1;

    int dirLightCount = -1;
    int dirLightDirection = -1;
    int dirLightIntensity = -1;

    int spotlightCount = -1;
    int spotlightCosineCutoff = -1;
    int spotlightPosition = -1;
    int spotlightDirection = -1;
    int spotlightIntensity = -1;
  };

private:
  void createTestScene();
  void loadShaderLocations();
  void uploadSceneToShader();

  void update();
  void render();

private:
  int m_width = 0;
  int m_height = 0;
  std::string m_title;

  Shader m_shader{};
  ShaderLocations m_locs{};

  Scene m_scene{};
};
