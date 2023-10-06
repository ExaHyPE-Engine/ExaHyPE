
import os
import sys
import subprocess

output_patch_file = "solution/solution-EulerSolver.peano-patch-file"
if os.path.isfile(output_patch_file):
    subprocess.call([
        "python3", "../../../python/peano4/visualisation/render.py",
        str(output_patch_file),
    ])
