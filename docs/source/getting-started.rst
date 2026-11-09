================
Getting Started
================

This document provides instructions for building Not Engine, running the demos,
and an overview of its basic features for users who are new to Not Engine.

--------------------------------
Installation
--------------------------------

Not Engine does not currently provide pre-built binaries, so you must build it yourself.

Requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- A graphics card and drivers that supports OpenGL 4.3 or later
- CMake
- Git
- A C/C++ compiler

Building
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note::

   Not Engine has currently only been tested on Linux.
   Operation and stability on Windows and macOS are not guaranteed.

Linux (Recommended)
--------------------------------

CMake, Git, Ninja (or Make), and a C/C++ compiler are required to build Not Engine.

Not Engine manages third-party dependencies using CMake.
If the required dependencies are already installed through a package manager, Not Engine uses them directly.
Otherwise, CMake downloads them using Git during configuration.

Debian/Ubuntu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt update
   sudo apt install cmake ninja-build git build-essential

Fedora
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo dnf install cmake ninja-build git @development-tools

Arch Linux
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo pacman -Syu
   sudo pacman -S cmake ninja git base-devel

After installing the required packages, run the following commands to configure, build, and install the library using CMake presets.

.. code-block:: bash

   # Configure (Ninja Release)
   cmake --preset ninja-release

   # Build the library in Release mode
   cmake --build --preset ninja-release-notengine

   # Install
   sudo cmake --install build/ninja-release

To build and run the demos, run the following commands.

.. code-block:: bash

   # Build the demos
   cmake --build --preset ninja-release-all

   # Run the default demo
   ./build/ninja-release/demo

   # Scene file rendering demo
   ./build/ninja-release/scene_render

   # Benchmark
   ./build/ninja-release/benchmark

The demo build preset (``ninja-release-all``) automatically builds the library dependency as well,
so you do not need to build or install the library beforehand when you only want to run the demos.

.. tip::

   You can also use Unix Makefiles presets (``make-release``, ``make-release-notengine``, ``make-release-all``) or Debug presets (``ninja-debug``, etc.).

.. tip::
   
   If the build fails on Linux, the cause may be a missing graphics driver, graphics development package, or Linux window-system backend dependency such as X11 or Wayland.

   You can install the required dependencies individually, or install libglfw3-dev, which usually pulls in the packages needed to build GLFW on a typical Linux desktop system.

Windows
--------------------------------

CMake, Git, and Visual Studio 2022 or 2026 with C++ development tools are recommended to build Not Engine.

Third-party dependencies may be installed beforehand using a package manager
such as vcpkg. CMake will use an installed dependency if it can be located through ``find_package()``.

If a required dependency cannot be found, CMake automatically downloads and
builds it using ``FetchContent``. Git is required when dependencies need to be fetched.

Fetched dependencies are stored under ``_deps`` in the corresponding build directory.

For example:

.. code-block:: text

   build/vs2022/_deps/
   build/vs2026/_deps/

After installing the required tools, run the following commands to configure, build, and install the library using CMake presets.

.. code-block:: powershell

   # Configure for Visual Studio 2022 (or vs2026)
   cmake --preset vs2022

   # Build the library in Release mode
   cmake --build --preset vs2022-release-notengine

   # Install (administrator privileges required)
   cmake --install build/vs2022 --config Release

To build and run the demos, run the following commands.

.. code-block:: powershell

   # Build the demos
   cmake --build --preset vs2022-release-all

   # Run the default demo
   .\build\vs2022\Release\demo.exe

   # Scene file rendering demo
   .\build\vs2022\Release\scene_render.exe

   # Benchmark
   .\build\vs2022\Release\benchmark.exe

The demo build preset (``vs2022-release-all``) automatically builds the library dependency as well,
so you do not need to build or install the library beforehand when you only want to run the demos.

Dependency Management
--------------------------------

Not Engine manages supported third-party dependencies through CMake.

During configuration, CMake first attempts to locate an existing installation
of each dependency using ``find_package()``. This allows dependencies installed
through system package managers or tools such as vcpkg to be used directly.

If a dependency cannot be found, Not Engine uses CMake's ``FetchContent`` module to download and build the required library automatically.

Dependencies downloaded through ``FetchContent`` are stored under ``_deps`` in the corresponding build directory.

Git is required to download dependencies through ``FetchContent``.

Automatic dependency fetching can be disabled by setting
``NOTENGINE_FETCH_DEPENDENCIES`` to ``OFF``.

.. code-block:: bash

   cmake -S . -B build -DNOTENGINE_FETCH_DEPENDENCIES=OFF

When automatic fetching is disabled, the required dependencies must be
installed and discoverable by CMake before configuring Not Engine.

CMake Presets Reference
--------------------------------

Not Engine provides various CMake presets configured in ``CMakePresets.json``:

Configure Presets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``ninja-debug``: Configure using Ninja (Debug) -> ``build/ninja-debug``
- ``ninja-release``: Configure using Ninja (Release) -> ``build/ninja-release``
- ``ninja-multi``: Configure using Ninja Multi-Config -> ``build/ninja-multi``
- ``make-debug``: Configure using Unix Makefiles (Debug) -> ``build/make-debug``
- ``make-release``: Configure using Unix Makefiles (Release) -> ``build/make-release``
- ``vs2022``: Configure using Visual Studio 2022 (x64) -> ``build/vs2022``
- ``vs2026``: Configure using Visual Studio 2026 (x64) -> ``build/vs2026``

Build Presets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``<preset>-notengine``: Builds only the ``notengine`` core static library (e.g., ``ninja-release-notengine``, ``vs2022-release-notengine``)
- ``<preset>-all``: Builds all demo executables (``demo``, ``benchmark``, ``scene_render``) and the engine library (e.g., ``ninja-release-all``, ``vs2022-release-all``)

macOS
--------------------------------

.. warning::

   Due to Apple's restrictive platform policies, macOS supports OpenGL only up to version 4.1. Not Engine requires OpenGL 4.3 or later and therefore cannot be used on macOS.

--------------------------------
Quick Start
--------------------------------

The easiest way to start a new project with Not Engine is to copy ``demo.cpp``,
which is released under the **Unlicense** (Public Domain) specifically so you can
use it as a zero-boilerplate template.

Paste the code below into ``src/main.cpp`` in the project root.

.. dropdown:: View full code for demo.cpp
   :icon: code

   .. literalinclude:: ../../demo/demo.cpp
      :language: cpp
      :caption: demo/demo.cpp

If you have built and linked the Not Engine yourself, this code will work without any modifications.

Asset root
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As you look through the demo code, you may come across a function called :cpp:func:`knot::getAssetRoot`. This function returns the base directory path where the engine source code and assets are located.

By default, it points to the root directory of the Not Engine source code. You can concatenate this base path with relative resource paths to access project files:

.. code-block:: cpp

   std::string imagePath = knot::getAssetRoot() + "assets/image.png";
   std::string shaderPath = knot::getAssetRoot() + "shaders/alpha.vert";

Default Behavior & CMake Configuration
--------------------------------------

By default, the asset root is configured during CMake setup to point to ``${CMAKE_SOURCE_DIR}``. 

If you want to customize this path at build time, set the ``NOTENGINE_ASSET_ROOT`` variable in your CMake configuration:

.. code-block:: cmake

   # Set the asset root to the project root within CMake
   set(NOTENGINE_ASSET_ROOT "${CMAKE_SOURCE_DIR}" CACHE PATH "Path to the asset root directory")
   
   target_compile_definitions(notengine PUBLIC 
       NOTENGINE_ASSET_ROOT=\"${NOTENGINE_ASSET_ROOT}\"
   )

You can also override it via the command line:

.. code-block:: bash

   cmake -B build -DNOTENGINE_ASSET_ROOT="/path/to/your/project"

Changing Asset Root at Runtime
------------------------------

If you need to change the asset root dynamically at runtime, use :cpp:func:`knot::setAssetRoot`:

.. code-block:: cpp

   #include <knot/resources.h>

   // Set a new base path at runtime
   knot::setAssetRoot("/path/to/custom/root/");

.. note::

   :cpp:func:`knot::getAssetRoot` guarantees that the returned path string always ends with a trailing slash (``/``), making path concatenation simple and safe.

.. warning::

   This document is currently a work in progress. Some sections may be incomplete or subject to change.

.. toctree::
   :maxdepth: 2
