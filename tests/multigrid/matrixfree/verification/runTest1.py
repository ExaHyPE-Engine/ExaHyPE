import subprocess
import os
import numpy as np
from postProcess import parse_patch_file


"""
Perform git clean -xdf, 
Execute test1.py -m release,
Run ./peano4matrixfree > out.txt
Post-process
"""

# git clean
subprocess.run(["git", "clean", "-xdf"], check=True)

# Run test Python script
poly_degree = 3
subprocess.run(["python3", "test1.py", "-m", "release", "-p", str(poly_degree)], check=True)

# Get path to the generated executable peano4matrixfree
exec_path = os.path.join(os.getcwd(), "peano4matrixfree")

# Run the executable ./peano4matrixfree
output = subprocess.run([exec_path], capture_output=True, text=True, check=True)
output_filename = "out.txt" # executable terminal output will be saved in this file
with open(output_filename, "w") as outfile:
    outfile.write(output.stdout)
    print("Executable output saved to out.txt")

# Post-process
# subprocess.run(["python3", "parseTest1.py"], check=True
v_solution = parse_patch_file("DGPoisson.solution-1.peano-patch-file", "DG") 
v_random   = parse_patch_file("DGPoisson.rand-0.peano-patch-file",     "DG")

diff_norm = 0.0 # |v_solution - v_random|_{max}

# Iterate over all cell keys in the solution dictionary
# keys are coordinates of the left-bottom vertex of each cell
for key in v_solution.keys():
    cell_values_solution = np.asarray(v_solution[key])
    # Access the same key in v_random 
    cell_values_random   = np.asarray(v_random[key])
    # Update global max norm with local cell max norm
    diff_norm = max( diff_norm, np.max(np.abs(cell_values_solution - cell_values_random)) )

# Check the criterion
pass_tolerance = 1e-8
if diff_norm < pass_tolerance:
    print(f'Test 1: \033[{ "32" }m{ "PASSED" }\033[0m')
else:
    print(f'Test 1: \033[{ "31" }m{ "FAILED" }\033[0m')