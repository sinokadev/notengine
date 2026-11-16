Unifont font backend
====================

``<knot/utility/unifont.h>`` contains the embedded glyph data and internal
OpenGL renderer used by the debug UI. Application code should include
``<knot/utility/debug_ui.h>`` for text rendering, buttons, sliders, and usage
examples; see :doc:`debug-ui`.

.. literalinclude:: ../../../include/knot/utility/unifont.h
   :language: cpp
   :caption: include/knot/utility/unifont.h