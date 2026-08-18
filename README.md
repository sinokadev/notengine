# Not Engine Alpha 0.1.7.1

> Everything as code!

A code-first game engine built on OpenGL, empowering you to build everything from scratch with clean, simple code.

### Upgrade Now
If you are using version 0.1.6 or earlier, upgrade to version 0.1.7 or later immediately.

Versions 0.1.6 and earlier have a critical issue where resources are not automatically cleaned up upon termination, leading to memory leaks. **Please upgrade immediately.**

<img width="1282" height="749" alt="image" src="/images/626650067-de731f10-f17b-42cb-a143-d4fbbff1a457.png" /><br>
<sub>demo/demo.cpp</sub>

## Supported Features

- Skymap
- PBR shader
- Event system
- OBJ file import

## Performance

| Version | Objects | Model | FPS | VSync |
|---|---:|---|---:|---|
| 0.1.5 | 1,000,000 | `notbox.obj` | 1 FPS | ON |
| 0.1.6 | 1,000,000 | `notbox.obj` | **30 FPS** | ON |

The same scene was used for both benchmarks.

Renderer optimization improved performance from **1 FPS to 30 FPS** — approximately **30× faster**.

> Note: Version 0.1.6 also achieved approximately 30 FPS with VSync disabled, confirming that the performance improvement was not caused by VSync.

### Test Environment

- CPU: AMD Ryzen 7 7800X3D (16) @ 5.05 GHz
- GPU: AMD Radeon RX 9070 XT [Discrete]
- Resolution: 1280×720
- Monitor: 144Hz

## Build

You will need the `cmake` and `glfw3` (development) packages to build this project.

### Prerequisites (Linux/Ubuntu)

```bash
sudo apt update
sudo apt install cmake libglfw3-dev build-essential
```

### Instructions

Run the following scripts to build the library and the demos:

```bash
# Build the core library
./script/library_release_build.sh

# Build the demo executables
./script/demo_release_build.sh
```

## Asset Source

- skymap: https://ambientcg.com/view?id=DaySkyHDRI001A
