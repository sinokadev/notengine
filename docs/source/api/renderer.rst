Renderer API
============

.. doxygenclass:: knot::Renderer
   :members:

Reflection probe captures render six views of the scene and bake their
irradiance and specular maps. Call ``captureReflectionProbe(scene)`` with an
active OpenGL context after configuring the probe; the method returns ``false``
if capture cannot be completed.

SSGI is controlled by ``knot::s.ssgi`` (disabled by default). It estimates
one-bounce indirect diffuse light from visible opaque surfaces at half
resolution; light sources and blockers outside the screen cannot contribute.
Adjust ``knot::s.ssgi_intensity`` and ``knot::s.ssgi_radius`` to tune its
strength and world-space ray length.

.. doxygenstruct:: knot::InstanceData
   :members:

.. doxygenstruct:: knot::VisibleInstance
   :members:

.. doxygenstruct:: knot::GPUMovingPointLight
   :members:

