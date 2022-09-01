<div align="center">
<img src="./documentation/Peano-logo.png"/>
</div>

## <u>Table of Contents</u>
- [<u>Table of Contents</u>](#utable-of-contentsu)
- [<u>General Information</u>](#ugeneral-informationu)
- [<u>Availability</u>](#uavailabilityu)
- [<u>Supported Platforms</u>](#usupported-platformsu)
- [<u>Dependencies and Prerequisites</u>](#udependencies-and-prerequisitesu)
- [<u>How to Build and Run (using GNU autotools)</u>](#uhow-to-build-and-run-using-gnu-autotoolsu)
  - [Setup your build environment](#setup-your-build-environment)
  - [Configure your build](#configure-your-build)
  - [Compilation](#compilation)
  - [Running a Simulation](#running-a-simulation)
  - [Cleanup](#cleanup)
- [<u>Guidebook</u>](#uguidebooku)

## <u>General Information</u>
Peano is an open source framework for solvers on dynamically adaptive
Cartesian meshes.

Its core is built with C++, but many tools around it are written in Python.
Peano  is based upon the fact that spacetrees, a generalisation of the 
classical octree concept, yield a cascade of adaptive Cartesian grids. 
Consequently, any spacetree traversal is equivalent to an element-wise 
traversal of the hierarchy of the adaptive Cartesian grids. The software 
Peano  realises such a grid traversal and storage algorithm, and it provides 
hook-in points for applications performing per-element, per-vertex, and so 
forth operations on the grid. It also provides interfaces for dynamic load balancing, sophisticated geometry representations, and other features.

## <u>Availability</u>
All software is available as open source through www.peano-framework.org.
This page also holds extensive documentation (written, extracted from 
source code and as video tutorials).

## <u>Supported Platforms</u>
We test and maintain Peano for Linux only.
If you prefer Windows or MacOS, it should work as long as you provide the mandatory tools, but we won't be able to help.

## <u>Dependencies and Prerequisites</u>
Peano's core is plain C++17/C++20 code. 
We however use a whole set of tools around it.
The most important ones are:

* C++17/C++20-compatible C++ compiler (required).
* Python3 (optional; not required if you work only with the C++ baseline).
* MPI (optional). MPI's multithreaded support is required.
* Intel's Threading Build Blocks, C++ threads or OpenMP (optional).
* GNU autotools (automake) to set up the system (required).
* The Visualization Toolkit (VTK) if you want to use the built-in visualisation facilities (optional).
* Doxygen if you want to create HTML pages of PDFs of the in-code documentation.
* pdflatex if you want to build the guidebook (Peano's documentation) yourself (optional).

A detailed list can be found in the guidebook which we ship with the
download (TeX sources). It is also available through the webpage where
it is built in regular intervals.

## <u>How to Build and Run (using GNU autotools)</u>
Peano's build process relies on the autotool chain.
It yields a couple of libraries and executables. 
Most codes then use Peano's Python API to build an application on top of the libraries.
You obviously also can create a sole C++ application.
The Python API actually yields such applications, too, i.e. Python is not required to run the simulations but only to construct them.

### Setup your build environment
If you download a Peano archive, you can skip this step.
If you work against the Peano repository, you however have to run these steps:

```shell
libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing
```

### Configure your build
We rely on configure to set up the build system.
Use configure's help to find out which options you want to use.

```shell
./configure --help
./configure --my-options
```

Some exemplary configure commands are given below. For detailed information please refer to the guidebook.

* Intel
```shell
./configure --with-multithreading=omp --enable-exahype --enable-loadbalancing --enable-blockstructured --enable-fenv CC=icx CXX=icpx CXXFLAGS="-O3 -std=c++20 -fiopenmp" LDFLAGS="-fiopenmp"
./configure --with-multithreading=omp --with-gpu=omp --enable-exahype --enable-loadbalancing --enable-blockstructured --enable-fenv CC=icx CXX=icpx CXXFLAGS="-O3 -std=c++20 -fiopenmp -fopenmp-targets=spir64" LDFLAGS="-fiopenmp"
```

* LLVM
```shell
./configure --with-multithreading=omp --with-gpu=omp --enable-exahype --enable-loadbalancing --enable-blockstructured --enable-fenv CC=clang CXX=clang++ CXXFLAGS="-O3 -std=c++17 -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=sm_86"
```

* NVHPC
```shell
./configure --with-multithreading=omp --with-gpu=omp --enable-exahype --enable-loadbalancing --enable-blockstructured --enable-fenv CC=nvc++ CXX=nvc++ CXXFLAGS="-O4 -std=c++20 -mp=gpu -gpu=cc86" LDFLAGS="-mp=gpu -gpu=cc86"
```

### Compilation
```shell
make
make install
```
Invoke make -jX to use X threads to build the code. Peano is mainly a 
library collection, i.e. you might want to skip the install command and
work directly against the libraries built in the source directory.

### Running a Simulation
```shell
export PYTHONPATH=$PEANO_DIR/python
cd applications/exahype2/euler
python3 example-scripts/finitevolumes.py -maxh 0.05
```

### Cleanup
```shell
make clean
make distclean
make maintainer-clean
```

## <u>Guidebook</u>
For more details regarding build configuration and features of the
software, take a look at the Peano documentation. You can build a PDF version of the guidebook via:

```shell
cd documentation
scons
```
