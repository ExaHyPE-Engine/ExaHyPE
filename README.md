[![SQAaaS badge shields.io](https://img.shields.io/badge/sqaaas%20software-bronze-e6ae77)](https://api.eu.badgr.io/public/assertions/vg8PP-0FQlafJ-L-cRs_Xw "SQAaaS bronze badge achieved")

<div align="center" style="margin-bottom: 30px;">
  <img src="./documentation/Peano-logo.png"/>
</div>

<div align="center" style="text-align: center;"><b>
  <a href="http://www.peano-framework.org">Website</a> •
  <a href="https://hpcsoftware.pages.gitlab.lrz.de/Peano/index.html">Documentation</a> •
  <a href="https://gitlab.lrz.de/hpcsoftware/Peano/-/tags">Tags</a> •
  <a href="https://gitlab.lrz.de/hpcsoftware/Peano/-/releases">Releases</a>
</b></div>

# Overview

Peano is an open-source framework for solvers on dynamically adaptive
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

# Installation

Get started with Peano following our [Installation Guide](https://hpcsoftware.pages.gitlab.lrz.de/Peano/de/dce/page_installation.html).

# Availability

All software is available as open source through [www.peano-framework.org](http://www.peano-framework.org).

# Dependencies and Prerequisites

Peano's core is plain C++17/C++20 code.
We however use a whole set of tools around it.

* C++17/C++20-compatible C++ compiler (required).
* GNU Autotools (automake) to set up the system (required).
* CMake > 3.20 (required).
* Python3 (optional but recommended; not required if you work only with the C++ baseline).
* MPI (optional). MPI's multithreaded support is required.
* Intel's Threading Build Blocks, C++ threads or OpenMP for multithreading (optional).
* CUDA, ROCm, OpenSYCL or Intel oneAPI for GPU offloading (optional).
* The Visualization Toolkit (VTK) if you want to use the built-in visualisation facilities (optional).
* HDF5 (optional).
* NetCDF (optional).
* LIBXSMM (optional).
* Doxygen if you want to create HTML pages of the documentation (optional).

# Docker

We maintain a collection of [Dockerfiles](https://hpcsoftware.pages.gitlab.lrz.de/Peano/df/dbb/page_installation_with_docker.html).

# Apptainer

We maintain a collection of [Apptainer Definition Files](https://hpcsoftware.pages.gitlab.lrz.de/Peano/db/d56/page_installation_with_apptainer.html).

# Spack

To considerably alleviate the installation process of Peano, especially on HPC systems,
we provide [packages](https://hpcsoftware.pages.gitlab.lrz.de/Peano/d6/db6/page_installation_with_spack.html) that rely on [Spack](https://github.com/spack/spack/wiki).

# Contributing

Contributions are **welcome and very much appreciated**.
Please refer [to our Contribution Guide](CONTRIBUTING.md) for more details.

# Code of Conduct

We follow a [Code of Conduct](CODE_OF_CONDUCT.md).
Please follow the rules when participating in our community.

# License

Modified BSD License, see [LICENSE](LICENSE).
