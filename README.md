# Not Engine Beta 0.2.0

![GitHub commit activity](https://img.shields.io/github/commit-activity/t/sinokadev/notengine) [![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/sinokadev/notengine/build.yml)](https://github.com/sinokadev/notengine/actions) [![Read the Docs](https://img.shields.io/readthedocs/notengine)](https://notengine.readthedocs.io/en/latest/) ![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/sinokadev/notengine/total) [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/sinokadev/notengine)
](https://github.com/sinokadev/notengine/pulls) [![GitHub License](https://img.shields.io/github/license/sinokadev/notengine)](https://github.com/sinokadev/notengine/blob/main/LICENSE) 

> Everything as code!

A code-first game engine built on OpenGL, empowering you to build everything from scratch with clean, simple code.

**PRs and issues are welcome!**

<img width="1282" height="749" alt="image" src="/images/shadowmap.png" /><br>

## Supported Features

- Skymap
- PBR shader
- Event system
- OBJ file import
- MTL material import (albedo, roughness, metallic, normal maps)
- Play Audio

## Performance

| Version | Objects | Model | FPS | VSync |
|---|---:|---|---:|---|
| 0.1.5 | 1,000,000 | `notbox.obj` | 1 FPS | ON |
| 0.1.6 | 1,000,000 | `notbox.obj` | **30 FPS** | ON |
| 0.1.10 | 1,000,000 | `notbox.obj` | **46 FPS** | OFF |

The same scene was used for both benchmarks.

Renderer optimization improved performance from **1 FPS to 30 FPS** — approximately **30× faster**.

> Note: Version 0.1.6 also achieved approximately 30 FPS with VSync disabled, confirming that the performance improvement was not caused by VSync.

### Test Environment

- CPU: AMD Ryzen 7 7800X3D (16) @ 5.05 GHz
- GPU: AMD Radeon RX 9070 XT [Discrete]
- Resolution: 1280×720
- Monitor: 144Hz

## Build

You will need the `cmake`, `ninja` (or `make`), and `glfw3` (development) packages to build this project.

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

# Benchmark
./build/ninja-release/benchmark
```

## Asset Source

- skymap: https://ambientcg.com/view?id=DaySkyHDRI001A

## Star History

<a href="https://www.star-history.com/?repos=sinokadev%2Fnotengine&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=sinokadev/notengine&type=date&theme=dark&legend=top-left&sealed_token=8zWNyxLJDIcZDmt26iDQup4hkwqqKFTk3B4h7SQ5zy_a2ScOp5yboWcm3Ad0ZK_6keAOYbcNYMYg6wJABSGtK7avPjye2IB7HdHTQveh29N1xXwjeZ1_BzkTUBoqN7wmTXuKy24hTpKRecVwiE2SQrLqu4RkcJM7b6GxURYu6Wjmb09hycdMUE59cKZo" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=sinokadev/notengine&type=date&legend=top-left&sealed_token=8zWNyxLJDIcZDmt26iDQup4hkwqqKFTk3B4h7SQ5zy_a2ScOp5yboWcm3Ad0ZK_6keAOYbcNYMYg6wJABSGtK7avPjye2IB7HdHTQveh29N1xXwjeZ1_BzkTUBoqN7wmTXuKy24hTpKRecVwiE2SQrLqu4RkcJM7b6GxURYu6Wjmb09hycdMUE59cKZo" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=sinokadev/notengine&type=date&legend=top-left&sealed_token=8zWNyxLJDIcZDmt26iDQup4hkwqqKFTk3B4h7SQ5zy_a2ScOp5yboWcm3Ad0ZK_6keAOYbcNYMYg6wJABSGtK7avPjye2IB7HdHTQveh29N1xXwjeZ1_BzkTUBoqN7wmTXuKy24hTpKRecVwiE2SQrLqu4RkcJM7b6GxURYu6Wjmb09hycdMUE59cKZo" />
 </picture>
</a>

## Contributors

<a href="https://github.com/sinokadev/notengine/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=sinokadev/notengine" />
</a>

Made with [contrib.rocks](https://contrib.rocks).
