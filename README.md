![unmaintained](http://img.shields.io/badge/status-unmaintained-red.png)
# OpenGL-Application
A simple OpenGL 4.1 (core profile) scene for Linux: several OBJ models with translation, a day/night cycle with a moving sun and a low-poly moon, twinkling stars and the naked-eye planets (Venus, Mars, Jupiter, Saturn), drifting clouds, a flickering campfire light, fog, a skybox and shadow mapping.

![Preview](UngureanuFlorin-Catalin_30434.png)

## Build

Dependencies (Debian/Ubuntu):

```
sudo apt install build-essential cmake libglfw3-dev libglew-dev libgl-dev
```

GLM is vendored in `third_party/glm`.

```
cmake -S . -B build
cmake --build build
```

## Run

Assets are loaded with relative paths, so run from the repository root:

```
cmake --build build --target run
# or
./build/opengl_scene
```

## Controls

| Key | Action |
|---|---|
| W / A / S / D + mouse | Move / look around |
| Q / E | Rotate the scene |
| K / L | Rotate the sun/moon path |
| N | Pause / resume the day/night cycle |
| T | Skip ahead 3 hours |
| P | Skip ahead 10 days (watch the planets move) |
| [ / ] | Fewer / more clouds |
| F / G | Fog on / off |
| B | Campfire light on / off |
| H / J | Lamp (warm point light) on / off |
| X | Toggle wireframe |
| Arrow keys | Drive the tank (it collides with the buildings, wood pile and rocket) |
| C | Launch the rocket |
| Esc | Quit |

## Adding models

The loader only reads OBJ + MTL. To convert a `.glb` (glTF binary) model:

```
python3 -I tools/glb2obj.py path/to/model.glb objects/<name> <name>
```

Materials without textures get small solid-color PNGs, because the shader always samples a diffuse texture.

## Credits

- "Low Poly Moon" by [FoxVenture](https://sketchfab.com/FoxVenture), [Sketchfab](https://sketchfab.com/3d-models/low-poly-moon-d671eb35d2244c8cb98d06b4948d9937), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/moon/`.
