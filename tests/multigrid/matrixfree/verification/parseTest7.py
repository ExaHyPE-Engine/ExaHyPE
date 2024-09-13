import numpy as np
from postProcess import parse_patch_file, plot_field_dg, plot_field

if __name__ == "__main__":
    v_dg_sol = parse_patch_file("DGPoisson.solution-1.peano-patch-file", "DG") 
    v_dg_ini = parse_patch_file("DGPoisson.solution-0.peano-patch-file", "DG")

    # first CG distribution just after the fist restriction 
    v_cg_rnd = parse_patch_file("CollocatedPoisson.rand-0.peano-patch-file",  "CG")
    v_cg_sol = parse_patch_file("CollocatedPoisson.value-1.peano-patch-file", "CG")


    diff_norm = 0.0 # |v_solution - v_random|_{max}
    
    # Iterate over all cell keys in the solution dictionary
    # keys are coordinates of the left-bottom vertex of each cell
    for key in v_dg_sol.keys():
        cell_values_solution = np.asarray(v_dg_sol[key])
        # Access the same key in v_random 
        cell_values_random   = np.asarray(v_dg_ini[key])

    # N = 9
    # plot_field_dg( parse_patch_file("DGPoisson.solution-1.peano-patch-file", "DG"), [1/N,1/N] )
    # plot_field( parse_patch_file("CollocatedPoisson.value-0.peano-patch-file", "CG") )

    diff_norm = np.max(np.abs(v_cg_rnd[:,2] - v_cg_sol[:,2]))
    print(f'|v_solution - v_random|_max = {diff_norm}')