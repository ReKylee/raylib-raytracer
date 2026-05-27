#include "app/SceneLibrary.hpp"

#include "scene/SceneParser.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace raytracer::app {

SceneLibrary::SceneLibrary(std::filesystem::path scenesDirectory)
    : m_scenesDirectory(std::move(scenesDirectory)) {
  scan();
  load(0);
}

bool SceneLibrary::load(std::size_t index) {
  m_currentScene = index % m_scenePaths.size();

  try {
    m_scene = scene::ParseScene(m_scenePaths[m_currentScene].string());
    m_loadError.clear();
    return true;
  } catch (const std::exception &error) {
    m_loadError = error.what();
    return false;
  }
}

bool SceneLibrary::loadNext() { return load(m_currentScene + 1); }

std::string SceneLibrary::currentSceneName() const {
  return m_scenePaths[m_currentScene].filename().string();
}

void SceneLibrary::scan() {
  if (!std::filesystem::is_directory(m_scenesDirectory)) {
    throw std::runtime_error("Application: scenes directory not found: " +
                             m_scenesDirectory.string());
  }

  m_scenePaths.clear();
  for (const auto &entry :
       std::filesystem::directory_iterator{m_scenesDirectory}) {
    if (entry.is_regular_file() && entry.path().extension() == ".txt") {
      m_scenePaths.push_back(entry.path());
    }
  }

  std::ranges::sort(m_scenePaths);

  if (m_scenePaths.empty()) {
    throw std::runtime_error("Application: no .txt scene files in: " +
                             m_scenesDirectory.string());
  }
}

} // namespace raytracer::app
