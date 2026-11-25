cmake_minimum_required(VERSION 3.10)
project(opengl_scene CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(opengl_scene
  src/main.cpp
  src/Camera.cpp
  src/Mesh.cpp
  src/Model3D.cpp
  src/Shader.cpp
  src/SkyBox.cpp
  src/stb_image.c)

# Vendored GLM is included as "glm/glm.hpp".
target_include_directories(opengl_scene PRIVATE src third_party)
target_link_libraries(opengl_scene PRIVATE OpenGL::GL glfw GLEW::GLEW)

# Assets are loaded via relative paths, so run from the repo root.
add_custom_target(run COMMAND opengl_scene WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} DEPENDS opengl_scene)
