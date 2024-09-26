import numpy as np
from postProcess import parse_patch_file, reorder_array, plot_field, plot_field_dg

if __name__ == "__main__":
    # Read v_ACG = A_CG * u_cg_random
    v_ACG = parse_patch_file("Test5CG.acg-1.peano-patch-file", "CG")
    # Read v_restrict_ADG = R * A_DG * P * u_cg_random
    v_restricted_ADG = parse_patch_file("Test5CG.vrestrictADG-1.peano-patch-file", "CG")

    # Read v_MCG = M_CG * u_cg_random
    v_MCG = parse_patch_file("Test5CG.mcg-1.peano-patch-file", "CG")
    # Read v_restricted_MDG = R * M_DG * P * u_cg_random
    v_restricted_MDG = parse_patch_file("Test5CG.vrestrictMDG-1.peano-patch-file", "CG")

    # Align the arrays to a consistent order of vertices
    v_restricted_ADG = reorder_array(v_ACG, v_restricted_ADG)
    v_restricted_MDG = reorder_array(v_MCG, v_restricted_MDG)

    print(f'|R*A_DG*P*u_cg_random - A_CG*u_cg_random|_max = {np.max(np.abs(v_restricted_ADG[:,2] - v_ACG[:,2]))}')
    print(f'|R*M_DG*P*u_cg_random - M_CG*u_cg_random|_max = {np.max(np.abs(v_restricted_MDG[:,2] - v_MCG[:,2]))}')

    #plot_field( parse_patch_file("Test5CG.rand-0.peano-patch-file","CG"))
