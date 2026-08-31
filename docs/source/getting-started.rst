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

- A graphics card that supports OpenGL 4.3 or later

Building
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note::

   Not Engine has currently only been tested on Linux.
   Operation and stability on Windows and macOS are not guaranteed.

Linux (Recommended)
--------------------------------

CMake, Ninja (or Make), and the ``glfw3`` package are required to build Not Engine.

Debian/Ubuntu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt update
   sudo apt install cmake ninja-build libglfw3-dev build-essential

Fedora
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo dnf install cmake ninja-build glfw-devel @development-tools

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

Windows
--------------------------------

CMake and GLFW are required to build Not Engine. Visual Studio 2022 or 2026 with C++ development tools is recommended.

- **CMake**: Available from `cmake.org <https://cmake.org/download/>`_

- **GLFW**:

  - Using vcpkg (recommended):

    .. code-block:: powershell

       vcpkg install glfw3:x64-windows

  - Official binaries:
    Download the pre-built Windows binaries from
    `glfw.org <https://www.glfw.org/download.html>`_

.. tip::

   If you installed GLFW using vcpkg, run ``vcpkg integrate install`` so CMake can automatically locate installed packages.

After installing the required tools and dependencies, run the following commands to configure, build, and install the library using CMake presets.

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

.. toctree::
   :maxdepth: 2
