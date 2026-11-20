## 0.2.3 - 2026-11-20

### Added
- Added a particle system with CPU simulation and instanced billboard rendering.
- Added a rain demo and streak rendering for fast-moving particles.
- Added 3D positional audio and EFX reverb environments through OpenAL.
- Added post-processing and bloom.
- Added experimental SSGI and reflection probes.
- Added PCSS and shadow bias settings.
- Added a runtime shader registry and engine settings state.
- Added splash scenes, loading images, debug UI, and font rendering.
- Added glTF loading for lights and object names.
- Added object groups and improved model handling.
- Added a fixed frame delta setting for reproducible captures.
- Added TODO management rules and roadmap documentation.

### Changed
- Improved resource ownership and related internal structures in scenes and the renderer.
- Changed the shader manager to a singleton.
- Separated asset roots from shader source roots across demos and documentation.
- Updated key input conversion to use GLFW scan codes instead of key codes.
- Removed the unused `seno-editor`.

### Fixed
- Fixed particle streaks to follow world velocity instead of the camera.
- Fixed shadow sampler, PCF, and PCSS rendering issues.
- Fixed out-of-range mesh indices and scene state handling in the Seno and glTF loaders.
- Fixed `lookAt` rotations for straight up and down directions.
- Fixed material texture fallbacks, text rendering, and screenshot key behavior.
- Fixed documentation builds and demo help text.

## 0.2.2 - 2026-10-23

## What's Changed
* Added depth-prepass SSAO with blur and an O-key toggle in the scene renderer.
* Added PCF point-light shadows and improved shadow quality while keeping point-shadow maps at 512 for performance.
* Added a PNG screenshot capture API.
* Added glTF model and scene support, sample assets, group-level transforms, and transparent-material rendering.
* Improved PBR shading, roughness-map LOD handling, and renderer performance.
* Simplified build configuration and centralized the version in a single `VERSION` file.

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.2.1...v0.2.2

## 0.2.1 - 2026-09-27

## What's Changed
* chore: separate CMake deps file from CMakeLists.txt by @choonarine in https://github.com/sinokadev/notengine/pull/5
* Performance improvement
* Add tween animation

## New Contributors
* @choonarine made their first contribution in https://github.com/sinokadev/notengine/pull/5

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.2.0...v0.2.1

## 0.2.0 - 2026-09-02

# Not Engine Beta!!

## Added
- completely new camera system

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.12...v0.2.0

## 0.1.12 - 2026-09-01

## What's Changed
* CMakePresets.json 추가 by @watta0 in https://github.com/sinokadev/notengine/pull/2
* Implaments audio.h by @watta0 in https://github.com/sinokadev/notengine/pull/3

## New Contributors
* @watta0 made their first contribution in https://github.com/sinokadev/notengine/pull/2

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.11...v0.1.12

## 0.1.11 - 2026-08-29

# Change

- add Dir Shadow Map

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.10...v0.1.11

## 0.1.10 - 2026-08-23

### Added
- Added a directional shadow map and scene-camera documentation.
- Added Doxygen documentation and Read the Docs build support.
- Added a Seno hot-reloader and `.mtl` material loading for OBJ models.
- Added a Python USD-to-Seno conversion script.
- Added scene initialization logging and domain information.

### Changed
- Added Seno format versions 3 and 4 with backward compatibility.
- Added and adjusted render exposure and lighting behavior.
- Updated demo code and removed obsolete test files.

### Fixed
- Fixed the benchmark demo and renderer instance handling.
- Fixed VSync documentation and configuration values.
- Fixed Read the Docs configuration and Doxygen setup.

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.9...v0.1.10

## 0.1.8.1 - 2026-08-20

HOTFIX

### Fixed
- Fixed Seno loading so it uses the current format.
- Fixed the benchmark demo.
- Fixed compatibility with Seno format versions.

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.8...v0.1.8.1

## 0.1.8 - 2026-08-20

# Changelog

- Applied optimization to disable instancing for single objects.
- Fixed a structural issue arising from Lights and Cameras inheriting from Object.
- Scene format development and loader creation

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.7.1...v0.1.8

## 0.1.7.1 - 2026-08-18

HOTFIX

### Fixed
- Fixed scene camera copying.
- Fixed README documentation.

**Full Changelog**: https://github.com/sinokadev/notengine/compare/v0.1.7...v0.1.7.1

## 0.1.7 - 2026-08-18

# Changelog
- Added `quit` function
- Fixed an issue where resources were not automatically closed upon termination.

**Full Changelog**: https://github.com/sinokadev/notengine/commits/v0.1.7

## 1.4-alpha - 2026-06-26

### Added
- Added the initial engine, demo applications, shader classes, and shader manager.
- Added camera and keyboard input systems, including GLFW scan-code conversion.
- Added texture support and a model/scene foundation.
- Added rendering features for objects, instancing, and frustum culling.
- Added PBR shading, multiple lights, and an event system.
- Added documentation, licensing, project roadmap, and build information.

**Full Changelog**: https://github.com/sinokadev/notengine/commits/v1.4-alpha
