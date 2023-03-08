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
  file_dir = os.path.dirname(os.path.realpath(__file__))
  # The root dir of the project is: ../../../ -> one more depth because of the out of source build
  project_root_dir = os.path.join(file_dir, '../../../..', )

  # Go through all directories in root dir to find a CMakeCache
  for path in [path.path for path in os.scandir(project_root_dir) if path.is_dir()]:
    files = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    if 'CMakeCache.txt' in files:
      return path
  return ""
