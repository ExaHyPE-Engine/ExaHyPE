import numpy as np
from postProcess import parse_patch_file, reorder_array, plot_field

if __name__ == "__main__":
    v_solution = parse_patch_file("CollocatedPoisson.value-1.peano-patch-file", "CG")
    v_random   = parse_patch_file("CollocatedPoisson.rand-1.peano-patch-file",  "CG")
    v_random = reorder_array(v_solution, v_random)
    print(f'{np.max(np.abs(v_solution - v_random)) = }')

    #!!! v_solution and v_random are now Nx3 matrices! for the norm, compare only v[:,2]. Extra check for coordinates consistency can be done in reorder_array()  
    #!!! pay attention to the same in all tests for CG!

    plot_field(v_solution)