# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

An OpenGL 4.1 core-profile / C++11 demo scene for Linux (README marks it unmaintained): several OBJ models (house, tents, grass, farmer, goat, vulture, wood), a skybox, directional light with shadow mapping, fog, and a keyboard/mouse camera. There are no tests and no linter.

## Build and run

- Install `build-essential cmake libglfw3-dev libglew-dev libgl-dev`. GLM is vendored in `third_party/glm`.
- `cmake -S . -B build && cmake --build build` builds `build/opengl_scene`. `cmake --build build --target run` builds and runs it.
- Assets (`objects/`, `shaders/`, `textures/`) are loaded via paths relative to the working directory, so always run from the repo root. Filenames are case-sensitive (e.g. `objects/wood/wood.obj`).
- Tested with the Ubuntu 24.04 packages (CMake 3.28, GLFW 3.3, GLEW 2.2) on Mesa with a 4.6 core context.

- `tools/glb2obj.py` converts `.glb` models to OBJ/MTL for the loader (no dependencies). Run it with `python3 -I`.

## Architecture

All code is in `src/`. `main.cpp` holds most of the application as globals and free functions: window/GL init, `initModels`/`initShaders`/`initUniforms`/`initFBOs`, `processMovement` (per-frame input handling), `renderScene`, and `main`.

Helper classes are in the `gps` namespace:
- `Shader`: compiles and links a shader program from vert/frag files.
- `Model3D` and `Mesh`: load OBJ plus MTL via `tiny_obj_loader.h` and draw with a given shader. Textures are loaded with `stb_image`. Each model lives in its own `objects/<name>/` folder, and the constructor takes the `.obj` path and the base dir for the `.mtl`.
- `Camera`: free-fly camera driven by `move()` and `mouse_callback()`.
- `SkyBox`: cubemap loaded from `textures/skybox/sahara_*.tga`, drawn with `skyboxShader`.

Rendering is two-pass shadow mapping. The depth pass uses `simpleDepthMap` and writes into `shadowMapFBO`/`depthMapTexture`. The main pass uses `shaderStart`, which does lighting, shadows and fog. `lightCube` draws a cube at the light position, and the skybox is drawn last. Uniform locations are cached in globals (`modelLoc`, `lightDirLoc`, `fogLoc`, …) in `initUniforms`. A new uniform needs a global location, a lookup in `initUniforms`, and a use in the shader.

Vertex attribute locations are fixed (0 position, 1 normal, 2 texcoords) in `Mesh.cpp`/`SkyBox.cpp`, and each vertex shader declares matching `layout(location=N)`. All shaders start with `#version 410 core`.

`updateDayNight()` runs every frame. It advances `timeOfDay` (60 s per day) and computes `currentLightDir`, which is the sun, or the moon at night. That direction drives the shading, the shadow matrix and the light cube. It also uploads the light color, ambient light, fog color and skybox tint. Don't set `lightDir`/`lightColor` uniforms elsewhere; they are overwritten each frame. `updateFireLight()` runs after the view matrix is set. It uploads the campfire as a flickering point light in eye space; the fire is at `FIRE_POS_MODEL` in the house model's space, so its transform must match the house's model matrix. The fire gets brighter as `nightFactor` rises, and B toggles it. `drawMoon()` draws `objects/moon` after the skybox with its own unlit `moon` shader. It is placed `MOON_DISTANCE` from the camera along `moonSkyDir`, lit by `sunSkyDir` (which gives it phases), and faded with `nightFactor`. It is not part of the shadow pass or collision. Stars are procedural in `skyboxShader.frag`: hashed 3D grid cells on the direction vector, in two layers. They fade with `nightFactor` and turn with the sun/moon arc through `starRotation`, which `updateDayNight()` sets. Planets are steady points in the same shader. `Planets.hpp` computes their elongation from the sun and their magnitude from circular orbits with real radii, periods and J2000 longitudes. The calendar starts at `PLANET_START_DAY` plus `dayCount`, and P adds 10 days. `updatePlanets()` turns those into world directions and twilight/glare-faded brightness. It also adds the yearly star drift to `starRotation`. The skybox is drawn last, because `SkyBox::Draw` binds its own program.

Variable names don't match the models: `farmer` (`objects/farmer/farmer.obj`) is the drivable tank, and `tank` (`objects/goat1/goat1.obj`) is the rocket that C launches. The tank's transform is `tankModelMatrix()`, used by both render passes. `moveTankWithCollision()` applies the arrow-key movement and undoes it, or slides along one axis, if the tank's XZ footprint would newly overlap a static object. Footprints are oriented boxes built from per-mesh bounds (`Model3D::GetMeshBounds`, `Collision.hpp`). The obstacles are the house, tents, wood pile and the rocket while it is on the pad. The terrain (`grass`) and the bird are excluded. Movement uses `frameDt`, which is computed once per frame at the top of `renderScene`.

Input uses a `pressedKeys[1024]` array set by the GLFW key callback and read each frame in `processMovement`. Keys: WASD plus mouse move the camera; Q/E rotate the scene; K/L rotate the sun/moon arc; N pauses the day/night cycle; T skips ahead; P skips 10 days; B toggles the campfire; F/G fog on/off; H/J lights on/off; X toggles wireframe; arrow keys drive the tank; C launches the "rocket"; Esc quits. Note that X toggles on every frame while held.
