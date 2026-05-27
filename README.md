# raylib-raytracer

<!--toc:start-->

- [raylib-raytracer](#raylib-raytracer)
  - [Features](#features)
  - [Build](#build)
  - [Run](#run)
  - [Controls](#controls)
  - [Scene file format](#scene-file-format)
  <!--toc:end-->

A small GPU ray tracer written in C++23 with raylib and GLSL. Scene data comes
from simple text files, and the actual ray tracing work happens in a fragment
shader over the framebuffer.

This was built for Assignment 2 in the Shenkar Applied Computer Graphics
course.

## Features

- Sphere and plane primitives
- Infinite checkerboard texturing on planes
- Phong lighting (ambient, diffuse, specular) with per-material shininess
- Global ambient, directional, and spotlight lights
- Spotlights with distance attenuation and smooth cone falloff
- Hard shadows from every light source
- Per-material reflections with a bounded bounce depth
- Four-sample anti-aliasing for smoother edges
- ACES tone mapping with adjustable gamma
- Free-fly debug camera
- Automatic scene discovery from `Scenes/`
- Basic parse-error handling for malformed scene files

## Build

Requirements:

- CMake 3.15+
- Clang

raylib 6.0 is fetched by CMake, so it does not need to be installed separately.

```bash
cmake -B build
cmake --build build
```

The build copies `assets/` and `Scenes/` next to the executable.

## Run

Windows:

```powershell
.\bin\raytracer.exe
```

Linux / macOS:

```bash
./bin/raytracer
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

Scenes are plain text files. Each meaningful line has this shape:

```text
<tag> v1 v2 v3 v4
```

| Tag | Meaning                                                                                                 |
| --- | ------------------------------------------------------------------------------------------------------- |
| `e` | Camera eye position `x y z` (4th coord ignored)                                                         |
| `a` | Global ambient intensity `r g b` (4th coord ignored)                                                    |
| `o` | Object - sphere if `w > 0` (center + radius `w`), plane if `w <= 0` (un-normalized normal + offset `w`) |
| `c` | Material `r g b shininess`, pairs with the i-th `o` line                                                |
| `d` | Light direction `x y z w`, where `w == 0` is directional and `w == 1` is spotlight                      |
| `p` | Spotlight `x y z cos(cutoff)`, pairs with the i-th _spotlight_ `d` line                                 |
| `i` | Light intensity `r g b` (4th coord ignored), pairs with the i-th `d` line                               |

Lines must appear in the order `e, a, o*, c*, d*, p*, i*`. Blank lines are
allowed. Anything else is treated as a parse error and shown in the app instead
of crashing.

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

- `scene1.txt` through `scene5.txt`: original course scenes, each with a
  reference PNG
- `scene_*.txt`: extra valid test scenes, including minimal scenes, empty object
  or light lists, spotlight-only lighting, and mixed whitespace
- `z_test_invalid_*.txt`: intentionally invalid scenes used to test parser
  errors
