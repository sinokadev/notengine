Resources API
=============

Shaders
-------

Runtime shader sources
~~~~~~~~~~~~~~~~~~~~~~

Game code can register shader source text at runtime with
:cpp:func:`knot::ShaderRegistry::registerSource`. The key is a path relative to
``SourceRoot``; use the same path when constructing ``ShaderSource``. For
example:

.. code-block:: cpp

   const std::string vertexKey = "shaders/effect.vert";
   const std::string fragmentKey = "shaders/effect.frag";
   const std::string vertexSource = R"(#version 330 core
   void main() { gl_Position = vec4(0.0); }
   )";
   const std::string fragmentSource = R"(#version 330 core
   out vec4 color;
   void main() { color = vec4(1.0); }
   )";

   knot::ShaderRegistry::registerSource(vertexKey, vertexSource);
   knot::ShaderRegistry::registerSource(fragmentKey, fragmentSource);

   knot::ShaderSource source(
       knot::getSourceRoot() + vertexKey,
       knot::getSourceRoot() + fragmentKey);

The source argument is shader text, not a file path. Registered text remains
available in memory until it is unregistered or the process exits. Registration
does not make the registry embed shader files during the build. If the source
is written as a string literal in the game code, that literal is compiled into
the program like other string data; if it is read from a file, it is loaded at
runtime.

:cpp:class:`knot::ShaderSource` looks up each shader in this order: the
engine's built-in registry, the runtime registry, then the supplied file path.
Runtime keys must be unique, and registration fails if the key is empty,
already registered, or uses a filename present in the built-in registry.
:cpp:func:`knot::ShaderRegistry::unregisterSource` removes a runtime entry by
its key.

.. doxygenclass:: knot::ShaderRegistry
   :members:

.. doxygenclass:: knot::ShaderSource
   :members:

.. doxygenclass:: knot::Shader
   :members:

.. doxygenclass:: knot::PbrShader
   :members:

.. doxygenclass:: knot::AlphaShader
   :members:

Materials
---------

.. doxygenclass:: knot::Material
   :members:

.. doxygenclass:: knot::TextureMaterial
   :members:

.. doxygenclass:: knot::AlphaMaterial
   :members:

.. doxygenclass:: knot::PbrMaterial
   :members:

Scene types
-----------

.. doxygenstruct:: knot::SubMesh
   :members:

.. doxygenstruct:: knot::Model
   :members:

.. doxygenclass:: knot::Transform
   :members:

.. doxygenclass:: knot::Object
   :members:

.. doxygenclass:: knot::Light
   :members:

.. doxygenclass:: knot::PbrPointLight
   :members:
   
.. doxygenclass:: knot::DirLight
   :members:

Global functions
----------------

.. doxygenfunction:: knot::setSourceRoot

.. doxygenfunction:: knot::getSourceRoot

.. doxygenfunction:: knot::setEngineRoot

.. doxygenfunction:: knot::getEngineRoot

