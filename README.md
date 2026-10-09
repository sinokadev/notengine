# Not Engine Beta 0.2.1

![GitHub commit activity](https://img.shields.io/github/commit-activity/t/sinokadev/notengine) [![GitHub Actions Workflow Status](https://github.com/sinokadev/notengine/actions/workflows/build.yml/badge.svg)](https://github.com/sinokadev/notengine/actions) [![Read the Docs](https://img.shields.io/readthedocs/notengine)](https://notengine.readthedocs.io/en/latest/) ![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/sinokadev/notengine/total) [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/sinokadev/notengine)
](https://github.com/sinokadev/notengine/pulls) [![GitHub License](https://img.shields.io/github/license/sinokadev/notengine)](https://github.com/sinokadev/notengine/blob/main/LICENSE) 

> Everything as code!

A code-first game engine built on sokol_gfx (OpenGL 4.3 backend), empowering you to build everything from scratch with clean, simple code.

**PRs and issues are welcome!**

2026-09-27: I just made this for my own use.

<img width="1282" height="749" alt="image" src="/images/shadowmap.png" /><br>

## Supported Features

- Skymap
- PBR shader
- Event system
- OBJ file import
- MTL material import (albedo, roughness, metallic, normal maps)
- Submesh support
- Play Audio

## Performance

| Version | Objects | Model | FPS | VSync |
|---|---:|---|---:|---|
| 0.1.5 | 1,000,000 | `notbox.obj` | 1 FPS | ON |
| 0.1.6 | 1,000,000 | `notbox.obj` | **30 FPS** | OFF |
| 0.1.10 | 1,000,000 | `notbox.obj` | **46 FPS** | OFF |

The same scene was used for both benchmarks.

Renderer optimization improved performance from **1 FPS to 30 FPS** — approximately **30× faster**.

### Test Environment

- CPU: AMD Ryzen 7 7800X3D (16) @ 5.05 GHz
- GPU: AMD Radeon RX 9070 XT [Discrete]
- Resolution: 1280×720
- Monitor: 165Hz

## Build

You will need the `cmake`, `ninja` (or `make`), and `glfw3` (development) packages to build this project.

If you don't install third party libraries using package manager, then CMake installs libraries using `FetchContent` module through Git.

### Prerequisites (Linux/Ubuntu)

```bash
sudo apt update
sudo apt install cmake ninja-build libglfw3-dev build-essential
```

### Instructions

Configure and build using CMake Presets:

```bash
# Configure (Ninja Release)
cmake --preset ninja-release

# Build the core library
cmake --build --preset ninja-release-notengine

# Build the demo executables
cmake --build --preset ninja-release-all
```

Run the built demos:

```bash
# Default demo
./build/ninja-release/demo

# Scene file rendering demo
./build/ninja-release/scene_render

# glTF model demo (Cube)
./build/ninja-release/gltf_model

# glTF scene demo (Sponza)
./build/ninja-release/gltf_scene

# Benchmark
./build/ninja-release/benchmark
```

## Blender Exporter

Export Blender scenes as Seno v8 with separate OBJ models, MTL materials, and
textures in one folder using the [Seno exporter](script/blender_seno/README.md).
Install `script/blender_seno/seno_exporter.py` as a Blender add-on, then choose
**File → Export → Seno Scene (folder)**.

## Asset Source

- skymap: https://ambientcg.com/view?id=DaySkyHDRI001A
- test audio (wav): https://commons.wikimedia.org/wiki/File%3AAudio.wav
- test audio (mp3): https://commons.wikimedia.org/wiki/File%3AUncleSigmund_-_ahhh_(cc0)_(freesound).mp3
- test gltf models: https://github.com/KhronosGroup/glTF-Sample-Models

## Star History

<a href="https://www.star-history.com/?repos=sinokadev%2Fnotengine&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=sinokadev/notengine&type=date&theme=dark&legend=top-left&sealed_token=8zWNyxLJDIcZDmt26iDQup4hkwqqKFTk3B4h7SQ5zy_a2ScOp5yboWcm3Ad0ZK_6keAOYbcNYMYg6wJABSGtK7avPjye2IB7HdHTQveh29N1xXwjeZ1_BzkTUBoqN7wmTXuKy24hTpKRecVwiE2SQrLqu4RkcJM7b6GxURYu6Wjmb09hycdMUE59cKZo" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=sinokadev/notengine&type=date&legend=top-left&sealed_token=8zWNyxLJDIcZDmt26iDQup4hkwqqKFTk3B4h7SQ5zy_a2ScOp5yboWcm3Ad0ZK_6keAOYbcNYMYg6wJABSGtK7avPjye2IB7HdHTQveh29N1xXwjeZ1_BzkTUBoqN7wmTXuKy24hTpKRecVwiE2SQrLqu4RkcJM7b6GxURYu6Wjmb09hycdMUE59cKZo" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=sinokadev/notengine&type=date&legend=top-left&sealed_token=8zWNyxLJDIcZDmt26iDQup4hkwqqKFTk3B4h7SQ5zy_a2ScOp5yboWcm3Ad0ZK_6keAOYbcNYMYg6wJABSGtK7avPjye2IB7HdHTQveh29N1xXwjeZ1_BzkTUBoqN7wmTXuKy24hTpKRecVwiE2SQrLqu4RkcJM7b6GxURYu6Wjmb09hycdMUE59cKZo" />
 </picture>
</a>

### Rendering API

GPU buffers, textures, shaders, pipelines and render passes use the vendored
`sokol_gfx.h`; GLFW owns the window and OpenGL context. The current backend
requires OpenGL 4.3 for point-light storage buffers. Other sokol backends are
not enabled by this migration.

`Engine` handles the frame lifecycle. When using `Renderer` directly, make the
GLFW context current, call `init()`, and pair `beginFrame(width, height, color)`
with `endFrame()` before swapping buffers. Destroy scenes before shutting down
the renderer, and shut down the renderer before destroying the context.

Texture IDs now identify sokol texture views. Use `destroyTexture(id)` to release
the view and its image; `createTexture(pixels, width, height)` accepts RGBA8 data.
Mesh buffers are exposed as `vertexBuffer` and `indexBuffer`. Shader uniforms
and material bindings are staged until `Shader::draw()` applies the pipeline.
Custom GLSL sources must supply `ShaderSource::interface` reflection metadata
with native, tightly packed uniform blocks and the engine's vertex layout.

The Seno editor retains its ImGui OpenGL backend and resets sokol's state cache
at that integration boundary. Its scene viewport uses the sokol renderer.
