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
Peano  realises such a grid traversal and storage algorithm, and it provides
hook-in points for applications performing per-element, per-vertex, and so
forth operations on the grid. It also provides interfaces for dynamic load balancing,
sophisticated geometry representations, and other features.

# Availability
All software is available as open source through www.peano-framework.org.
This page also holds extensive documentation (written, extracted from
source code and as video tutorials).

# Supported Platforms
We test and maintain Peano for Linux only.
If you prefer Windows or MacOS, it should work as long as you provide the mandatory tools,
but we won't be able to help.

# Dependencies and Prerequisites
Peano's core is plain C++17/C++20 code.
We however use a whole set of tools around it.
The most important ones are:

* C++17/C++20-compatible C++ compiler (required).
* Python3 (optional; not required if you work only with the C++ baseline).
* MPI (optional). MPI's multithreaded support is required.
* Intel's Threading Build Blocks, C++ threads or OpenMP (optional).
* GNU autotools (automake) to set up the system (required).
* CMake > 3.20 (required).
* The Visualization Toolkit (VTK) if you want to use the built-in visualisation facilities (optional).
* Doxygen if you want to create HTML pages of PDFs of the in-code documentation.
* pdflatex if you want to build the guidebook (Peano's documentation) yourself (optional).

# Toolchains
Peano's build process relies on GNU autotools or CMake.
The build yields a couple of libraries and executables.
Most codes then use Peano's Python API to build an application on top of the libraries.
You obviously also can create a sole C++ application.
The Python API actually yields such applications, too, i.e.,
Python is not required to run the simulations but only to construct them.

You can find Docker images for each toolchain we support at [Dockerhub](https://hub.docker.com/u/peanoframework).
In order to run a specific toolchain Docker image you can execute the following command:

```shell
docker pull peanoframework/<toolchain>
docker run -it --rm --gpus=all --privileged peanoframework/<toolchain> /bin/bash
```

where `<toolchain>` needs to replaced by the toolchain you want to use. When you run the Docker container interactively using the command above, you can quickly check whether GPU offloading is supported on your system by executing `./check_XXX_offload`.

A collection of configuration commands using different toolchains can be found in the interactive [configure script](./.continuous_integration/configure.sh) which can be called from the root directory as follows:

```shell
bash .continuous_integration/configure.sh
```

For CMake, it is recommended to use `ccmake`.

For a detailed information on each flag please refer to the guidebook.

Invoke `make -jX` to use `X` threads to build the code. Peano is mainly a
library collection, i.e., you might want to skip the install command and
work directly against the libraries built in the source directory.

# Running a Simulation
```shell
cd applications/exahype2/acoustic-wave
python3 acoustic-wave.py
./acoustic-wave-application-*
```

# Cleanup
```shell
make clean
make distclean
make maintainer-clean
```

# Documentation and Guidebook
A detailed list of features can be found in the [guidebook](./documentation/) which we ship with the
repository (TeX sources).
You can build a PDF version of the guidebook via:

```shell
cd documentation
make
```
