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
  call_directory = os.path.join(os.getcwd(), "..")
  file_dir = os.path.dirname(os.path.realpath(__file__))
  project_root_dir = os.path.abspath(os.path.join(file_dir, '..', '..', '..'))

  if os.path.commonpath([call_directory, project_root_dir]) != project_root_dir:
    found_cache_file = os.path.exists(os.path.join(project_root_dir, "Config.cmake"))
    if found_cache_file:
      find_CMake_build_dir.saved_CMake_build_dir = project_root_dir
      return find_CMake_build_dir.saved_CMake_build_dir
    else:
      find_CMake_build_dir.saved_CMake_build_dir = ""
      return find_CMake_build_dir.saved_CMake_build_dir

  # We iterate upwards aka implement "cd .." until we reach the Peano root, or until we find CMakeCache.txt.
  current_iter_dir = call_directory

  if 'PEANO_ROOT' in os.environ:
    env_value = os.environ['PEANO_ROOT']
    find_CMake_build_dir.saved_CMake_build_dir = env_value
    return find_CMake_build_dir.saved_CMake_build_dir

  # At project root we try one last time, only for Config.cmake.
  found_config_file = os.path.exists(os.path.join(project_root_dir, "Config.cmake"))
  if found_config_file:
    find_CMake_build_dir.saved_CMake_build_dir = current_iter_dir
    return find_CMake_build_dir.saved_CMake_build_dir

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


def caller_distance_to_build_root():
  # File dir is under src/
  file_dir = os.path.dirname(os.path.realpath(__file__))
  # The root dir of the project is: ../../../ -> one more depth because of the out-of-source build.
  # The caller has to be inside the build directory.
  call_directory = os.getcwd()
  project_root_dir = os.path.abspath(os.path.join(file_dir, '../../..', ))

  # Ensure we call the script under Peano folder.

  # This check means that the python modules (call_directory) are not inside (freshly cloned Peano git repository)
  # If the modules are installed for example to opt the project root directory should be /opt but call directory can
  # be .../Peano/applications/... this case must be handles, if Peano libraries are installed through spack or make install
  # Then the project root should contain a CMakeCache.txt file as part of the instlalation process
  if os.path.commonpath([call_directory, project_root_dir]) != project_root_dir:
    return 0

  current_iter_dir = call_directory
  dist = 0
  while current_iter_dir != project_root_dir:
    found_cache_file = os.path.exists(os.path.join(current_iter_dir, "CMakeCache.txt")) or \
      os.path.exists(os.path.join(current_iter_dir, "Config.cmake"))
    if found_cache_file:
      return dist
    current_iter_dir =os.path.abspath(os.path.join(current_iter_dir, ".."))
    dist += 1

  # At project root, check one final time but only for config.cmake
  if os.path.exists(os.path.join(project_root_dir, "Config.cmake")):
    return 0

  return -1
