# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import os

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

    version("muc", branch="muc/exahype")

    variant("multithreading", default=True, description="Build with multithreading (OpenMP) support")
    variant("mpi", default=True, description="Build with Message Passing Interface (MPI) support")
    variant("hdf5", default=True, description="Build with HDF5 support")
    variant("netcdf", default=True, description="Build with NetCDF support")

    depends_on("python")
    depends_on("py-pip")

    depends_on('gsl@2.8')
    depends_on('libxsmm@1.17 +generator')
    depends_on('easi@1.6.1 jit=lua')

    depends_on("mpi", when="+mpi")

    depends_on("hdf5@1.14.6 -fortran -java +threadsafe ~mpi +shared +cxx +hl", when="~mpi +hdf5")
    depends_on("hdf5@1.14.6 -fortran -java +threadsafe +mpi +shared +cxx +hl", when="+mpi +hdf5")

    depends_on("netcdf-c@4.9.2 +shared ~mpi", when="~mpi +netcdf")
    depends_on("netcdf-c@4.9.2 +shared +mpi", when="+mpi +netcdf")
    depends_on("netcdf-cxx4@4.3.1 +shared +pic", when="+netcdf")

    depends_on("cuda@11:", when="+cuda")

    conflicts(
        "cuda_arch=none",
        when="+cuda",
        msg="A value for cuda_arch must be specified. Add cuda_arch=XX",
    )

    variant(
        "gpu_backend",
        default="cuda",
        description="GPU accelerator backend",
        values=("sycl", "cuda", "kokkos"),
        when="+cuda",
    )


    def cmake_args(self):
        args = [
            "-DENABLE_EXAHYPE=ON",
            "-DENABLE_TOOLBOX_LOADBALANCING=ON",
            "-DENABLE_TOOLBOX_BLOCKSTRUCTURED=ON",
            "-DENABLE_TOOLBOX_PARTICLES=ON",
            "-DENABLE_TOOLBOX_CURVI=ON",
            "-DWITH_EASI=ON",
            "-DWITH_LIBXSMM=OFF",
            self.define_from_variant("WITH_MULTITHREADING", "multithreading"),
            self.define_from_variant("WITH_MPI", "mpi"),
            self.define_from_variant("WITH_HDF5", "hdf5"),
            self.define_from_variant("WITH_NETCDF", "netcdf"),
        ]

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
        env.set("PEANO_SRC_ROOT_DIR", self.spec.prefix)
        env.set("PEANO_CMAKE_BUILD_DIR", self.spec.prefix)
        env.prepend_path("PYTHONPATH", self.spec.prefix + "/python")
        env.prepend_path("PYTHONPATH", join_path(self.spec.prefix, "lib", "python" + str(self.spec["python"].version.up_to(2)), "site-packages"))
