import os

def find_cmake_build_dir():
  file_dir  = os.path.dirname(os.path.realpath(__file__))
  # The root dir of the project is: ../../..
  project_root_dir = os.path.join(file_dir, '../../..', )

  # Go through all directories in root dir to find a CMakeCache
  for path in [path.path for path in os.scandir(project_root_dir) if path.is_dir()]:
    files = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    if 'CMakeCache.txt' in files:
      return path
  return ""
