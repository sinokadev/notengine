Events and input API
====================

.. doxygenstruct:: knot::Event
   :members:

.. doxygenenum:: knot::EventType

``Event`` values are emitted through ``Engine::setEventCallback``. Keyboard
and mouse values use ``ScanCode``, ``KeyState``, and ``MouseKey`` from
``knot/key.h``.

