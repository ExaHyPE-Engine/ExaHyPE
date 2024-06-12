# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import subprocess

from .Overwrite import Overwrite


def write_file(overwrite,overwrite_is_default,full_qualified_filename):
  """
    overwrite is of type Overwrite and is what the user passes in
    overwrite_is_default is a boolean
  """
  if overwrite==Overwrite.Always:
    return True
  elif overwrite==Overwrite.Never and os.path.exists( full_qualified_filename ):
    return False
  elif overwrite==Overwrite.Never and not os.path.exists( full_qualified_filename ):
    return True
  elif overwrite==Overwrite.Default:
    if not overwrite_is_default and os.path.exists( full_qualified_filename ):
      return False
    else:
      return True
  else:
    print( "Error: undefined overwrite strategy" )
    return True


def find_CMake_build_dir():
  """
  This function searches for the CMake build directory by iteratively checking parent directories.
  Starting from the callers directory (using getcwd), iterating until root directory of Peano and
  this is computed using the location of this file therefore adapt the code if the file is moved
  somewhere else. Using CMake means that it was configured before, and therefore a CMakeCache.txt
  exists.

  :return: The path to the CMake build directory if found, or an empty string otherwise.
  :rtype: str
  """
  if find_CMake_build_dir.CMake_build_dir_check_done:
    return find_CMake_build_dir.saved_CMake_build_dir

  find_CMake_build_dir.CMake_build_dir_check_done = True
  call_directory = os.getcwd()
  file_dir = os.path.dirname(os.path.realpath(__file__))
  project_root_dir = os.path.abspath(os.path.join(file_dir, '..', '..', '..'))

  if os.path.commonpath([call_directory, project_root_dir]) != project_root_dir:
    return ""

  # We iterate upwards aka implement "cd .." until we reach the Peano root, or until we find CMakeCache.txt.
  current_iter_dir = call_directory
  while current_iter_dir != project_root_dir:
    found_cache_file = os.path.exists(os.path.join(current_iter_dir, "CMakeCache.txt"))
    if found_cache_file:
      find_CMake_build_dir.saved_CMake_build_dir = current_iter_dir
      return current_iter_dir
    current_iter_dir = os.path.abspath(os.path.join(current_iter_dir, ".."))

  return find_CMake_build_dir.saved_CMake_build_dir

find_CMake_build_dir.CMake_build_dir_check_done = False
find_CMake_build_dir.saved_CMake_build_dir = ""


def find_configure_log():
  if find_configure_log.find_configure_log_check_done:
    return find_configure_log.configure_log_path

  find_configure_log.find_configure_log_check_done = True
  find_configure_log.configure_log_path = ""

  file_dir = os.path.dirname(os.path.realpath(__file__))
  project_root_dir = os.path.abspath(os.path.join(file_dir, '..', '..', '..'))
  if os.path.exists(os.path.join(project_root_dir, "config.log")):
    find_configure_log.configure_log_path = os.path.join(project_root_dir, "config.log")

  return find_configure_log.configure_log_path

find_configure_log.find_configure_log_check_done = False
find_configure_log.configure_log_path = ""


def using_cuda_backend():
  """
  This function checks if Peano is built with CUDA. Caches the result, after the first call calling this
  function will have no cost as it returns a cached value. It checks the value for the GPU backend
  saved in the CMakeCache.txt whether it is set to CUDA.

  :return: True if the CUDA backend is being used, False otherwise.
  :rtype: bool
  """
  if using_cuda_backend.using_cuda_backend_check_done:
    return using_cuda_backend.using_cuda_backend_cached_value

  using_cuda_backend.using_cuda_backend_check_done = True
  cmake_build_dir_path = find_CMake_build_dir()
  if cmake_build_dir_path != "":  # Check for CUDA while we are using CMake
    try:
      with open(os.path.join(cmake_build_dir_path,"CMakeCache.txt")) as cache:
        for line in cache:
          if "WITH_GPU:STRING=cuda" in line:
            using_cuda_backend.using_cuda_backend_cached_value = True
            return using_cuda_backend.using_cuda_backend_cached_value
    except Exception:
      using_cuda_backend.using_cuda_backend_cached_value = False
      return using_cuda_backend.using_cuda_backend_cached_value
  else:  # Check for CUDA while we are using Automake
    config_log_path = find_configure_log()

    if config_log_path == "":
      using_cuda_backend.using_cuda_backend_cached_value = False
      return using_cuda_backend.using_cuda_backend_cached_value

    for line in open(config_log_path):
      if line.startswith("GPUOffloadingCUDA_TRUE=''"):
        using_cuda_backend.using_cuda_backend_cached_value = True
        return using_cuda_backend.using_cuda_backend_cached_value

    using_cuda_backend.using_cuda_backend_cached_value = False
    return using_cuda_backend.using_cuda_backend_cached_value

  return using_cuda_backend.using_cuda_backend_cached_value

using_cuda_backend.using_cuda_backend_check_done = False
using_cuda_backend.using_cuda_backend_cached_value = False


def caller_distance_to_build_root():
  # File dir is under src/
  file_dir = os.path.dirname(os.path.realpath(__file__))
  # The root dir of the project is: ../../../ -> one more depth because of the out-of-source build.
  # The caller has to be inside the build directory.
  call_directory = os.getcwd()
  project_root_dir = os.path.abspath(os.path.join(file_dir, '../../..', ))

  if os.path.commonpath([call_directory, project_root_dir]) != project_root_dir:
    return -1

  current_iter_dir = call_directory
  dist = 0
  while current_iter_dir != project_root_dir:
    found_cache_file = os.path.exists(os.path.join(current_iter_dir, "CMakeCache.txt"))
    if found_cache_file:
      return dist
    current_iter_dir =os.path.abspath(os.path.join(current_iter_dir, ".."))
    dist += 1

  return -1


def get_gpu_vendor(architecture_string):
  if architecture_string == None:
    return None
  elif architecture_string.lower().startswith("gfx"):
    return "AMD"
  elif architecture_string.lower().startswith("spir"):
    return "INTEL"
  elif architecture_string.lower().startswith("sm") or \
    architecture_string.lower().startswith("cc") or \
    architecture_string.isnumeric():
    return "NVIDIA"
  return None


def get_mpi_implementation(mpicxx):
    # Run mpicxx --version and infer which MPI implementation is being used.
    # MPICH and Intel oneAPI print information including MPICH and Intel(R) MPI Library.
    # OpenMPI doesn't print any information that enables identification.
    # A workaround can be running mpicxx --showme.
    mpicxx = mpicxx.strip()  # Remove leading and trailing whitespace, including newline characters.
    bash_command = f"{mpicxx} -v"
    result = subprocess.run(bash_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    result_str = ""
    if result.returncode == 0:
        result_str = str(result.stdout) + " " + str(result.stderr)
    if "MPICH" in result_str:
        return "MPICH"
    elif "Intel(R) MPI Library" in result_str:
        return "INTEL"
    else:
        showme_bash_command = f"{mpicxx} --showme"
        showme_result = subprocess.run(showme_bash_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if showme_result.returncode == 0:
            if "openmpi" in str(showme_result.stdout):
                return "OPENMPI"
            elif "openmpi" in str(showme_result.stderr):
                return "OPENMPI"
    return None


def parse_cmake_cache_for_mpi_and_gpu_vendor():
  cmake_build_dir = find_CMake_build_dir()

  mpi_vendor = None
  gpu_vendor = None
  with open(os.path.join(cmake_build_dir, "CMakeCache.txt"), "r") as f:
    for line in f:
      if "MPI_CXX_COMPILER:FILEPATH" in line:
        mpicxx = line.split("=")[1]  # Get path to mpicxx. E.g., /usr/bin/mpicxx
        mpi_vendor = get_mpi_implementation(mpicxx)
      elif "SLURM_SRUN_COMMAND" in line:
        mpi_vendor = "SLURM"
      if "WITH_GPU_ARCH:STRING" in line:
        gpu_arch = line.split("=")[1]  # Get GPU arch
        gpu_vendor = get_gpu_vendor(gpu_arch)
      if mpi_vendor and gpu_vendor:
        break
  return (mpi_vendor, gpu_vendor)


def parse_makefile_for_mpi_and_gpu_vendor():
  # Automake does not differentiate between mpicxx and cxx variables
  cxx = None
  cxxflags = None
  nvcc = None
  nvccflags = None
  gpu_vendor = None
  mpi_vendor = None

  peano_root_path = os.path.join(os.path.dirname(__file__), "..", "..", "..")
  makefile_path = os.path.join(peano_root_path, "src", "Makefile")

  with open(makefile_path, "r") as makefile:
    for line in makefile:
      if line.startswith("CXX ="):
        cxx = line[len("CXX ="):]
      elif line.startswith("CXXFLAGS ="):
        cxxflags = line[len("CXXFLAGS ="):]
      elif line.startswith("NVCC ="):
        nvcc = line[len("NVCC ="):]
      if line.startswith("NVCCFLAGS ="):
        nvccflags = line[len("NVCCFLAGS ="):]
      if cxx and cxxflags and nvcc and nvccflags:
        break
  if nvcc == None:
    nvcc = ""
  if nvccflags == None:
    nvccflags = ""

  #print(cxx, cxxflags, nvcc, nvccflags)

  cxxflags = cxx + " " +  cxxflags
  nvccflags = nvcc + " " + nvccflags

  # There are various ways to pass the GPU architecture as a part of CXXFLAGS.
  # It can be: --offload-arch=<...>, --cuda-gpu-arch=<...>, --arch=<...>, -gpu=<...>
  cxxflags_tokenized = cxxflags.split()
  nvccflags_tokenized = nvccflags.split()

  offload_flags = ["--offload-arch", "--cuda-gpu-arch", "--arch", "-gpu", "-arch"]
  mpi_vendor = get_mpi_implementation(cxx.split()[0])

  gpu_arch = None
  for flags_tokenized in [cxxflags_tokenized, nvccflags_tokenized]:
    for flag_token in flags_tokenized:
      for offload_flag in offload_flags:
        if flag_token.startswith(offload_flag):
            if "=" in flag_token:
              gpu_arch = flag_token.split("=")[1]
            else:
              gpu_arch = flag_token.split()[1]
            break
      if gpu_arch:
        break
    if gpu_arch:
      break

  if gpu_arch:
    gpu_vendor = get_gpu_vendor(architecture_string=gpu_arch)

  return (mpi_vendor, gpu_vendor)
