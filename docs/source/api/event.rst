Events and input API
====================

.. doxygenstruct:: knot::Event
   :members:

.. doxygenenum:: knot::EventType

:cpp:class:`knot::Event` values are emitted through
:cpp:func:`knot::Engine::setEventCallback`. Keyboard and mouse values use
:cpp:class:`knot::ScanCode`, :cpp:class:`knot::KeyState`, and
:cpp:class:`knot::MouseKey` from
``knot/key.h``.

