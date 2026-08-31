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

CMake and the ``glfw3`` package are required to build Not Engine.

Debian/Ubuntu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo apt update
   sudo apt install cmake libglfw3-dev build-essential

Fedora
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo dnf install cmake glfw-devel @development-tools

After installing the required packages, run the following commands to build and install the library.

.. code-block:: bash

   # Build the library in Release mode
   ./script/library_release_build.sh

   # Install
   sudo cmake --install build

To build and run the demos, run the following commands.

.. code-block:: bash

   # Build the demos
   ./script/demo_release_build.sh

   # Run the default demo
   ./build/demo

   # Scene file rendering demo
   ./build/scene_render

   # Benchmark
   ./build/benchmark

The demo build script automatically builds the library when necessary,
so you do not need to build or install the library beforehand when you only want to run the demos.

Windows
--------------------------------

CMake and GLFW are required to build Not Engine.

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

After installing the required tools and dependencies, run the following commands to build and install the library.

.. code-block:: powershell

   # Build the library in Release mode
   .\script\library_release_build.bat

   # Install (administrator privileges required)
   cmake --install build --config Release

To build and run the demos, run the following commands.

.. code-block:: powershell

   # Build the demos
   .\script\demo_release_build.bat

   # Run the default demo
   .\build\Release\demo.exe

   # Scene file rendering demo
   .\build\Release\scene_render.exe

   # Benchmark
   .\build\Release\benchmark.exe

The demo build script automatically builds the library when necessary,
so you do not need to build or install the library beforehand when you only want to run the demos.

macOS
--------------------------------

.. warning::

   Due to Apple's restrictive platform policies, macOS supports OpenGL only up to version 4.1. Not Engine requires OpenGL 4.3 or later and therefore cannot be used on macOS.

.. toctree::
   :maxdepth: 2
