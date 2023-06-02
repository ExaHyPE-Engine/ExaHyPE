# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os.path

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

def find_cmake_build_dir():
  # File dir is under src/
  file_dir = os.path.dirname(os.path.realpath(__file__))
  # The root dir of the project is: ../../../ -> one more depth because of the out of source build
  # The caller has to be inside the build directory
  call_directory = os.getcwd()
  project_root_dir = os.path.abspath(os.path.join(file_dir, '../../..', ))

  # Ensure we call the script under Peano4 folder
  if os.path.commonpath([call_directory, project_root_dir]) != project_root_dir:
    raise Exception("The Peano4 code generator has to be called in a folder under Peano4 project's root directory. E.g. cd build/... & python ...")

  # We will iterate upwards aka implement "cd .." until we reach the peano4 root, or until we find CMakeCache.txt 
  current_iter_dir = call_directory
  while current_iter_dir != project_root_dir:
    found_cache_file = os.path.exists(os.path.join(current_iter_dir, "CMakeCache.txt"))
    if found_cache_file:
      return current_iter_dir
    current_iter_dir =os.path.abspath(os.path.join(current_iter_dir, ".."))
  raise ""

def using_cuda_backend():
  build_dir_path = find_cmake_build_dir()
  if build_dir_path != "":
    try:
      with open(os.path.join(build_dir_path,"CMakeCache.txt")) as cache:
        for line in cache:
          if "WITH_GPU:STRING=cuda" in line:
            return True
    except Exception:
      return False
  return False