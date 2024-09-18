import numpy as np
import re
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time

from verification.postProcess import parse_patch_file, plot_field_dg, construct_dg_function

MaxNorm = []
for i in range(0,200,1):
    file_name = f"DGPoisson.solution-{i}.peano-patch-file"
    dg_solution = parse_patch_file(file_name, "DG")

    # Construct the exact solution for the same mesh
    exact_solution = construct_dg_function(dg_solution, [1/9,1/9])

    # Construct the difference dg_solution - exact_solution
    # Zeros
    diff = {key: [0] * len(value) for key, value in dg_solution.items()}
    max_norm = 0.0
    for key in dg_solution.keys():
        diff[key] = [ (val_sol - val_ex) for val_sol, val_ex in zip(dg_solution[key], exact_solution[key]) ]
        max_norm = max( np.max(np.abs(diff[key])), max_norm )

    MaxNorm.append(max_norm)
    print(max_norm)

    #fig, ax = plot_field_dg(dg_solution, [1/9,1/9])
    # plt.title(f'|dg_sov - exact_sol|_max = {max_norm}')
    #plt.show()

plt.plot(MaxNorm, label='|sol_dg - sol_exact|_max', marker='o')
plt.yscale('log')
plt.xlabel('DG Iterations')
plt.ylabel('|sol_dg - sol_exact|_max')
plt.show()

