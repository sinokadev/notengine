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

### Added
- Added depth-prepass SSAO with blur and an O-key toggle in the scene renderer.
- Added PCF point-light shadows and improved shadow quality while keeping point-shadow maps at 512 for performance.
- Added a PNG screenshot capture API.
- Added glTF model and scene support, sample assets, group-level transforms, and transparent-material rendering.

### Changed
- Improved PBR shading, roughness-map LOD handling, and renderer performance.
- Simplified build configuration and centralized the version in a single `VERSION` file.

## 0.2.1 - 2026-09-27

### Added
- Added tween animation.

### Changed
- Separated CMake deps file from `CMakeLists.txt` by @choonarine in https://github.com/sinokadev/notengine/pull/5
- Improved overall performance.

### New Contributors
- @choonarine made their first contribution in https://github.com/sinokadev/notengine/pull/5

## 0.2.0 - 2026-09-02
*Not Engine Beta release*

### Added
- Implemented a completely new camera system.

## 0.1.12 - 2026-09-01

### Added
- Added `CMakePresets.json` by @watta0 in https://github.com/sinokadev/notengine/pull/2
- Implemented `audio.h` by @watta0 in https://github.com/sinokadev/notengine/pull/3

### New Contributors
- @watta0 made their first contribution in https://github.com/sinokadev/notengine/pull/2

## 0.1.11 - 2026-08-29

### Added
- Added directional shadow map.

## 0.1.10 - 2026-08-23

### Added
- Added a directional shadow map and scene-camera documentation.
- Added Doxygen documentation and Read the Docs build support.
- Added a Seno hot-reloader and `.mtl` material loading for OBJ models.
- Added a Python USD-to-Seno conversion script.
- Added scene initialization logging and domain information.

### Changed
- Added Seno format versions 3 and 4 with backward compatibility.
- Adjusted render exposure and lighting behavior.
- Updated demo code and removed obsolete test files.

### Fixed
- Fixed the benchmark demo and renderer instance handling.
- Fixed VSync documentation and configuration values.
- Fixed Read the Docs configuration and Doxygen setup.

## 0.1.8.1 - 2026-08-20 (Hotfix)

### Fixed
- Fixed Seno loading so it uses the current format.
- Fixed the benchmark demo.
- Fixed compatibility with Seno format versions.

## 0.1.8 - 2026-08-20

### Added
- Developed scene format and created scene loader.

### Changed
- Applied optimization to disable instancing for single objects.

### Fixed
- Fixed a structural issue arising from Lights and Cameras inheriting from Object.

## 0.1.7.1 - 2026-08-18 (Hotfix)

### Fixed
- Fixed scene camera copying.
- Fixed README documentation.

## 0.1.7 - 2026-08-18

### Added
- Added `quit` function.

### Fixed
- Fixed an issue where resources were not automatically closed upon termination.

## Alpha 1.4 - 2026-06-26

### Added
- Added OBJ model loading and sample models.
- Added Phong and PBR shaders, with support for multiple lights.
- Added the event system and a Pong demo.

### Fixed
- Fixed window sizing on scaled displays.

## Alpha 1.3 - 2026-06-18

### Added
- Added a `Scene` type to own scene objects and related state.
- Added the engine's first scene-based demo flow.

### Changed
- Updated the project license and public headers.

## Alpha 1.2 - 2026-06-17

### Added
- Added mouse input and a toolkit demo.

### Changed
- Split the camera declaration and implementation into separate files.
- Added formatter configuration and standardized naming.

## Alpha 1.1 - 2026-06-14

### Added
- Added texture loading and keyboard input helpers based on GLFW scan codes.
- Added the project license and expanded the engine API.

## Alpha 1.0 - 2026-06-11

### Added
- Established the CMake-based C++/OpenGL engine, including its engine, window, renderer, resource, and shader foundations.
- Added basic camera-driven mesh rendering and a rotating-box demo.