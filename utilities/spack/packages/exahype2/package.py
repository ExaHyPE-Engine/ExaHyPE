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
    url = "https://github.com/ExaHyPE-Engine/ExaHyPE"
    git = "https://github.com/ExaHyPE-Engine/ExaHyPE.git"

    version("main", branch="main")

    variant("hdf5", default=True, description="Build with HDF5 support")
    variant("netcdf", default=True, description="Build with NetCDF support")

    depends_on("python")
    depends_on("py-pip")

    depends_on("mpi")

    depends_on(
        "hdf5@1.14.6 -fortran -java +threadsafe +mpi +shared +cxx +hl", when="+hdf5"
    )

    depends_on("netcdf-c@4.9.3 +shared +mpi", when="+netcdf")

    depends_on("cuda@11:", when="+cuda")

    conflicts(
        "cuda_arch=none",
        when="+cuda",
        msg="A value for cuda_arch must be specified. Add cuda_arch=XX",
    )

    variant(
        "gpu_backend",
        default="native",
        description="GPU accelerator backend",
        values=("native", "kokkos", "acpp", "ompt"),
        when="+cuda",
    )

    def cmake_args(self):
        args = [
            self.define_from_variant("PEANO_WITH_HDF5", "hdf5"),
            self.define_from_variant("PEANO_WITH_NETCDF", "netcdf"),
        ]

        if self.spec.satisfies("+cuda"):
            cuda_arch = self.spec.variants["cuda_arch"].value[0]
            gpu_backend = self.spec.variants["gpu_backend"].value
            args.append(f"-DPEANO_WITH_GPU_BACKEND={gpu_backend}")
            args.append(f"-DPEANO_WITH_GPU_ARCH=sm_{cuda_arch}")

        return args

    def install(self, spec, prefix):
        super(Exahype2, self).install(spec, prefix)

        python_exe = Executable(spec["python"].command.path)

        requirements_path = join_path(self.stage.source_path, "requirements.txt")
        if os.path.exists(requirements_path):
            python_exe(
                "-m", "pip", "install", "--prefix=" + prefix, "-r", requirements_path
            )

    def setup_run_environment(self, env):
        env.set("PEANO_SRC_ROOT_DIR", self.spec.prefix)
        env.set("PEANO_BUILD_DIR", self.spec.prefix)
        env.set("PEANO_CMAKE_BUILD_DIR", self.spec.prefix)  # alias for PEANO_BUILD_DIR
        env.prepend_path("PYTHONPATH", self.spec.prefix + "/api")
        env.prepend_path(
            "PYTHONPATH",
            join_path(
                self.spec.prefix,
                "lib",
                "python" + str(self.spec["python"].version.up_to(2)),
                "site-packages",
            ),
        )
        # The JupyterLab server reads its frontend extensions and kernels from the
        # directories on JUPYTER_PATH and JUPYTER_CONFIG_PATH. Both point into the
        # package prefix that install() pip-installs them into.
        env.prepend_path("PATH", join_path(self.spec.prefix, "bin"))
        env.prepend_path(
            "JUPYTER_PATH", join_path(self.spec.prefix, "share", "jupyter")
        )
        env.prepend_path(
            "JUPYTER_CONFIG_PATH", join_path(self.spec.prefix, "etc", "jupyter")
        )
