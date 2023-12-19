import os
from jinja2 import Environment, FileSystemLoader
from .Helper    import parse_cmake_cache_for_mpi_and_gpu_vendor
from .Helper    import parse_makefile_for_mpi_and_gpu_vendor
from .Helper    import find_CMake_build_dir

class GPUSelect(object):
  def __init__(self):
    template_dir = os.path.dirname(os.path.abspath(__file__))
    env = Environment(loader=FileSystemLoader(template_dir))
    self.template = env.get_template("GPUSelect.template.sh")

    cmake_build_dir = find_CMake_build_dir()
    if cmake_build_dir == "":
      mpi_vendor, gpu_vendor = parse_makefile_for_mpi_and_gpu_vendor()
    else:
      mpi_vendor, gpu_vendor =  parse_cmake_cache_for_mpi_and_gpu_vendor()

    self.context = {
      "mpi_implementation": mpi_vendor,
      "gpu_vendor": gpu_vendor
    }

  def generate(self, directory):
    output_file_path = os.path.join(directory, "GPUSelect.sh")
    with open(output_file_path, "w") as output:
        output.write(self.template.render(self.context))
