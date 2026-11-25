# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

A university OpenGL 4 / C++ demo scene (README marks it unmaintained): several OBJ models (house, tents, grass, farmer, goat, vulture, wood), a skybox, directional light with shadow mapping, fog, and keyboard/mouse camera. Windows-only Visual Studio 2013 project; there is no Makefile/CMake, no tests, and no linter.

## Build and run

- Open `OpenGL_4_Application_VS2013.sln` in Visual Studio (toolset v120) and build Debug/Release. Cannot be built on Linux as checked in: GLFW/GLEW are vendored as prebuilt Windows `.lib`/`.dll` files in `OpenGL_4_Application_VS2013/GLFW` and `GLEW`, and GLM is vendored in `glm/`.
- The program loads assets (`objects/`, `shaders/`, `textures/`) with paths relative to the working directory, so run it with the project directory `OpenGL_4_Application_VS2013/` as the working directory (VS does this by default).
- `Debug/`, `Release/`, `.vs/`, `.sdf`, `.suo`, and macOS `._*` files are committed build/IDE artifacts. Ignore them and don't edit them.

## Architecture

All code is in `OpenGL_4_Application_VS2013/`. `OpenGL_4_Application_VS2013.cpp` holds most of the application as globals and free functions: window/GL init, `initModels`/`initShaders`/`initUniforms`/`initFBOs`, `processMovement` (per-frame input handling), `renderScene`, and `main`.

Helper classes are in the `gps` namespace:
- `Shader`: compiles and links a shader program from vert/frag files.
- `Model3D` and `Mesh`: load OBJ plus MTL via `tiny_obj_loader.h` and draw with a given shader. Textures are loaded with `stb_image`. Each model lives in its own `objects/<name>/` folder, and the constructor takes the `.obj` path and the base dir for the `.mtl`.
- `Camera`: free-fly camera driven by `move()` and `mouse_callback()`.
- `SkyBox`: cubemap loaded from `textures/skybox/sahara_*.tga`, drawn with `skyboxShader`.

Rendering is two-pass shadow mapping. The depth pass uses `simpleDepthMap` and writes into `shadowMapFBO`/`depthMapTexture`. The main pass uses `shaderStart`, which does lighting, shadows and fog. `lightCube` draws a cube at the light position, and the skybox is drawn last. Uniform locations are cached in globals (`modelLoc`, `lightDirLoc`, `fogLoc`, …) in `initUniforms`. A new uniform needs a global location, a lookup in `initUniforms`, and a use in the shader.

Input uses a `pressedKeys[1024]` array set by the GLFW key callback and read each frame in `processMovement`. Keys: WASD plus mouse move the camera; Q/E rotate the scene; K/L rotate the light; F/G fog on/off; H/J lights on/off; X toggles wireframe; arrow keys move the goat; C launches the "rocket"; Esc quits. Note that X toggles on every frame while held.
