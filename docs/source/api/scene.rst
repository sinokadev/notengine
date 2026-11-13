Scene API
=========

.. doxygenclass:: knot::Scene
   :members:

.. doxygenstruct:: knot::ReflectionProbe
   :members:

Configure one local probe with :cpp:func:`knot::Scene::setReflectionProbe`, then
call :cpp:func:`knot::Renderer::captureReflectionProbe` after scene geometry,
lighting, and the global environment have been set up. Captures are explicit;
call the renderer method again after changing the scene or probe position.
The probe contributes diffuse and specular IBL inside its bounds, blending back
to the scene's global environment near the bounds' edges. Set
``knot::s.reflection_probe`` in ``knot/state.h`` to enable or disable probe
contributions at runtime.

