import os
import sys
sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

# @todo Dmitry: This file does not belong in here! This is either part of the
#               tutorials or the documentation, but not directly in the api 
#               dir.


# Workspace API modules
from input.GridPatchFileReader import GridPatchFileReader  # [src/toolbox/loadbalancing/api/input/GridPatchFileReader.py](src/toolbox/loadbalancing/api/input/GridPatchFileReader.py)
import strategies.SplitTrees as split_strategy              # [src/toolbox/loadbalancing/api/strategies/SplitTrees.py](src/toolbox/loadbalancing/api/strategies/SplitTrees.py)

def demo(file_path: str, dimensions: int):
    """
    Demo: Read grid patch files, extract hardcoded split parameters, configure Hardcoded LB.
    
    Args:
        file_path: Directory containing Peano grid patch files (prefix 'grid-')
        dimensions: 2 or 3 (must match input files)
    """
    if not os.path.isdir(file_path):
        raise FileNotFoundError(f"Input directory not found: {file_path}")
    print(f"Loading grid patch files from '{file_path}' (dimensions={dimensions})")
    tree = GridPatchFileReader(file_path=file_path, dimensions=dimensions)
    print(tree)

    # Reset to single subtree and apply splitting to create multiple subtrees for the demo
    tree = split_strategy.reset_tree_to_single_subtree(tree)

    destination_ranks = [1, 2]
    hardcoded_config, hardcoded_mode = split_strategy.split_tree(
        tree,
        number_of_subtrees=3,
        enable_propagate_bottom_up=True,
        enable_extract_split_instructions=True,
        destination_ranks=destination_ranks,
        verbose=True,
    )  # creates subtrees {0,1,2}
    print("After split_tree():")
    print(tree)

    print("Hardcoded constructor arguments:")
    print("  destination_ranks =", destination_ranks)
    print("  mode              =", hardcoded_mode)
    print("  config            =", hardcoded_config)

    # Create minimal ExaHyPE2 project
    project = exahype2.Project(
        namespace=["toolbox","loadbalancing","demo"],
        project_name="HardcodedLB",
        directory=".",
        executable="HardcodedLB.demo"
    )

    solver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep(
        "DemoSolver",
        patch_size=9,
        unknowns=1,
        auxiliary_variables=0,
        min_volume_h=0.05,
        max_volume_h=0.1,
        normalised_time_step_size=0.01,
        flux=exahype2.solvers.PDETerms.User_Defined_Implementation,
        eigenvalues=exahype2.solvers.PDETerms.User_Defined_Implementation,
        source_term=exahype2.solvers.PDETerms.None_Implementation
    )
    project.add_solver(solver)

    project.set_global_simulation_parameters(
        dimensions=tree._dimension,
        size=[1.0,1.0,1.0][0:tree._dimension],
        offset=[0.0,0.0,0.0][0:tree._dimension],
        min_end_time=0.0,
        max_end_time=0.0,
        first_plot_time_stamp=0.0,
        time_in_between_plots=0.0,
        periodic_BC=[False,False,False]
    )

    # Set the Hardcoded strategy using the extracted parameters
    project.set_load_balancing(
        "toolbox::loadbalancing::strategies::Hardcoded",
        hardcoded_config
    )

    # Configure build (adjust path/mode to your setup)
    project.set_Peano4_installation("../../../../", peano4.output.CompileMode.Release)

    # Generate only (no compile) for demonstration
    peano4_project = project.generate_Peano4_project(verbose=True)
    peano4_project.generate()
    print("Demo project generated with Hardcoded load balancing.")
    peano4_project.build(make_clean_first=True, number_of_parallel_builds=-1)
    
    # ---- Demo summary output -------------------------------------------------
    print("\n=== Hardcoded Load Balancing Summary ===")
    print(f"Subtrees: {tree._subtree_count}")
    print(f"Leaf count: {int(tree._leaf_count)}")
    print("destination_ranks =", destination_ranks)
    print("Hardcoded mode =", hardcoded_mode)
    print("hardcoded_config =", hardcoded_config)
    print("\nProject object:")
    print(project)

if __name__ == "__main__":
    # Adjust file_path and dimensions as needed
    # Provide a directory containing 'grid-*' files.
    demo(file_path="Input/example", dimensions=3)