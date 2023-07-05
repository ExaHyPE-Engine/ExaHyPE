import os
import sys
import subprocess

if not os.path.exists("postprocessing"):
    os.makedirs("postprocessing")
if not os.path.exists("postprocessing/solution"):
  os.makedirs("postprocessing/solution")

if os.path.isfile("output.txt"):
  subprocess.call([
     "python3", "../../../python/exahype2/postprocessing/plot-runtime-behaviour.py",
     "output.txt",
     "-v",
     "-t" 'runtime-vs-time-step',
  ])

output_patch_file = "solution/solution-EulerSolver.peano-patch-file"
if os.path.isfile(output_patch_file):
     subprocess.call([
     "python3", "../../../python/peano4/visualisation/render.py",
     str(output_patch_file),
     "--dir=postprocessing"
  ])

  #convert = peano4.visualisation.Convert(output_patch_file, True)
  #convert.set_visualisation_tools_path("../../../src/convert")
  #convert.extract_fine_grid()
  #convert.convert_to_vtk()
