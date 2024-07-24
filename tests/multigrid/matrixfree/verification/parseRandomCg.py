import numpy as np
from postProcess import parse_patch_file, reorder_array, plot_field

if __name__ == "__main__":
    v_solution = parse_patch_file("CollocatedPoisson.value-1.peano-patch-file") 
    v_random   = parse_patch_file("CollocatedPoisson.rand-1.peano-patch-file")
    v_random = reorder_array(v_solution, v_random)
    print(f'{np.max(np.abs(v_solution - v_random)) = }')

    plot_field(v_solution)