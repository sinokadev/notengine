Engine settings
===============

``EngineSettings`` holds the renderer options exposed through the global
``knot::engine_settings`` instance (also available as ``knot::s``). Defaults
enable the sky map, shadows, and SSAO; SSGI is disabled by default. SSGI intensity
defaults to 0.35 and its ray radius to 2.0 world units. Ambient strength defaults
to 1.0, directional shadow resolution to 2048, and point shadow resolution to
512. Enable SSGI with ``knot::s.ssgi = true``.

.. doxygenstruct:: knot::EngineSettings
   :members:

.. doxygenvariable:: knot::engine_settings

.. doxygenvariable:: knot::s

The generated ``knot/version.h`` header provides the major, minor, patch, and
combined version macros at build time.
