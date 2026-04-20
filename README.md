[![SQAaaS badge shields.io](https://img.shields.io/badge/sqaaas%20software-silver-lightgrey)](https://api.eu.badgr.io/public/assertions/uhyUShBWT1Wfd0Vx3EoMrQ "SQAaaS silver badge achieved")

[![SQAaaS badge](https://github.com/EOSC-synergy/SQAaaS/raw/master/badges/badges_150x116/badge_software_silver.png)](https://api.eu.badgr.io/public/assertions/uhyUShBWT1Wfd0Vx3EoMrQ "SQAaaS silver badge achieved")

<div align="center" style="margin-bottom: 30px;">
  <img src="./documentation/Peano-logo.png"/>
</div>

<div align="center" style="text-align: center;"><b>
  <a href="http://www.peano-framework.org">Website</a> •
  <a href="https://tum-i5.github.io/ExaHyPE-Documentation/">Documentation</a> •
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

# Availability

All software is available as open source through [www.peano-framework.org](http://www.peano-framework.org).

# Installation (Dependencies and Prerequisites)

Get started with Peano following our [Installation Guide](https://hpcsoftware.pages.gitlab.lrz.de/Peano/de/dd9/page_installation_home.html).

# Environments

We maintain packaged [environments](environments)
for Docker, Apptainer, and [Spack](https://github.com/spack/spack/wiki) to simplify
setup on workstations and HPC systems.

# Contributing

Contributions are **welcome and very much appreciated**.
Please refer [to our Contribution Guide](CONTRIBUTING.md) for more details.

# Code of Conduct

We follow a [Code of Conduct](CODE_OF_CONDUCT.md).
Please follow the rules when participating in our community.

# License

Modified BSD License, see [LICENSE](LICENSE).
