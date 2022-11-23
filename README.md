<div align="center">
<img src="./documentation/Peano-logo.png"/>
</div>

# General Information
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

# Availability
All software is available as open source through www.peano-framework.org.
This page also holds extensive documentation (written, extracted from 
source code and as video tutorials).

# Supported Platforms
We test and maintain Peano for Linux only.
If you prefer Windows or MacOS, it should work as long as you provide the mandatory tools, but we won't be able to help.

# Dependencies and Prerequisites
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

# How to Build and Run (using GNU autotools)
Peano's build process relies on the autotool chain.
It yields a couple of libraries and executables.
Most codes then use Peano's Python API to build an application on top of the libraries.
You obviously also can create a sole C++ application.
The Python API actually yields such applications, too, i.e., Python is not required to run the simulations but only to construct them.

## Setup your build environment
If you download a Peano archive, you can skip this step.
If you work against the Peano repository, you however have to run these steps:

```shell
libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing
```

## Configure your build using a toolchain
We rely on `configure` to set up the build system.
Use `configure`'s help to find out which options you want to use.

```shell
./configure --help
./configure --my-options
```

Some quickstart configure commands for different toolchains are given below. For detailed information please refer to the guidebook in the documentation.

### Intel OneAPI toolchain
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-multithreading=omp \
  CC=icx CXX=icpx \
  CXXFLAGS="-O3 -std=c++20 -W -Wall -Wextra -Wno-unknown-attributes -fiopenmp" \
  LDFLAGS="-fiopenmp"
```

### Intel OneAPI toolchain with OpenMP GPU
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-gpu=omp \
  --with-multithreading=omp \
  CC=icx CXX=icpx \
  CXXFLAGS="-O3 -std=c++20 -W -Wall -Wextra -Wno-unknown-attributes -fiopenmp -fopenmp-targets=spir64" \
  LDFLAGS="-fiopenmp"
```

### Clang toolchain
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-multithreading=omp \
  CC=clang CXX=clang++ \
  CXXFLAGS="-O3 -std=c++17 -W -Wall -Wextra -Wno-unknown-attributes -fopenmp"
```

### Clang toolchain with OpenMP GPU
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-gpu=omp \
  --with-multithreading=omp \
  CC=clang CXX=clang++ \
  CXXFLAGS="-O3 -std=c++17 -W -Wall -Wextra -Wno-unknown-attributes -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=<gpu-arch>"
```
where `<gpu-arch>` is either `cc50`, `cc52`, `cc53`, `cc60`, `cc61`, `cc62`, `cc70`, `cc72`, `cc75`, `cc80`, `cc86` or `cc90`.

### Clang toolchain with SYCL GPU

```shell
./configure
  --enable-exahype
  --enable-loadbalancing
  --enable-blockstructured
  --with-gpu=sycl
  --with-multithreading=sycl
  CC=clang CXX="clang++ --gcc-toolchain=/cosma/local/gcc/11.1.0/bin/gcc"
  CXXFLAGS="-O3 -Xsycl-target-frontend -O3 -fgpu-inline-threshold=100000 -Wno-unknown-cuda-version -Wno-deprecated-declarations -Wno-linker-warnings -std=c++20 -Wno-unknown-attributes -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend --cuda-gpu-arch=sm_80"
  LDFLAGS="-Xsycl-target-frontend -O3 -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend --cuda-gpu-arch=sm_80 -lstdc++fs"
```

### NVIDIA NVC++ toolchain with OpenMP GPU
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-gpu=omp \
  --with-multithreading=omp \
  CC=nvc++ CXX=nvc++ \
  CXXFLAGS="-O4 -std=c++20 -W -Wall -Wextra -mp=gpu -gpu=<gpu-arch>" \
  LDFLAGS="-mp=gpu -gpu=<gpu-arch>"
```
where `<gpu-arch>` is either `cc70`, `cc72`, `cc75`, `cc80`, `cc86` or `cc90`.

If you want to use the CUDA API you will need to add `-cuda` to the `CXXFLAGS` and the `LDFLAGS`.

### NVIDIA NVC++ toolchain with C++ GPU
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-gpu=cpp \
  --with-multithreading=cpp \
  CC=nvc++ CXX=nvc++ \
  CXXFLAG="-O4 -std=c++20 -W -Wall -Wextra -mp=gpu -gpu=<gpu-arch>" \
  LDFLAGS="-mp=gpu -gpu=<gpu-arch>"
```
where `<gpu-arch>` is either `cc70`, `cc72`, `cc75`, `cc80`, `cc86` or `cc90`.

### AMD toolchain with OpenMP GPU
```shell
./configure \
  --enable-exahype \
  --enable-loadbalancing \
  --enable-blockstructured \
  --with-gpu=omp \
  --with-multithreading=omp \
  CC=clang CXX=clang++ \
  CXXFLAGS="-O3 -std=c++20 -W -Wall -Wextra -Wno-unknown-attributes -fopenmp -O3 -std=c++20 --offload-arch=<gpu-arch>" \
  LDFLAGS="-fopenmp --offload-arch=<gpu-arch> -lstdc++fs"
```
where `<gpu-arch>` is either `gfx906` (MI 50), `gfx908` (MI 100), or `gfx90a` (MI 200 series).

## Compilation
```shell
make
make install
```
Invoke make -jX to use X threads to build the code. Peano is mainly a 
library collection, i.e. you might want to skip the install command and
work directly against the libraries built in the source directory.

## Running a Simulation
```shell
export PYTHONPATH=$PEANO_DIR/python
cd applications/exahype2/euler
python3 example-scripts/finitevolumes.py -maxh 0.05
```

## Cleanup
```shell
make clean
make distclean
make maintainer-clean
```

# Documentation and Guidebook
For more details regarding build configuration and features of the
software, take a look at the Peano documentation. You can build a PDF version of the guidebook via:

```shell
cd documentation
scons
```
