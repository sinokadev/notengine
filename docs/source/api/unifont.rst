Unifont text rendering
======================

The header-only ``knot::utility::drawText`` helper draws the bundled 8x16
Unifont bitmap as pixel-aligned text. Include
``<knot/utility/unifont.h>`` and call it after setting the OpenGL viewport,
typically near the end of a frame callback.

Coordinates are relative to the viewport. The origin is its top-left corner,
and each glyph advances the cursor by 8 pixels. Newlines return to the initial
x coordinate and move down 16 pixels. The font supports printable ASCII
characters (32–126); other bytes render as ``?``. The default color is opaque
white.

.. code-block:: cpp

   #include <knot/utility/unifont.h>

   // In a render callback, after setting the viewport:
   knot::utility::drawText(12, 20, "FPS: 60");

Use the optional RGBA argument to choose a color:

.. code-block:: cpp

   knot::utility::drawText(12, 40, "Ready", {0.2f, 1.0f, 0.3f, 1.0f});

The font bitmap is stored as a packed array in the header, so the helper does
not load ``assets/font.png`` at runtime. The source image is included as the
editable reference asset.

.. doxygenfunction:: knot::utility::drawText
