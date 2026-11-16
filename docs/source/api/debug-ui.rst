Debug UI
========

``<knot/utility/debug_ui.h>`` provides immediate-mode text, button, and slider
helpers for simple debugging overlays. It uses the current OpenGL context and
viewport and reads cursor and mouse-button input directly from GLFW. Include
``<GLFW/glfw3.h>`` when passing a ``GLFWwindow``.

.. warning::

   These helpers are a temporary debugging tool until Not Engine supports its
   own GUI and may be removed at any time. They are unstable and tightly coupled
   to GLFW; do not use them for gameplay UI. Use a separate library such as Dear
   ImGui for gameplay interfaces.

Call the helpers during rendering, after setting the desired OpenGL viewport.
Coordinates are framebuffer pixels relative to the viewport's top-left corner,
matching :cpp:func:`knot::utility::drawText`. Text uses an embedded 8x16 bitmap font and supports
printable ASCII; unsupported characters render as ``?``.

.. code-block:: cpp

   #include <knot/utility/debug_ui.h>

   // In the render callback, after setting the viewport:
   knot::utility::drawText(12, 20, "FPS: 60");
   if (knot::utility::drawButton(window, 12, 44, "Reset")) {
       resetDebugState();
   }
   knot::utility::drawSlider(window, 12, 68, 160, exposure, 0.0f, 4.0f);

:cpp:func:`knot::utility::drawButton` renders its label in brackets and returns
``true`` on the left-mouse press that begins inside the button.
:cpp:func:`knot::utility::drawSlider` renders an ASCII track quantized to
8-pixel cells; dragging with the left mouse button updates the referenced
value, clamps it to the supplied range, and returns ``true`` when that value
changes. Slider widths below 24 pixels and ranges where ``minimum`` is not
less than ``maximum`` are ignored.

.. doxygenfunction:: knot::utility::drawText

.. cpp:function:: bool knot::utility::drawButton(GLFWwindow* window, int x, int y, std::string_view label)

.. cpp:function:: bool knot::utility::drawSlider(GLFWwindow* window, int x, int y, int width, float& value, float minimum, float maximum)
