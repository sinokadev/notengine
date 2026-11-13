Renderer API
============

.. doxygenclass:: knot::Renderer
   :members:

Reflection probe captures render six views of the scene and bake their
irradiance and specular maps. Call ``captureReflectionProbe(scene)`` with an
active OpenGL context after configuring the probe; the method returns ``false``
if capture cannot be completed.

.. doxygenstruct:: knot::InstanceData
   :members:

.. doxygenstruct:: knot::VisibleInstance
   :members:

.. doxygenstruct:: knot::GPUMovingPointLight
   :members:

