# Group transforms

`ObjectManager::transformGroup(name, delta)` transforms every current group member
as a compound object. Rotation and scale use one world-space pivot, so members
orbit and spread around that point together instead of rotating in place.

```cpp
knot::Transform delta;
delta.position = glm::vec3(1.0f, 0.0f, 0.0f); // world-space translation
delta.pivot = glm::vec3(0.0f, 2.0f, 0.0f);    // common world-space pivot
delta.rotation = glm::angleAxis(glm::radians(30.0f), glm::vec3(0, 1, 0));
delta.scale = glm::vec3(2.0f);
scene.getObjectManager().transformGroup("robot", delta);
```

The function returns the number of affected objects. Each call applies an
additional delta; identity does nothing. The default pivot is the world origin.
The matrix is composed as `deltaMatrix * previousGroupMatrix * localMatrix`,
retaining shear from rotations mixed with nonuniform scale. Rendering and
frustum culling use this full matrix.

After applying a group transform, object `position`, `rotation`, `scale`, and
`pivot` remain local values. Read `getWorldMatrix()` for the final world transform;
the inherited direction helpers still describe the local rotation. Groups remain
tags, with no persistent parent or nested hierarchy. Membership is checked on each
call: new members do not inherit previous deltas, and removed members keep their
last transform. An object tagged in multiple groups participates in each call,
once per call, in call order. Transforms are runtime state and are not serialized
to `.seno` files.

Run the CPU-only regression tests with:

```sh
cmake --build build/ninja-release --target test_group_transform
./build/ninja-release/test_group_transform
```
