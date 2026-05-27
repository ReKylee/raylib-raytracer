# raylib-raytracer

A GPU-accelerated ray tracer written in C++23 and GLSL. Scenes are described
in plain text files and rendered by a fragment shader running over the
framebuffer. Built for Assignment 2 of the Shenkar Applied Computer Graphics
course.

## Features

- Sphere and plane primitives, with infinite checkerboard texturing on planes
- Phong lighting (ambient, diffuse, specular) with per-material shininess
- Three light types: global ambient, directional, and spotlight (with cone cutoff)
- Hard shadows from every light source
- Per-material reflections with bounded bounce depth
- ACES tone mapping with adjustable gamma
- Free-fly debug camera (WASD + mouse)
- Multiple scene files discovered automatically and cycled at runtime
- Friendly handling of malformed scene files — an on-screen banner reports the
  error instead of crashing the app

## Build

Requirements: CMake 3.15+ and Clang. raylib 6.0 is fetched automatically by
CMake (no manual install needed).

```bash
cmake -B build
cmake --build build
```

The build also copies `assets/` and `Scenes/` into the runtime directory next
to the produced executable.

## Run

```powershell
.\bin\raytracer.exe          # Windows (PowerShell)
```

```bash
./bin/raytracer              # Linux / macOS
```

The first scene in `Scenes/` (alphabetical order) is loaded at startup.

## Controls

| Key                   | Action                                              |
| --------------------- | --------------------------------------------------- |
| `Space`               | Load the next scene file in `Scenes/`               |
| `W` / `A` / `S` / `D` | Move forward / left / back / right                  |
| `Q` / `E`             | Move down / up                                      |
| `Shift`               | Move faster (3× speed)                              |
| Mouse                 | Look around                                         |
| `Esc`                 | Toggle mouse capture                                |
| `T`                   | Toggle tone mapping (ACES / Raw)                    |
| `L`                   | Cycle light filter (All / Directional / Spotlights) |
| `+` / `-`             | Increase / decrease gamma                           |
| `F1`                  | Toggle debug overlay                                |
| `F10`                 | Quit                                                |

## Scene file format

Each scene is a plain text file. Every meaningful line is
`<tag> v1 v2 v3 v4`:

| Tag | Meaning                                                                                                   |
| --- | --------------------------------------------------------------------------------------------------------- |
| `e` | Camera eye position `x y z` (4th coord ignored)                                                           |
| `a` | Global ambient intensity `r g b` (4th coord ignored)                                                      |
| `o` | Object — sphere if `w > 0` (center + radius `w`), plane if `w <= 0` (un-normalized normal + offset `w`)   |
| `c` | Material `r g b shininess`, pairs with the i-th `o` line                                                  |
| `d` | Light direction `x y z w`, where `w == 0` is directional and `w == 1` is spotlight                        |
| `p` | Spotlight `x y z cos(cutoff)`, pairs with the i-th *spotlight* `d` line                                   |
| `i` | Light intensity `r g b` (4th coord ignored), pairs with the i-th `d` line                                 |

Lines must appear in the order `e, a, o*, c*, d*, p*, i*`. Blank lines are
tolerated; anything else is a parse error.

Example:

```text
e 0.0 0.0 4.0 1.0
a 0.1 0.2 0.3 1.0
o 0.0 -0.5 -1.0 -3.5
o -0.7 -0.7 -2.0 0.5
o 0.6 -0.5 -1.0 0.5
c 0.0 1.0 1.0 10.0
c 1.0 0.0 0.0 10.0
c 0.6 0.0 0.8 10.0
d 0.5 0.0 -1.0 1.0
d 0.0 0.5 -1.0 0.0
p 2.0 1.0 3.0 0.6
i 0.2 0.5 0.7 1.0
i 0.7 0.5 0.0 1.0
```

The `Scenes/` folder ships with:

- `scene1.txt` … `scene5.txt` — original course scenes, each paired with a
  reference PNG.
- `scene_*.txt` — additional valid edge cases (minimal scene, no lights, no
  objects, only spotlights, mixed whitespace).
- `z_bad_*.txt` — intentionally malformed files for testing parser error
  handling. Cycling into one shows the in-app error banner with the specific
  parse failure.

## Project structure

```text
src/
  main.cpp                      Program entry point
  app/
    Application.{hpp,cpp}       Window, render loop, input dispatch
    SceneLibrary.{hpp,cpp}      Scene discovery, cycling, load-error state
    CameraControls.{hpp,cpp}    Free-fly camera (WASD + mouse)
    DebugControls.{hpp,cpp}     On-screen overlay, tone-mapping/gamma toggles
  scene/
    scene.hpp                   CPU-side scene data structures
    SceneParser.{hpp,cpp}       Text-file → Scene parser
  render/
    RaytraceRenderer.{hpp,cpp}  Per-frame shader uniform uploads + draw call
    ShaderUniforms.{hpp,cpp}    Uniform location lookup helpers
assets/shaders/
  raytrace.fs                   Fragment shader: ray casting, intersection,
                                Phong lighting, reflections, tone mapping
Scenes/                         Scene files (.txt) and reference renders (.png)
CMakeLists.txt                  Build configuration (fetches raylib via FetchContent)
```
