"""
Plot residual norm vs number of DG iterations for DG solver
"""

import numpy as np
import re
import matplotlib.pyplot as plt
import mplcursors

res_norm = [] # residual norms after each DG iteration in the formal [|r|_max, |r|_2, |r|_h]

# Open output file
with open('out.txt', 'r') as file:
    lines = file.readlines()

# Work out mesh size
n_mesh = None
for line in lines:
    match = re.search(r'h_min=([\d.]+)', line)
    if match:
        n_mesh = round(1.0/float(match.group(1))) # n_mesh = 3 for 3x3 mesh
        print(f"Mesh size detected: {n_mesh}x{n_mesh}")
        break
if n_mesh is None:
    print("Mesh size not detected")

# Check each line in the output file for "DG smoother step, disable CG solver"
for i in range(len(lines)):
    if "run Solve" in lines[i]:
        # Look at the next line that contains the residual norms
        next_line = lines[i + 1]
        pattern = r"([-+]?\d*\.?\d+(?:e[-+]?\d+)?)"
        full_pattern = rf"\|r\|\_max={pattern},\s*\|r\|\_2={pattern},\s*\|r\|\_h={pattern}"
        norms = re.search(full_pattern, next_line)
        if norms:
            r_max, r_2, r_h = map(float, norms.groups())  # Convert to floats
            res_norm.append([r_max, r_2, r_h])

res_norm_array = np.array(res_norm)

degree = input("Enter polynomial degree: ")

# Plot max-norm vs DG iterations
plt.plot(res_norm_array[:,0], label='r_max', marker='o')
plt.plot(res_norm_array[:,1], label='r_2', marker='o')
plt.yscale('log')
plt.xlabel('DG iterations')
plt.ylabel('|r|_max')

###  Compare to the stable version output ###
res_norm_stable = np.load(f"../../../../../PeanoData/res_norm_dg_mesh_{n_mesh}_p_{degree}_STABLE.npy")
plt.plot(res_norm_stable[:,0], label='r_max stable', marker='x')
plt.plot(res_norm_stable[:,1], label='r_2 stable', marker='x')
#############################################

plt.show()

dir = '../../../../../PeanoData/'
file_name = f'res_norm_dg_mesh_{n_mesh}_p_{degree}.npy'

np.save(f"{dir}{file_name}", res_norm_array)
print(f"Saved residual norms to {dir}{file_name}")