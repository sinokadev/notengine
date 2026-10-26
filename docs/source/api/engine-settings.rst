Engine settings
===============

``EngineSettings`` holds the renderer options exposed through the global
``knot::engine_settings`` instance (also available as ``knot::s``). Defaults
enable the sky map, shadows, and SSAO, with ambient strength 1.0, directional
shadow resolution 2048, and point shadow resolution 512.

.. doxygenstruct:: knot::EngineSettings

The generated ``knot/version.h`` header provides the major, minor, patch, and
combined version macros at build time.
