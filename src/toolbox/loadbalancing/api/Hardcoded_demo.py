import os
import sys
sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

# Workspace API modules
from input.GridPatchFileReader import GridPatchFileReader  # [src/toolbox/loadbalancing/api/input/GridPatchFileReader.py](src/toolbox/loadbalancing/api/input/GridPatchFileReader.py)
import Strategies.SplitTrees as split_strategy              # [src/toolbox/loadbalancing/api/Strategies/SplitTrees.py](src/toolbox/loadbalancing/api/Strategies/SplitTrees.py)

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
# @todo Dmitry I'm happy with a name, but it really should be optional, i.e. I don't want to have to specify it here
    tree = GridPatchFileReader(name=f"Files-{file_path}", file_path=file_path, dimension=dimensions)
    print(tree)

    # Reset to single subtree and apply splitting to create multiple subtrees for the demo
    tree = split_strategy.reset_tree_to_single_subtree(tree)
# @todo Dmitry It should number of subtree rather than number of splits.
    gridSweeps, splittingTrees, numberOfCells, subTrees = split_strategy.split_tree(
        tree,
        number_of_splits=2,
        enable_propagate_bottom_up=True,
        enable_extract_split_instructions=True,
    )  # creates subtrees {0,1,2}
    print("After split_tree():")
    print(tree)

    print("Extracted hardcoded parameters:")
    print("  gridSweeps     =", gridSweeps)
    print("  splittingTrees =", splittingTrees)
    print("  numberOfCells  =", numberOfCells)
    print("  subTrees       =", subTrees, "(subtree ids, not ranks)")

    # Map subtree ids to destination ranks (simple mapping; adjust for real MPI runs)
    destinationRanks = [max(1, sid) for sid in subTrees]

    # Build Hardcoded constructor argument string (single mode variant)
    hardcoded_mode = "peano4::SplitInstruction::Mode::AggressiveTopDown"
    hardcoded_config = (
        "{"
        f"{{{', '.join(map(str, gridSweeps))}}}, "
        f"{{{', '.join(map(str, splittingTrees))}}}, "
        f"{{{', '.join(map(str, numberOfCells))}}}, "
        f"{{{', '.join(map(str, destinationRanks))}}}, "
        f"{hardcoded_mode}"
        "}"
    )

    print("Hardcoded constructor arguments:")
    print("  gridSweeps       =", gridSweeps)
    print("  splittingTrees   =", splittingTrees)
    print("  numberOfCells    =", numberOfCells)
    print("  destinationRanks =", destinationRanks)
    print("  mode             =", hardcoded_mode)

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
    print("gridSweeps     =", gridSweeps)
    print("splittingTrees =", splittingTrees)
    print("numberOfCells  =", numberOfCells)
    print("destinationRanks =", destinationRanks)
    print("Hardcoded mode =", hardcoded_mode)
    print("\nC++ constructor example:")
    print("toolbox::loadbalancing::strategies::Hardcoded(")
    print(f"  {{{', '.join(map(str, gridSweeps))}}},")
    print(f"  {{{', '.join(map(str, splittingTrees))}}},")
    print(f"  {{{', '.join(map(str, numberOfCells))}}},")
    print(f"  {{{', '.join(map(str, destinationRanks))}}},")
    print(f"  {hardcoded_mode}")
    print(");")
    print("\nProject object:")
    print(project)

if __name__ == "__main__":
    # Adjust file_path and dimensions as needed
    # Provide a directory containing 'grid-*' files.
    demo(file_path="Input/example", dimensions=3)