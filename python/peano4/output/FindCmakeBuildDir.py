import os

def find_cmake_build_dir():
    file_dir  = os.path.dirname(os.path.realpath(__file__))
    # The root dir of the project is: ../../..
    project_root_dir = os.path.join(file_dir, '../../..',)
    
    # Go through all directories in root dir to find a CMakeCache
    for path in [path.path for path in os.scandir(project_root_dir) if path.is_dir()]:
      files = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
      if 'CMakeCache.txt' in files:
        return path

    # Warning message if we haven't found a build directory yet
    print( """
Warning: if you are using the Autoconf build system you can ignore this message, otherwise
if you are going to use the generated project with CMake build system, please
configure the core project before generating any executables. Make sure you create a build
directory and configure the core project before by calling a command like cmake -D... .. or 
ccmake .. etc. If you want to configure the core project after the generation (not advised)
of the executable, you will need to manually add the CMakeLists.txt file of the generated 
executable with an appropriate add_subdirectory(...) call. If you configure the core project first,
it will be done automatically. 
""")
    return ""