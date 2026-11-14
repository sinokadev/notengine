Particle API
============

``knot/particle.h`` is a GPU-agnostic CPU particle simulation. A
:cpp:class:`knot::ParticleSystem` owns a fixed-capacity pool, spawns, integrates
and recycles particles on the CPU, and a scene registers it with
:cpp:func:`knot::Scene::addParticleSystem` so the scene advances it every frame
and :cpp:func:`knot::Renderer::renderParticles` draws it as instanced billboards.

.. doxygenclass:: knot::ParticleSystem
   :members:

.. doxygenstruct:: knot::ParticleSystemSettings
   :members:

.. doxygenstruct:: knot::Particle
   :members:
