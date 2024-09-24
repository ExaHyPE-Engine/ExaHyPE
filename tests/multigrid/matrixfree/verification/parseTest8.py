import numpy as np
from postProcess import parse_patch_file, reorder_array, plot_field

if __name__ == "__main__":
    # v_left = A_CG * u_exact
    v_left  = parse_patch_file("CollocatedPoisson.vu-1.peano-patch-file")
    # v_right = M * b_exact
    v_right = parse_patch_file("CollocatedPoisson.vb-1.peano-patch-file")

    v_right = reorder_array(v_left, v_right)
    print(f'{np.max(np.abs(v_left - v_right)) = }')