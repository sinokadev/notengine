Particle API
============

``knot/particle.h`` is a GPU-agnostic CPU particle simulation. A
``knot::ParticleSystem`` owns a fixed-capacity pool, spawns, integrates and
recycles particles on the CPU, and a scene registers it with
``knot::Scene::addParticleSystem`` so the scene advances it every frame and
``knot::Renderer::renderParticles`` draws it as instanced billboards.

.. doxygenclass:: knot::ParticleSystem
   :members:

.. doxygenstruct:: knot::ParticleSystemSettings
   :members:

.. doxygenstruct:: knot::Particle
   :members:
