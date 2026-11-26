![unmaintained](http://img.shields.io/badge/status-unmaintained-red.png)
# OpenGL-Application
A simple OpenGL 4.1 (core profile) scene for Linux: several OBJ models with translation (including a camel in the camp, a rooster wandering around it and a giant sandworm rising out of the desert beyond the canyon), a dragon hovering over the canyon, a black hole hanging in the sky, a day/night cycle with a moving sun and a low-poly moon, twinkling stars and the naked-eye planets (Venus, Mars, Jupiter, Saturn), drifting clouds that cast shadows, rain with lightning (which puts out the campfire), a rainbow afterwards and puddles that reflect the camp and the sky, snow that settles on the ground and melts (with the northern lights on clear nights over full snow), sandstorms, a tornado, shooting stars, fireflies at night, a flickering campfire with flames and rising sparks, fog, a skybox and shadow mapping.

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
| R | Start / stop rain (lightning strikes on its own in heavy rain; stop it with a low sun for a rainbow) |
| V | Start / stop snow (it settles on upward-facing surfaces and melts afterwards) |
| U | Start / stop a sandstorm |
| O | Tornado: touches down, wanders through the canyon (the camera shakes when it is close); O again lifts it |
| Z | Lightning strike now (while raining) |
| M | Shooting star (about 1 in 8 is a green fireball) |
| F / G | Fog on / off |
| B | Campfire light on / off |
| H / J | Lamp (warm point light) on / off |
| X | Toggle wireframe |
| Arrow keys | Drive the tank (it collides with the buildings, wood pile, rocket and camel) |
| C | Launch the rocket |
| Esc | Quit |

## Adding models

The loader only reads OBJ + MTL. To convert a `.glb` (glTF binary) model:

```
python3 -I tools/glb2obj.py path/to/model.glb objects/<name> <name>
```

Embedded base color textures (PNG/JPEG) are extracted next to the OBJ. Materials without textures get small solid-color PNGs, because the shader always samples a diffuse texture.

## Credits

- "Low Poly Moon" by [FoxVenture](https://sketchfab.com/FoxVenture), [Sketchfab](https://sketchfab.com/3d-models/low-poly-moon-d671eb35d2244c8cb98d06b4948d9937), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/moon/`.
- "Blackhole" by [rubykamen](https://sketchfab.com/rubykamen), [Sketchfab](https://sketchfab.com/3d-models/blackhole-74cbeaeae2174a218fe9455d77902b5c), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/blackhole/`: a solid black hole inside its glowing accretion disc.
- "Camel" by [anfisa-potekha](https://sketchfab.com/anfisa-potekha), [Sketchfab](https://sketchfab.com/3d-models/camel-768c12e97a3840c4b75c2b9c8cff307a), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/camel/`.
- "Cock (Chicken Gun)" by [MirP](https://sketchfab.com/MirP001), [Sketchfab](https://sketchfab.com/3d-models/cock-chicken-gun-82ae54f80e774c3fbd61ea17180beaab), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/rooster/`.
- "Shenron (Dragon Ball)" by [Anthony Yanez](https://sketchfab.com/paulyanez), [Sketchfab](https://sketchfab.com/3d-models/shenron-dragon-ball-96e8ad1e206941ce859c5733bec75d30), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/dragon/` without the dragon balls.
- "Dune Sandworm" by [Wavebe](https://sketchfab.com/Wavebe), [Sketchfab](https://sketchfab.com/3d-models/dune-sandworm-8d4fe90ed22f4e4293b8cdfeb4968c0f), licensed [CC BY 4.0](http://creativecommons.org/licenses/by/4.0/). Converted to OBJ in `objects/sandworm/`, scaled 1.5x and placed in the valley beyond the camp.
