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

    variant("mpi", default=True, description="Build with MPI support")
    variant("tracer", default=True, description="Build with particle tracer support")
    variant("hdf5", default=True, description="Build with HDF5 support")
    variant("netcdf", default=True, description="Build with NetCDF support")

    depends_on("cmake@3.31.2")

    depends_on("mpi", when="+mpi")

    depends_on("python@3.13.0")
    depends_on("py-pip@24.3.1")

    depends_on("hdf5@1.14.5 -fortran -java +threadsafe ~mpi +shared +cxx +hl", when="~mpi +hdf5")
    depends_on("hdf5@1.14.5 -fortran -java +threadsafe +mpi +shared +cxx +hl", when="+mpi +hdf5")

    depends_on("netcdf-c@4.9.2 +shared ~mpi", when="~mpi +netcdf")
    depends_on("netcdf-c@4.9.2 +shared +mpi", when="+mpi +netcdf")
    depends_on("netcdf-cxx4@4.3.1 +shared +pic", when="+netcdf")

    depends_on('gsl@2.8')
    depends_on('libxsmm@1.17 +generator')

    variant("omp", default=True, description="Build with OpenMP multithreading support")
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
            "-DWITH_LIBXSMM=OFF",
            self.define_from_variant("WITH_MPI", "mpi"),
            self.define_from_variant("WITH_HDF5", "hdf5"),
            self.define_from_variant("WITH_NETCDF", "netcdf"),
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


    def install(self, spec, prefix):
        super(Exahype2, self).install(spec, prefix)

        python_exe = Executable(spec['python'].command.path)

        requirements_path = join_path(self.stage.source_path, 'requirements.txt')
        if os.path.exists(requirements_path):
            python_exe('-m', 'pip', 'install', '--prefix=' + prefix, '-r', requirements_path)


    def setup_run_environment(self, env):
        env.prepend_path("PEANO_SRC_ROOT_DIR", self.spec.prefix)
        env.prepend_path("PEANO_CMAKE_BUILD_DIR", self.spec.prefix)
        env.prepend_path("PYTHONPATH", self.spec.prefix + "/python")
