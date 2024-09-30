import numpy as np
from postProcess import parse_patch_file, plot_field_dg
import matplotlib.pyplot as plt

if __name__ == "__main__":
    v_solution = parse_patch_file("DGPoisson.solution-1.peano-patch-file", "DG") 
    v_random   = parse_patch_file("DGPoisson.rand-0.peano-patch-file",     "DG")
    v_solution_ini = parse_patch_file("DGPoisson.solution-0.peano-patch-file", "DG")

    diff_norm = 0.0 # |v_solution - v_random|_{max}
    
    # Iterate over all cell keys in the solution dictionary
    # keys are coordinates of the left-bottom vertex of each cell
    for key in v_solution.keys():
        cell_values_solution = np.asarray(v_solution[key])
        # Access the same key in v_random 
        cell_values_random   = np.asarray(v_random[key])

        # Update global max norm with local cell max norm
        diff_norm = max( diff_norm, np.max(np.abs(cell_values_solution - cell_values_random)) )

    N = 9
    plot_field_dg(v_solution, [1/N,1/N])
    plt.show()

    print(f'|v_solution - v_random|_max = {diff_norm}')
    