# Not Engine Alpha 0.1.6

> Everything as code!

A code-first game engine built on OpenGL, empowering you to build everything from scratch with clean, simple code.

### 🔥 Alpha 0.1.6 Release Highlight

> **Renderer optimization has been applied!!!!!!!!!!!!**
>
> **Now, rendering 250,000 Utah Teapots—each with 94,426 faces—yields 19 FPS!!!!!!!!!!!!!!!!!!**

<img width="1282" height="749" alt="image" src="/images/image.png" /><br>
<sub>demo/demo.cpp</sub>

# Supported Feature

- Skymap
- PBR shader
- Event system
- OBJ file import

# Build

You will need the `cmake` and `glfw3` (development) packages to build this project.

## Prerequisites (Linux/Ubuntu)

```
sudo apt update
sudo apt install cmake libglfw3-dev build-essential
```

## Instructions

Run the following scripts to build the library and the demos:

```
# Build the core library
./script/library_release_build.sh

# Build the demo executables
./script/demo_release_build.sh
```

## 에셋 출처

- https://ambientcg.com/view?id=DaySkyHDRI001A
