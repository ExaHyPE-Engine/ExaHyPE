# Copyright 2013-2023 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import os

import spack.variant
from spack.package import *


class Exahype2(CMakePackage, CudaPackage):
    """ExaHyPE 2 - ExaHyPE is an open source simulation engine to solve
    hyperbolic PDE systems. It is built on top of dynamically
    adaptive Cartesian meshes and offers support for Finite Volume,
    Runge-Kutta Discontinuous Galerkin and ADER-DG discretisations.
    ExaHyPE is written in a way that most computer science aspects
    as well as most of the numerics are hidden away from the user:
    Users plug in user functions for their PDE formulation
    (such as flux functions and eigenvalues) into the engine and
    then delegate all further work to ExaHyPE.
    """

    homepage = "www.peano-framework.org"
    url = "https://gitlab.lrz.de/hpcsoftware/Peano"
    git = "https://gitlab.lrz.de/hpcsoftware/Peano.git"

    maintainers("hpcsoftware")

    version("p4", branch="p4")

    # Config options
    variant(
        "dimensions",
        default="2,3",
        values=("2", "3"),
        multi=True,
        description="Dimensionality",
    )
    variant(
        "build_type",
        default="Debug;Release;Asserts;Trace;Stats",
        values=("Debug", "Release", "Asserts", "Trace", "Stats", "Debug;Release;Asserts;Trace;Stats"),
        multi=False,
        description="Build type",
    )

    variant("mpi", default=False, description="Build with MPI support")
    variant("tracer", default=False, description="Build with particle tracer support")
    variant("hdf5", default=False, description="Build with HDF5 support")
    variant("netcdf", default=False, description="Build with NetCDF support")

    depends_on("cmake", type="build")
    depends_on('libxsmm+generator')

    depends_on("mpi", when="+mpi")

    depends_on("hdf5 +shared +threadsafe ~mpi", when="~mpi +hdf5")
    depends_on("hdf5 +shared +threadsafe +mpi", when="+mpi +hdf5")

    depends_on("netcdf-c +shared ~mpi", when="~mpi +netcdf")
    depends_on("netcdf-c +shared +mpi", when="+mpi +netcdf")

    depends_on('py-pip')
    depends_on('py-numpy')
    depends_on('py-scipy')
    depends_on('py-matplotlib')
    depends_on('py-sympy')
    depends_on('py-mpmath')
    depends_on('py-jinja2')

    variant("omp", default=False, description="Build with OpenMP multithreading support")
    variant("sycl", default=False, description="Build with SYCL multithreading support")
    variant("cpp", default=False, description="Build with std::par multithreading support")
    variant("tbb", default=False, description="Build with TBB multithreading support")

    conflicts("+omp", when="+sycl", msg="OpenMP and SYCL support are exclusive")
    conflicts("+omp", when="+cpp", msg="OpenMP and std::par support are exclusive")
    conflicts("+sycl", when="+cpp", msg="SYCL and std::par support are exclusive")
    conflicts("+tbb", when="+cpp", msg="TBB and std::par support are exclusive")
    conflicts("+tbb", when="+sycl", msg="TBB and SYCL support are exclusive")
    conflicts("+tbb", when="+omp", msg="TBB and OpenMP support are exclusive")

    depends_on("cuda@11:", when="+cuda")

    conflicts(
        "cuda_arch=none",
        when="+cuda",
        msg="A value for cuda_arch must be specified. Add cuda_arch=XX",
    )

    variant(
        "gpu_backend",
        default="omp",
        description="GPU accelerator backend",
        values=("omp", "cpp", "sycl"),
        when="+cuda",
    )


    def cmake_args(self):
        args = [
            "-DENABLE_LOADBALANCING=ON",
            "-DENABLE_BLOCKSTRUCTURED=ON",
            "-DENABLE_EXAHYPE=ON",
            "-DWITH_LIBXSMM=ON",
            self.define_from_variant("WITH_DIMENSIONS", "dimensions"),
            self.define_from_variant("CMAKE_BUILD_TYPE", "build_type"),
            self.define_from_variant("WITH_MPI", "mpi"),
            self.define_from_variant("ENABLE_PARTICLES", "tracer"),
        ]

        if self.spec.satisfies("+omp"):
            args.append("-DWITH_MULTITHREADING=omp")
        if self.spec.satisfies("+cpp"):
            args.append("-DWITH_MULTITHREADING=cpp")
        if self.spec.satisfies("+sycl"):
            args.append("-DWITH_MULTITHREADING=sycl")
        if self.spec.satisfies("+tbb"):
            args.append("-DWITH_MULTITHREADING=tbb")

        if self.spec.satisfies("+cuda"):
            cuda_arch = self.spec.variants["cuda_arch"].value[0]
            gpu_backend = self.spec.variants["gpu_backend"].value
            args.append(f"-DWITH_GPU={gpu_backend}")
            args.append(f"-DWITH_GPU_ARCH=sm_{cuda_arch}")

        return args


    def setup_run_environment(self, env):
        env.prepend_path("PEANO_ROOT", self.spec.prefix)
        env.prepend_path("EXAHYPE_ROOT", self.spec.prefix)
        env.prepend_path("EXAHYPE2_ROOT", self.spec.prefix)
        env.prepend_path("PYTHONPATH", self.spec.prefix + "/python")
