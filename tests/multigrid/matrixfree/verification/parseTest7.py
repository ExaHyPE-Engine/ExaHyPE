import numpy as np
from postProcess import parse_patch_file, plot_field_dg, plot_field

if __name__ == "__main__":
    v_dg_sol = parse_patch_file("DGPoisson.solution-1.peano-patch-file", "DG") 
    v_dg_ini = parse_patch_file("DGPoisson.solution-0.peano-patch-file", "DG")
    
    rhs_dg_0 = parse_patch_file("DGPoisson.rhs-0.peano-patch-file", "DG")
    rhs_dg_1 = parse_patch_file("DGPoisson.rhs-1.peano-patch-file", "DG")

    rhs_cg_0 = parse_patch_file("CollocatedPoisson.rhs-0.peano-patch-file", "CG")
    rhs_cg_1 = parse_patch_file("CollocatedPoisson.rhs-1.peano-patch-file", "CG")


    # first CG distribution just after the fist restriction 
    v_cg_ini = parse_patch_file("CollocatedPoisson.value-0.peano-patch-file", "CG")
    v_cg_sol = parse_patch_file("CollocatedPoisson.value-1.peano-patch-file", "CG")


    diff_norm = 0.0 # |v_solution - v_random|_{max}
    diff_norm_rhs = 0.0
    
    # Iterate over all cell keys in the solution dictionary
    # keys are coordinates of the left-bottom vertex of each cell
    for key in v_dg_sol.keys():
        cell_values_solution = np.asarray(v_dg_sol[key])
        # Access the same key in v_random 
        cell_values_random   = np.asarray(v_dg_ini[key])

        # print(f'{cell_values_solution = }')
        # print(f'{cell_values_random   = }')
        # print(f'{np.abs(cell_values_solution - cell_values_random) = }')

        # Update global max norm with local cell max norm
        diff_norm = max( diff_norm, np.max(np.abs(cell_values_solution - cell_values_random)) )
        #diff_norm_rhs = max( diff_norm_rhs, np.max(np.abs(np.asarray(rhs_0[key]) - np.asarray(rhs_1[key]))) )
        # print(f'{diff_norm = }')
        # input()

    #print(f'{diff_norm_rhs = }')
    #plot_field_dg(v_random, [1/9,1/9])
    N = 9
    #plot_field_dg(rhs_1, [1/N,1/N])
    #plot_field_dg(v_solution_ini, [1/N,1/N])
    #plot_field_dg(v_random, [1/N,1/N])

    #plot_field(v_cg_ini)
    #plot_field_dg(v_dg_ini, [1/N,1/N])
    #plot_field(v_cg_sol)

    plot_field_dg(rhs_dg_0, [1/N,1/N])
    plot_field(rhs_cg_0)

    print(f'|v_solution - v_random|_max = {diff_norm}')