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
    variant("hdf5", default=True, description="Build with HDF5 support")
    variant("netcdf", default=True, description="Build with NetCDF support")
    variant("curvi", default=True, description="Build with Curvi toolbox support")

    depends_on("python")
    depends_on("py-pip")

    depends_on('gsl@2.8')
    depends_on('libxsmm@1.17 +generator')
    depends_on('easi@1.6.1 jit=lua')

    depends_on("mpi")

    depends_on("hdf5@1.14.6 -fortran -java +threadsafe +mpi +shared +cxx +hl", when="+hdf5")

    depends_on("netcdf-c@4.9.2 +shared +mpi", when="+netcdf")
    depends_on("netcdf-cxx4@4.3.1 +shared +pic", when="+netcdf")

    depends_on("cuda@11:", when="+cuda")

    conflicts(
        "cuda_arch=none",
        when="+cuda",
        msg="A value for cuda_arch must be specified. Add cuda_arch=XX",
    )
    conflicts("+curvi~netcdf", msg="Curvi requires NetCDF support")

    variant(
        "gpu_backend",
        default="native",
        description="GPU accelerator backend",
        values=("native", "kokkos", "acpp", "ompt"),
        when="+cuda",
    )


    def cmake_args(self):
        args = [
            "-DPEANO_WITH_EASI=ON",
            "-DPEANO_WITH_LIBXSMM=OFF",
            self.define_from_variant("PEANO_WITH_MULTITHREADING", "multithreading"),
            self.define_from_variant("PEANO_WITH_HDF5", "hdf5"),
            self.define_from_variant("PEANO_WITH_NETCDF", "netcdf"),
            self.define_from_variant("PEANO_WITH_CURVI", "curvi"),
        ]

        if self.spec.satisfies("+cuda"):
            cuda_arch = self.spec.variants["cuda_arch"].value[0]
            gpu_backend = self.spec.variants["gpu_backend"].value
            args.append(f"-DPEANO_WITH_GPU_BACKEND={gpu_backend}")
            args.append(f"-DPEANO_WITH_GPU_ARCH=sm_{cuda_arch}")

        return args


    def install(self, spec, prefix):
        super(Exahype2, self).install(spec, prefix)

        python_exe = Executable(spec['python'].command.path)

        requirements_path = join_path(self.stage.source_path, 'requirements.txt')
        if os.path.exists(requirements_path):
            python_exe('-m', 'pip', 'install', '--prefix=' + prefix, '-r', requirements_path)


    def setup_run_environment(self, env):
        env.set("PEANO_SRC_ROOT_DIR", self.spec.prefix)
        env.set("PEANO_BUILD_DIR", self.spec.prefix)
        env.set("PEANO_CMAKE_BUILD_DIR", self.spec.prefix)  # alias for PEANO_BUILD_DIR
        env.prepend_path("PYTHONPATH", self.spec.prefix + "/api")
        env.prepend_path("PYTHONPATH", join_path(self.spec.prefix, "lib", "python" + str(self.spec["python"].version.up_to(2)), "site-packages"))
        # The notebook dependencies (jupyterlab, ipywidgets, matplotlib, ...)
        # are pip-installed into the package prefix by install(). Setting only
        # PYTHONPATH lets `import ipywidgets` succeed, but the JupyterLab server
        # also needs to discover the pip-installed frontend extensions and kernels
        # under <prefix>/share/jupyter - otherwise the interactive renderers import
        # without error yet never draw anything in the browser. Put the prefix's
        # bin on PATH (so the matching jupyter is used) and its jupyter data/config
        # dirs on the jupyter search paths.
        env.prepend_path("PATH", join_path(self.spec.prefix, "bin"))
        env.prepend_path("JUPYTER_PATH", join_path(self.spec.prefix, "share", "jupyter"))
        env.prepend_path("JUPYTER_CONFIG_PATH", join_path(self.spec.prefix, "etc", "jupyter"))
