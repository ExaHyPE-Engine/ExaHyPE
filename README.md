<div align="center">
<img src="./documentation/logos/Peano-logo.png"/>
</div>

# General Information
Peano is an open source framework for solvers on dynamically adaptive
Cartesian meshes.

Its core is built with C++, but many tools around it are written in Python.
Peano is based upon the fact that spacetrees, a generalisation of the
classical octree concept, yield a cascade of adaptive Cartesian grids.
Consequently, any spacetree traversal is equivalent to an element-wise
traversal of the hierarchy of the adaptive Cartesian grids. The software
Peano realises such a grid traversal and storage algorithm, and it provides
hook-in points for applications performing per-element, per-vertex, and so
forth operations on the grid. It also provides interfaces for dynamic
load balancing, sophisticated geometry representations, and other features.

# Availability
All software is available as open source through www.peano-framework.org.
This page also holds extensive [online documentation](https://hpcsoftware.pages.gitlab.lrz.de/Peano/html/index.html) (written, extracted from
source code and as video tutorials).

# Supported Platforms
We test and maintain Peano for Linux only.
If you prefer Windows or MacOS, it should work as long as you provide
the mandatory tools, but we won't be able to help.

# Dependencies and Prerequisites
Peano's core is plain C++17/C++20 code.
We however use a whole set of tools around it.
The most important ones are:

* C++17/C++20-compatible C++ compiler (required).
* GNU autotools (automake) to set up the system (required).
* CMake > 3.20 (required).
* Python3 (optional but recommended; not required if you work only with the C++ baseline).
* MPI (optional). MPI's multithreaded support is required.
* Intel's Threading Build Blocks, C++ threads or OpenMP for multithreading (optional).
* CUDA, ROCm, OpenSYCL or Intel oneAPI for GPU offloading (optional).
* The Visualization Toolkit (VTK) if you want to use the built-in visualisation facilities (optional).
* Doxygen if you want to create HTML pages of the documentation (optional).

# Quickstart
```shell
python3 configure.py && make -j && make check
```

Invoke `make -jX` to use `X` threads to build the code.
Peano is mainly a library collection, i.e.,
you might want to skip the install command and work directly against the libraries built in the source directory.

Use `python3 configure.py --help` to see the available options to configure and build.

# Build Process
Peano's build process relies on GNU autotools or CMake.
The build yields a couple of libraries and executables.
Most codes then use Peano's Python API to build an application on top of the libraries.
You also can create a sole C++ application.
The Python API yields such applications, too, i.e.,
Python is not required to run the simulations but only to construct them.

## CMake
```shell
mkdir build && cd build && cmake .. && make -j && make test
```

It is recommended to use `ccmake`.
With this, you can see all available options to enable/disable.
For detailed information on each flag please refer to the documentation.

## Autotools
```shell
libtoolize; aclocal; autoconf; autoheader; cp src/config.h.in .; automake --add-missing

./configure && make -j && make check
```

# Containerisation
We maintain a collection of Docker and Apptainer [containers](./containers/README.md) with all required dependencies. Refer to the documentation on how to use them.

# Documentation
The [documentation](./documentation/) consists mostly of Doxygen which can be built locally with

```shell
doxygen documentation/Doxyfile
```
