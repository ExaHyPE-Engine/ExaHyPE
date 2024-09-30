"""
Plot residual norm vs number of DG iterations and two-grid cycles
"""

import numpy as np
import re
import matplotlib.pyplot as plt
import mplcursors

res_norm = [] # residual norms after each DG iteration in the formal [|r|_max, |r|_2, |r|_h]

# Open output file
with open('out.txt', 'r') as file:
    lines = file.readlines()

# Check each line in the output file for "DG smoother step, disable CG solver"
for i in range(len(lines)):
    if "DG smoother step, disable CG solver" in lines[i]:
        # Look at the next line that contains the residual norms
        next_line = lines[i + 1]
        pattern = r"([-+]?\d*\.?\d+(?:e[-+]?\d+)?)"
        full_pattern = rf"\|r\|\_max={pattern},\s*\|r\|\_2={pattern},\s*\|r\|\_h={pattern}"
        norms = re.search(full_pattern, next_line)
        if norms:
            r_max, r_2, r_h = map(float, norms.groups())  # Convert to floats
            res_norm.append([r_max, r_2, r_h])

res_norm_array = np.array(res_norm)

# Plot max-norm vs DG iterations
plt.plot(res_norm_array[:,0], label='r_max', marker='o')
plt.yscale('log')
#mplcursors.cursor(hover=True)

# Second axis for cycles
steps = 4  # Number of pre-smoothing steps
max_id = len(res_norm_array[:,0])
cycle_ticks = np.arange(3, max_id, steps)

# Labels for the second axis: 0, 1, 2, ...
cycle_labels = np.arange(len(cycle_ticks))

# Add second axis
cycles = plt.gca().secondary_xaxis('top')
cycles.set_xlabel('Two-grid cycles')

# Set ticks and labels
cycles.set_xticks(cycle_ticks)
cycles.set_xticklabels(cycle_labels)

# Mark V-cycles with vertical lines
for i in range(3, max_id, steps):
    plt.axvline(x=i, color='gray', linestyle='--', linewidth=0.5)

plt.show()

#np.save('../../../../../data***/***.npy', res_norm_array)