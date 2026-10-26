Transform tweening
==================

``Tween`` interpolates an object's position, rotation, and scale over a
duration measured in milliseconds. Call ``update`` once per frame with the
frame duration in seconds. The optional easing function receives normalized
progress from 0 to 1; linear interpolation is used when it is empty.

``mixTransform`` performs the corresponding one-step interpolation between
two transforms.

.. doxygenfunction:: knot::mixTransform

.. doxygenstruct:: knot::Tween
