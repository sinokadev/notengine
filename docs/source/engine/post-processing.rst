Post-processing
===============

Register full-screen shaders with
:cpp:func:`knot::Renderer::addPostProcessShader`. Passes run in registration
order after the scene has rendered. If no passes are registered, the renderer
uses its pass-through shader to copy the scene to the output framebuffer.

Each pass receives these standard uniforms and textures:

.. list-table::
   :header-rows: 1

   * - Uniform
     - Meaning
     - Texture unit
   * - ``u_InputTex``
     - Previous pass output (or the original scene color for the first pass)
     - 0
   * - ``u_SceneColorTex``
     - Original scene color, unchanged by post-processing
     - 1
   * - ``u_SceneDepthTex``
     - Original scene depth
     - 2
   * - ``u_Resolution``
     - Framebuffer width and height as a ``vec2``
     - N/A
   * - ``u_TexelSize``
     - Inverse framebuffer width and height as a ``vec2``
     - N/A

Use the optional :cpp:type:`knot::Renderer::PostProcessSetup` callback to set
pass-specific uniforms and bind additional textures on units 3 and above. The
callback also receives the original scene color/depth texture IDs and
framebuffer resolution.
:cpp:func:`knot::Renderer::removePostProcessShader` removes the first matching
pass; :cpp:func:`knot::Renderer::clearPostProcessShaders` removes the entire
chain.

The ``bloom_demo`` target demonstrates Bloom assembled from ordinary
post-processing shaders and registered passes. Bloom is not a special feature
or mode in ``Renderer``.

.. dropdown:: View full code for bloom_demo.cpp
   :icon: code

   .. literalinclude:: ../../../demo/bloom_demo.cpp
      :language: cpp
      :caption: demo/bloom_demo.cpp
