Loading splash
==============

``drawLoadingSplash`` draws the built-in loading image over the current
framebuffer. Pass the framebuffer width and height in pixels. Call it while an
OpenGL context is current; the helper creates its shader and texture on first
use.

.. code-block:: cpp

   knot::utility::drawLoadingSplash(framebufferWidth, framebufferHeight);

.. doxygenfunction:: knot::utility::drawLoadingSplash
