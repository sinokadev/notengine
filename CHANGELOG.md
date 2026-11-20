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