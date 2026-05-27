# SceneParser — Plan

Goal: replace the hardcoded `Application::createTestScene()` with `SceneParser` that
loads any `Scenes/*.txt` file into a `scene::Scene`.

## What exists already

- `src/scene/scene.hpp` — target data structures (`Scene`, `Sphere`, `Plane`,
  `AmbientLight`, `DirectionalLight`, `Spotlight`, `CameraData`). Parser output type.
- `src/app/Application.cpp::createTestScene()` — the hardcoded scene we will replace.
- `src/sceneParser/SceneParser.{hpp,cpp}` — empty, to be filled in.
- `Scenes/scene1..5.txt` — input examples (+ reference PNGs).

## File format recap (from Assignment_2.pdf)

Each line: `<tag> v1 v2 v3 v4`.

- `e x y z _`   → camera eye position (4th ignored).
- `a r g b _`   → global ambient intensity (4th ignored).
- `o x y z w`   → object. `w > 0` → Sphere(center=xyz, radius=w).
  `w <= 0` → Plane(normal=xyz, d=w).
- `c r g b n`   → matches the i-th `o`. Material color + shininess.
- `d x y z w`   → light direction. `w == 0` directional, `w == 1` spotlight.
- `p x y z cos` → matches the i-th *spotlight* `d`. Position + cosine cutoff.
- `i r g b _`   → matches the i-th `d` (directional or spotlight).

## Decisions

- **Error handling.** Throw on hard errors (file missing, mandatory tag absent,
  count mismatch, malformed number, out-of-range value). Silently skip blank
  lines, unknown tags, and trailing junk (scene2.txt has some).
- **Strict order.** We assume `e, a, o*, c*, d*, p*, i*` — matches all 5
  provided scenes and the assignment example. Out-of-order files = throw.
- **Camera derivation lives in the parser.** It builds full `CameraData` from
  `e` alone using the assignment's fixed-screen convention. `Application` stays
  dumb.
- **Number format.** Any decimal/integer literal — `std::stof` handles `"0"`,
  `"0.0"`, `"-3.5"`, `"1e-2"` uniformly. No special handling.
- **Scene selection UX.** At startup, scan `Scenes/*.txt` into a sorted list.
  Render index 0. **Space** advances to the next scene (wraps around). Re-uses
  existing `RaytraceRenderer::uploadScene`.
- **Shader caps.** `MAX_SPHERES=16 / MAX_PLANES=16 / MAX_LIGHTS=8` — fits all
  provided scenes (scene2 is the largest: 7 objects, 4 directionals, 3 spots).
  Leave as-is.

## Steps

1. **API.** Single entry point `parseScene(path) -> Scene`. Throws on hard errors.
1. **Lexer + expectation helpers.** Common primitives so every block uses the
   same logic:

   - `readOne(stream, tag) -> Vec4` — skip blanks/junk, read one line, error if
     tag mismatch or token count != 4.
   - `readMany(stream, tag) -> Vec4[]` — zero or more consecutive `tag` lines.
     Peek + putback at first non-match.
   - `readExactly(stream, tag, n) -> Vec4[]` — exactly `n` lines of `tag`.
     Throws on early non-match or wrong count.
1. **Parse** (using the helpers, in this order):

   ```text
   e_vec    = readOne(s, 'e')
   a_vec    = readOne(s, 'a')
   o_vecs   = readMany(s, 'o')
   c_vecs   = readExactly(s, 'c', count(o_vecs))
   d_vecs   = readMany(s, 'd')
   spotN    = count(d in d_vecs where d.w == 1)
   p_vecs   = readExactly(s, 'p', spotN)
   i_vecs   = readExactly(s, 'i', count(d_vecs))
   ```
1. **Assemble `Scene`:**

   - `scene.camera = cameraFromEye(e_vec.xyz)` — see derivation below.
   - `scene.ambient.intensity = a_vec.xyz`.
   - Zip `o_vecs`/`c_vecs`: `obj.w > 0` → Sphere(pos=obj.xyz, r=obj.w,
     color=col.xyz, shininess=col.w). Else → Plane(normal=obj.xyz,
     offset=obj.w, color=col.xyz, shininess=col.w).
   - Zip `d_vecs`/`i_vecs`, with a separate spotlight cursor into `p_vecs`:
       - `dir.w == 0` → DirectionalLight(direction=dir.xyz, intensity=i.xyz).
       - `dir.w == 1` → Spotlight(position=p_vecs[spotIdx].xyz,
         direction=dir.xyz, intensity=i.xyz,
         cosineCutoff=p_vecs[spotIdx].w). Advance `spotIdx`.
1. **Wire into `Application`.**

   - At construction: scan `Scenes/*.txt` into `std::vector<std::string>` of
     paths (sorted alphabetically so scene1.txt comes first).
   - Replace `createTestScene()` with `loadScene(0)`.
   - `loadScene(i)`:
     `m_scene = SceneParser::parseScene(paths[i]);`
     `m_renderer->uploadScene(m_scene);`
     `camera::Initialize(m_scene.camera, m_cameraOrientation);`
   - In `update()`: if `IsKeyPressed(KEY_SPACE)`,
     `loadScene((current + 1) % paths.size())`.
1. **Validate** by rendering scene1..5 and eyeball-comparing to the matching
   `Scenes/sceneN.png` reference images.

## Validation rules (per tag)

- All tags: exactly 4 numeric tokens.
- `e`, `a`, `i`: 4th coord ignored (assignment says so — don't validate value).
- `o`: 4th coord is the sphere/plane discriminator (`> 0` sphere, `<= 0` plane).
- `c`: 4th coord is shininess.
- `d`: 4th coord must be exactly `0.0` (directional) or `1.0` (spotlight).
  Else throw.
- `p`: 4th coord is `cos(cutoff)`. Must be in [-1, 1]. Else throw.

## cameraFromEye derivation

Assignment convention: screen is the [-1, 1] × [-1, 1] square at z=0. Camera
looks at the origin. All 5 provided scenes have eye on the z-axis (`e 0 0 z`),
so:

- `forward = normalize(-eye)`  (= `(0, 0, -1)` for any z-axis eye)
- `up      = (0, 1, 0)`
- `fovY    = 2 * atan(1 / |eye|)`  (convert to degrees for `CameraData.fovY`)

For non-axis eyes the formula generalizes (`forward = normalize(-eye)`, same
`up`), but `fovY` alone no longer captures the geometry exactly. Out of scope
for the provided scenes — revisit only if a test scene breaks it.

---

look in @Scenes/  : 
`scene1.txt` is the txt input, `sceneA.png` is the *expected* result, `sceneARealOutput.png` is a screenshot of the *actual result*.

Let's go over each and see what we have right and wrong:

*note: the reflections are there because we did extra work.

- scene 1:
    - aside from no spotlight, looks fine.
- scene 2:
    - In this case, when looking carefully at `scene2.txt`, it looks like our output is good, and `scene2.png` got it wrong
    - but again, missing spotlights
- scene 3:
    - aside from no spotlight, looks fine.
- scene 4:
    - aside from no spotlight, looks fine.
- scene 5:
    - In this case, when looking carefully at `scene5.txt`, it looks like our output is good, and `scene5.png` got it wrong
    - but again, missing spotlights

