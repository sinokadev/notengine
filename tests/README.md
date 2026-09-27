# Unit tests

Run from the repository root (CMake 3.25+ is required for the version 6 presets):

```sh
cmake --preset unit-debug
cmake --build --preset unit-debug
ctest --preset unit-debug
```

Use `unit-release` in all three commands to test a Release build. The presets
build the `unit_tests` aggregate target and select only CTest's `unit` label.
Each test executable reports named cases and exits nonzero on any failure;
checks remain enabled under `NDEBUG`. No extra test framework is downloaded.

The five suites cover:

- Transform composition, pivots, direction vectors, CPU model bounds and accessors.
- Camera projection/view matrices, movement, pitch limits and frustum intersections.
- Object/light registration, lookup, groups, removal, ownership and ID reset.
- Transform interpolation, easing, completion and fractional-millisecond accumulation.
- Relative/absolute scene asset paths and explicit `{assetRoot}` substitution.

Tests link the existing engine, so its normal GLFW, GLM and OpenGL build
dependencies are required. Execution needs no display, GPU context, Xvfb or
audio device: no window creation, mesh upload, rendering or device initialization
is performed. `gltf_test.cpp` is an existing graphics-context integration test
and is not registered in this unit suite.

For other generators, configure with `-DBUILD_TESTING=ON`, build `unit_tests`,
then run `ctest --test-dir <build-directory> -L unit --output-on-failure`.
For multi-config generators, supply `--config Debug` when building and `-C Debug`
when running CTest. `-DBUILD_TESTING=OFF` omits all new unit-test targets.
