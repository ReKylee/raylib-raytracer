#pragma once

#include "scene/scene.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace raytracer::app {

/// Discovers scene files and owns scene-switching/load-error state.
class SceneLibrary {
public:
  explicit SceneLibrary(std::filesystem::path scenesDirectory);

  [[nodiscard]] scene::Scene &scene() { return m_scene; }
  [[nodiscard]] const scene::Scene &scene() const { return m_scene; }
  [[nodiscard]] std::size_t currentIndex() const { return m_currentScene; }
  [[nodiscard]] std::size_t sceneCount() const { return m_scenePaths.size(); }
  [[nodiscard]] std::string currentSceneName() const;
  [[nodiscard]] const std::string &loadError() const { return m_loadError; }

  bool load(std::size_t index);
  bool loadNext();

private:
  void scan();

private:
  std::filesystem::path m_scenesDirectory;
  std::vector<std::filesystem::path> m_scenePaths;
  scene::Scene m_scene{};
  std::size_t m_currentScene = 0;
  std::string m_loadError;
};

} // namespace raytracer::app
