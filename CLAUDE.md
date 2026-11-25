# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

An OpenGL 4.1 core-profile / C++11 demo scene for Linux (README marks it unmaintained): several OBJ models (house, tents, grass, farmer, goat, vulture, wood), a skybox, directional light with shadow mapping, fog, and a keyboard/mouse camera. There are no tests and no linter.

## Build and run

- Install `build-essential cmake libglfw3-dev libglew-dev libgl-dev`. GLM is vendored in `third_party/glm`.
- `cmake -S . -B build && cmake --build build` builds `build/opengl_scene`. `cmake --build build --target run` builds and runs it.
- Assets (`objects/`, `shaders/`, `textures/`) are loaded via paths relative to the working directory, so always run from the repo root. Filenames are case-sensitive (e.g. `objects/wood/wood.obj`).
- Tested with the Ubuntu 24.04 packages (CMake 3.28, GLFW 3.3, GLEW 2.2) on Mesa with a 4.6 core context.

## Architecture

All code is in `src/`. `main.cpp` holds most of the application as globals and free functions: window/GL init, `initModels`/`initShaders`/`initUniforms`/`initFBOs`, `processMovement` (per-frame input handling), `renderScene`, and `main`.

Helper classes are in the `gps` namespace:
- `Shader`: compiles and links a shader program from vert/frag files.
- `Model3D` and `Mesh`: load OBJ plus MTL via `tiny_obj_loader.h` and draw with a given shader. Textures are loaded with `stb_image`. Each model lives in its own `objects/<name>/` folder, and the constructor takes the `.obj` path and the base dir for the `.mtl`.
- `Camera`: free-fly camera driven by `move()` and `mouse_callback()`.
- `SkyBox`: cubemap loaded from `textures/skybox/sahara_*.tga`, drawn with `skyboxShader`.

Rendering is two-pass shadow mapping. The depth pass uses `simpleDepthMap` and writes into `shadowMapFBO`/`depthMapTexture`. The main pass uses `shaderStart`, which does lighting, shadows and fog. `lightCube` draws a cube at the light position, and the skybox is drawn last. Uniform locations are cached in globals (`modelLoc`, `lightDirLoc`, `fogLoc`, …) in `initUniforms`. A new uniform needs a global location, a lookup in `initUniforms`, and a use in the shader.

Vertex attribute locations are fixed (0 position, 1 normal, 2 texcoords) in `Mesh.cpp`/`SkyBox.cpp`, and each vertex shader declares matching `layout(location=N)`. All shaders start with `#version 410 core`.

Input uses a `pressedKeys[1024]` array set by the GLFW key callback and read each frame in `processMovement`. Keys: WASD plus mouse move the camera; Q/E rotate the scene; K/L rotate the light; F/G fog on/off; H/J lights on/off; X toggles wireframe; arrow keys move the goat; C launches the "rocket"; Esc quits. Note that X toggles on every frame while held.
