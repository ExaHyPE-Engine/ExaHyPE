<div align="center">
<img src="./documentation/logos/Peano-logo.png"/>
</div>

# General information
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

# Supported platforms
We test and maintain Peano for Linux only.
If you prefer Windows or MacOS, it should work as long as you provide the mandatory tools,
but we won't be able to help.

# Dependencies and prerequisites
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

A detailed list can be found in the guidebook which we ship with the
download (TeX sources). It is also available through the webpage where
it is built in regular intervals.

# Setup your build environment with a toolchain
If you download a Peano archive, you can skip this step.
If you work against the Peano repository, you however have to run some steps.
Peano's build process relies on the autotool chain or CMake.
It yields a couple of libraries and executables.
Most codes then use Peano's Python API to build an application on top of the libraries.
You obviously also can create a sole C++ application.
The Python API actually yields such applications, too, i.e.,
Python is not required to run the simulations but only to construct them.

A collection of configuration commands using different toolchains can be found [here](./.continuous_integration).

For a detailed information on each flag please refer to the guidebook in the documentation.

## Compilation
```shell
make
make install
```
Invoke make -jX to use X threads to build the code. Peano is mainly a
library collection, i.e., you might want to skip the install command and
work directly against the libraries built in the source directory.

# Running a simulation
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

# Documentation and guidebook
For more details regarding build configuration and features of the
software, take a look at the Peano documentation. You can build a PDF version of the guidebook via:

```shell
cd documentation
make
```
