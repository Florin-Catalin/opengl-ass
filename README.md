![unmaintained](http://img.shields.io/badge/status-unmaintained-red.png)
# OpenGL-Application
A simple OpenGL 4.1 (core profile) scene for Linux: several OBJ models with translation, a day/night cycle with a moving sun and moon, a flickering campfire light, fog, a skybox and shadow mapping.

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
| F / G | Fog on / off |
| B | Campfire light on / off |
| H / J | Lamp (warm point light) on / off |
| X | Toggle wireframe |
| Arrow keys | Drive the tank (it collides with the buildings, wood pile and rocket) |
| C | Launch the rocket |
| Esc | Quit |
