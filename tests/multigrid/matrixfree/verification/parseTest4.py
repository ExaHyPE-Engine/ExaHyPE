import numpy as np
from postProcess import parse_patch_file, reorder_array

if __name__ == "__main__":
    v_cg_random = parse_patch_file("Test4CG.value-0.peano-patch-file",   "CG") # CG random distribution
    vector2     = parse_patch_file("Test4CG.vector2-1.peano-patch-file", "CG") # vector2 = R * v_dg_random

    v_dg_random = parse_patch_file("Test4DG.solution-0.peano-patch-file", "DG") # v_dg_random
    vector1     = parse_patch_file("Test4DG.vector1-1.peano-patch-file",  "DG") # vector1 = P * v_cg_random

    # Reorder vector2 in accordance with v_cg_random
    vector2 = reorder_array(v_cg_random, vector2)

    # Compute v_cg_random * R * v_dg_random = v_cg_random * vector2
    dot_prod_cg = np.dot(v_cg_random[:,2], vector2[:,2])
    
    # Compute dot product v_dg_random * P * v_cg_random = v_dg_random * vector1
    # Iterate over all cell keys in the dictionary
    # keys are coordinates of the left-bottom vertex of each cell
    dot_prod_dg = 0.0
    for key in v_dg_random.keys():
        cell_values_random  = np.asarray(v_dg_random[key])
        # Access the same key in vector1 
        cell_values_vector1 = np.asarray(vector1[key])

        dot_prod_dg += np.dot(cell_values_random, cell_values_vector1)

    # Check | v_cg_random * R * v_dg_random - v_dg_random * P * v_cg_random | < eps
    diff = abs(dot_prod_cg - dot_prod_dg)
    print(f'{diff}')