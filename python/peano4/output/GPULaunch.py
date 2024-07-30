# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import stat

from jinja2 import Environment, FileSystemLoader

from .Helper import parse_cmake_cache_for_mpi_and_gpu_vendor
from .Helper import parse_makefile_for_mpi_and_gpu_vendor
from .Helper import find_CMake_build_dir


class GPULaunch(object):
    def __init__(self):
        template_dir = os.path.dirname(os.path.abspath(__file__))
        env = Environment(loader=FileSystemLoader(template_dir))
        self._template = env.get_template("GPULaunch.template")
        self._generate = True

        cmake_build_dir = find_CMake_build_dir()
        if cmake_build_dir == "":
            #mpi_vendor, gpu_vendor = parse_makefile_for_mpi_and_gpu_vendor()  # TODO: Fix this by using the path of the caller
            mpi_vendor, gpu_vendor = None, None
        else:
            mpi_vendor, gpu_vendor = parse_cmake_cache_for_mpi_and_gpu_vendor()

        if mpi_vendor is None or gpu_vendor is None:
            self._generate = False
        else:
            self._context = {"GPU_VENDOR": gpu_vendor, "MPI_IMPLEMENTATION": mpi_vendor}


    def generate(self, directory):
        if self._generate:
            output_file_path = os.path.join(directory, "gpu-launch")
            with open(output_file_path, "w") as output:
                output.write(self._template.render(self._context))

            # Define permission bits
            executable_permissions = stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH
            # Change the file permissions to make it executable
            os.chmod(output_file_path, executable_permissions)
