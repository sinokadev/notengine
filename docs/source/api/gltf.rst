glTF model and scene import
==========================

Both APIs use the bundled cgltf parser and require a current OpenGL context
with GLAD initialized. They accept glTF 2.0 JSON and binary GLB, external buffers
(relative to the asset file), base64 buffer URIs, and embedded GLB buffers.

Model assets
------------

.. code-block:: cpp

   auto shader = scene.getResourceManager().getShader("pbrShader");
   auto model = knot::loadModelGLTF("assets/chair.glb", shader);
   if (model)
       scene.getObjectManager().registerObject(std::make_shared<knot::Object>(model));

``loadModelGLTF`` reads each mesh resource once into one model, with one submesh
per primitive. Nodes, node transforms, scene membership, cameras and lights are
ignored. Author model assets in the desired local coordinates. Scene assets
passed to this API have no supported placement/instancing semantics; use
``Scene::loadGLTF`` for them. Failure returns ``nullptr`` and logs the reason.

Seno v8 can reference a model through ``{"gltf": "models/chair.glb"}`` in its
``models`` array. Paths are resolved relative to the Seno file using the existing
asset-path rules. This uses model import semantics.

Scene assets
------------

.. code-block:: cpp

   bool loaded = scene.loadGLTF("assets/world.gltf");
   // Or select a particular scene by its zero-based index:
   loaded = scene.loadGLTF("assets/world.gltf", 1);

The default index, ``-1``, selects the declared default scene, falling back to
the first scene. If the file has no scenes, all root nodes are traversed.
An explicitly empty scene remains empty. Unreferenced mesh resources are not
imported into the scene. Each mesh node becomes a separate object, including
repeated references to the same mesh. Node names become object group names;
names need not be unique.

The hierarchy is flattened at import time: world translation becomes
``Object::position``; accumulated rotation, scale and shear are baked into a
node's geometry. Object rotation/scale start at identity. This preserves static
placement, normals and mirrored winding, but does not create a runtime parenting
relationship. Geometry is owned per mesh node; materials are shared by source
material within an import. Bounds are recalculated for the resulting geometry.

The first camera in depth-first scene order becomes active. Perspective cameras
retain vertical field of view, optional fixed aspect ratio and infinite far plane;
orthographic cameras retain x/y magnification. Camera/light orientation follows
the transformed local -Z axis. Point and directional ``KHR_lights_punctual``
lights are imported. Light range is ignored with a warning; the renderer retains
its own attenuation and lighting conventions.

All selected content is loaded before replacing objects, lights, environment
maps and the update callback. An import error returns ``false`` and retains the
current scene. A successful import without a camera retains the current camera.
glTF does not supply the engine's HDR environment; assign one after import.

Geometry and materials
----------------------

* Indexed and non-indexed triangles, triangle strips and fans are supported.
* Accessor strides, offsets, normalized components and sparse vertex accessors
  are decoded by cgltf. Sparse index accessors are rejected.
* Missing normals generate flat faces; missing tangents are generated with a
  finite fallback for absent/degenerate UVs. Tangent handedness is retained.
* Metallic-roughness PBR supports base color, metallic, roughness, occlusion and
  normal maps. Material factors are baked into imported textures; packed metallic
  (B) and roughness (G) channels are separated for the engine's PBR shader.
* Base-color images are converted from sRGB to linear RGB. PNG/JPEG images can
  be external files, base64 data URIs or buffer views (including GLB images).
  Image orientation and sampler wrap/filter settings are retained. Imported maps
  use the engine's 8-bit texture representation and are owned by their materials.

These conventions follow the `glTF 2.0 specification
<https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>`_.

Current limitations
-------------------

Only ``TEXCOORD_0`` is supported. Texture transforms, missing referenced images,
skinning, morph targets, Draco geometry, GPU instancing extensions, spot lights,
non-triangle primitives and singular mesh transforms cause import failure.
Required extensions other than ``KHR_lights_punctual`` and
``KHR_mesh_quantization`` are rejected.

Animations are ignored with a warning and the declared static pose is imported.
Vertex colors, alpha modes, double-sided materials, unlit shading and emission
are ignored with warnings. Optional material extensions fall back to the core
metallic-roughness representation. This is not a full glTF runtime or a lossless
scene round-trip API.

Verification
------------

.. code-block:: console

   cmake --build build/ninja-release --target test_gltf
   ./build/ninja-release/test_gltf

The integration test creates temporary fixtures and uses a hidden OpenGL 4.3
window. It requires a display (or ``xvfb-run`` on a suitably configured system).
