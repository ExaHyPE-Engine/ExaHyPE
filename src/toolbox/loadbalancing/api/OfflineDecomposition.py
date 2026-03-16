# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, time, os
import argparse

from Tree import Tree
from Input.GridPatchFileReader import *
from Input.RegularGridGenerator import *
from Input.CustomGridGenerator import *
from Visualize.TreeVisualizer import TreeVisualizer
import Strategies.SplitTrees as split_strategy


"""
Offline Tree Decomposition for Load Balancing

This script provides the main command-line interface for the Peano offline load balancing API.
It processes adaptive mesh trees and applies optimisation strategies to achieve better load
distribution across compute resources.

## Command-Line Arguments

### Input Options
-f, --file PATH         Path to input grid patch file directory
                        Example: -f /path/to/checkpoint/grid/files
                        Requires: Grid files with 'grid-' prefix from Peano checkpoints

-d, --dimension DIM     Spatial dimension (REQUIRED)
                        Values: 2 (for 2D grids) or 3 (for 3D grids)
                        
-l, --max-level LEVEL   Maximum refinement level for artificial grid generation
                        Used with: Regular grid generation (when no -f specified)
                        Range: 1-10 (higher values require significant memory)

### Processing Options  
-s, --splits COUNT      Number of additional subtrees to create
                        Example: -s 4 creates 5 total subtrees (original + 4 new)
                        Default: No splitting (analysis only)

-p, --plot              Generate visualisation plots
                        Outputs: PDF and PNG files showing tree structure
                        Default: No visualisation

-o, --output FILE       Output YAML file path
                        Default: Automatic naming based on input type
                        Example: "TreeStructure_l4_s3_split.yaml"

## Usage Examples

### Analyse Existing Simulation Data
```bash
# Process 3D checkpoint files with visualisation
python3 OfflineDecomposition.py -f /path/to/grid/files -d 3 -p

# Process and split into 4 subtrees
python3 OfflineDecomposition.py -f /path/to/grid/files -d 3 -s 4 -p
```

### Generate Test Cases
```bash
# Create 2D regular grid with 4 levels
python3 OfflineDecomposition.py -d 2 -l 4 -p

# Create and split 3D grid for performance testing
python3 OfflineDecomposition.py -d 3 -l 3 -s 8 -p
```

## File Requirements

### Input Files (when using -f)
- Directory containing Peano grid patch files
- Files must have 'grid-' prefix
- Generated with plot_grid_properties=True in Peano solver
- Multiple files indicate existing domain decomposition

### Output Files
- **Original YAML**: Complete tree structure before optimisation
- **Split YAML**: Optimised tree structure (if splitting applied)
- **Visualisation**: PDF/PNG plots (if -p flag used)

## Performance Considerations

- **Memory Usage**: Grows as 3^(dimension × max_level) for regular grids
- **Processing Time**: Splitting algorithms scale with tree size
- **File I/O**: Large checkpoint directories may require significant read time

## Error Handling

The script validates:
- Required dimension parameter
- Valid input source (either -f or -l must be specified)
- File accessibility and format
- Memory constraints for large grids

For integration with Peano workflows and theoretical background, see the load balancing API documentation.
"""

# This function will display split parameters to the console
def display_hardcoded_split_parameters(peano_tree: Tree, title: str) -> None:
    """
    Extract and display hardcoded split parameters from a tree for Peano's Hardcoded load balancer.
    
    Args:
        peano_tree: Tree object with applied splits
        title: Title to display before the parameters
    """
    if peano_tree._subtree_count <= 1:
        print("No splits to extract - tree has only one subtree")
        return
    
    # Get the split parameters
    grid_sweeps, splitting_trees, cell_counts, subtrees = split_strategy.extract_hardcoded_split_parameters(peano_tree)
    
    print("\n" + title)
    print("-" * len(title))
    print(f"Total subtrees: {peano_tree._subtree_count}")
    print(f"Total leaf nodes: {int(peano_tree._leaf_count)}")
    print()
    
    # Display parameters as C++ initializer lists for direct copy-paste into code
    print("Hardcoded Constructor Parameters:")
    print(f"  gridSweeps = {{{', '.join(map(str, grid_sweeps))}}}")
    print(f"  splittingTrees = {{{', '.join(map(str, splitting_trees))}}}")
    print(f"  numberOfCells = {{{', '.join(map(str, cell_counts))}}}")
    print(f"  subTrees = {{{', '.join(map(str, subtrees))}}}")
    print()
    
    # Display as individual tuples for better readability
    print("Individual Split Operations:")
    for i in range(len(grid_sweeps)):
        print(f"  {{{grid_sweeps[i]}; {splitting_trees[i]} → {subtrees[i]}; {cell_counts[i]}}}")
    print()


def main():
    parser = argparse.ArgumentParser(description='Offline Tree Decomposition for Load Balancing')
    parser.add_argument("-f", 	"--file", 		dest="file_path", 	    type=str,                           help="Path to input grid patch file (e.g., Input/example2)" )
    parser.add_argument("-y", 	"--yaml", 		dest="yaml_path", 	    type=str,                           help="Path to input YAML file created by generate_tree_yaml()" )
    parser.add_argument("-p", 	"--plot", 		dest="plot_flag", 	    default=False, action="store_true", help="Generate and save visualization plots of the tree structure" )
    parser.add_argument("-d",   "--dimension",  dest="grid_dimension",  type=int,                           help="Spatial dimension of the grid (2 for 2D, 3 for 3D) - auto-detected from YAML if not specified" )
    parser.add_argument("-l",   "--max-level",  dest="max_level",  	    type=int, default=0,                help="Maximum refinement level for artificial regular grid generation" )
    parser.add_argument("-s",   "--splits", 	dest="num_splits",      type=int,                           help="Number of additional subtrees to create via splitting")
    parser.add_argument("-o",   "--output",     dest="output_yaml",     type=str, default="output.yaml",    help="Output YAML file path for storing split results and tree structure")

    parser.add_argument(
        "--no-topology-update",
        dest="topology_update",
        action="store_false",
        default=True,
        help="Disable topology preservation (skip non-leaf subtree propagation) during split_tree()."
    )

    args = parser.parse_args()

    time1 = time.time()

    if args.yaml_path != None:
        name = "YAML-"+os.path.basename(args.yaml_path)
        filename = args.yaml_path
        tree = CustomGridGenerator(name=name, yaml_file_path=args.yaml_path)
        # Auto-detect dimension from loaded tree if not specified
        if args.grid_dimension is None:
            args.grid_dimension = tree._dimension
    elif args.file_path != None:
        if args.grid_dimension is None:
            print("Error: -d/--dimension is required when using grid patch files")
            sys.exit(1)
        name = "Files-"+args.file_path
        filename = args.file_path
        tree = GridPatchFileReader(name=name, file_path=args.file_path, dimension=args.grid_dimension)
    elif args.max_level > 0:
        if args.grid_dimension is None:
            print("Error: -d/--dimension is required when generating regular grids")
            sys.exit(1)
        name = "Regular-Grid-Generator"
        filename="TreeStructure"
        tree = RegularGridGenerator(name=name, max_level=args.max_level, dimension=args.grid_dimension)
    else:
        print("No input specified. Use one of: -f (grid files), -y (YAML file), or -l (regular grid generation)")
        sys.exit(1)

    time2 = time.time()

    print("Original tree:")
    print(tree)

    # Generate YAML output for the original tree
    print("\n" + "="*50)
    print("GENERATING ORIGINAL TREE YAML")
    print("="*50)

    # Create original tree output filename
    if args.file_path:
        original_yaml_filename = f"Files-{os.path.basename(args.file_path)}_original.yaml"
    else:
        original_yaml_filename = f"TreeStructure_l{args.max_level}_original.yaml"

    # Replace path separators to create a safe filename
    original_yaml_filename = original_yaml_filename.replace('/', '_').replace('\\', '_')

    split_strategy.generate_tree_yaml(tree, original_yaml_filename)

    # Display hardcoded split parameters for original tree
    if tree._subtree_count > 1:  # Only display if there are multiple subtrees
        print("\n" + "="*50)
        print("HARDCODED SPLIT PARAMETERS FOR ORIGINAL TREE")
        print("="*50)
        
        display_hardcoded_split_parameters(tree, "Original Tree Split Parameters")
    else:
        print("\n" + "="*50)
        print("SKIPPING ORIGINAL TREE HARDCODED SPLIT PARAMETERS")
        print("="*50)
        print("Original tree has only one subtree - no split parameters to extract")

    # Save original tree visualization if plotting is enabled
    viz_start_time = time.time()
    if args.plot_flag:
        # Generate filename for original tree
        if args.file_path:
            base_name = f"Files-{os.path.basename(args.file_path)}"
            original_plot_filename = f"{base_name}_original"
        elif args.yaml_path:
            base_name = f"YAML-{os.path.splitext(os.path.basename(args.yaml_path))[0]}"
            original_plot_filename = f"{base_name}_original"
        else:
            base_name = "TreeStructure"
            original_plot_filename = f"{base_name}_l{args.max_level}_original"
            
        # Replace path separators to create a safe filename
        original_plot_filename = original_plot_filename.replace('/', '_').replace('\\', '_')
        
        print("\n" + "="*50)
        print("GENERATING ORIGINAL TREE VISUALIZATION")
        print("="*50)
        
        TreeVisualizer(tree, args.grid_dimension, original_plot_filename)
        print(f"Original tree visualization saved as {original_plot_filename}.pdf and {original_plot_filename}.png")

    viz_end_time = time.time()

    # Initialize split timing variables
    split_start_time = 0.0
    split_end_time = 0.0

    # Apply splitting if number of splits is provided
    if args.num_splits is not None:
        print("\n" + "="*50)
        print("INITIALLY RESETTING TREE TO SINGLE SUBTREE")
        print("="*50)
        
        # Reset tree to single subtree
        tree = split_strategy.reset_tree_to_single_subtree(tree)

        print("\n" + "="*50)
        print("APPLYING FINEST LEVEL SPLITTING")
        print("="*50)
        
        split_start_time = time.time()

        # a runtime switch for whether split_tree() performs its Stage II bottom-up propagation
        if args.topology_update:
            tree = split_strategy.split_tree(tree, args.num_splits)
        else:
            # Temporarily disable SplitTrees._update_non_leaf_assignments without editing SplitTrees.py
            original_update = split_strategy._update_non_leaf_assignments
            try:
                split_strategy._update_non_leaf_assignments = lambda _tree: None
                tree = split_strategy.split_tree(tree, args.num_splits)
            finally:
                split_strategy._update_non_leaf_assignments = original_update

        split_end_time = time.time()

        print(f"Splitting Time Cost: {(split_end_time - split_start_time):.4f} seconds")

        print("\nTree after splitting:")
        print(tree)
        
        # Generate YAML output for the split results
        print("\n" + "="*50)
        print("GENERATING SPLIT RESULTS YAML")
        print("="*50)

        # Create split tree output filename based on input type and number of splits
        if args.file_path:
            split_yaml_filename = f"Files-{os.path.basename(args.file_path)}_s{args.num_splits}_split.yaml"
        elif args.yaml_path:
            base_name = os.path.splitext(os.path.basename(args.yaml_path))[0]
            split_yaml_filename = f"YAML-{base_name}_s{args.num_splits}_split.yaml"
        else:
            split_yaml_filename = f"TreeStructure_l{args.max_level}_s{args.num_splits}_split.yaml"

        # Replace path separators to create a safe filename
        split_yaml_filename = split_yaml_filename.replace('/', '_').replace('\\', '_')

        # Use the generated filename instead of args.output_yaml
        split_strategy.generate_tree_yaml(tree, split_yaml_filename)
        print(f"Split tree YAML saved as {split_yaml_filename}")

        # Display hardcoded split parameters
        print("\n" + "="*50)
        print("HARDCODED SPLIT PARAMETERS FOR SPLIT TREE")
        print("="*50)

        display_hardcoded_split_parameters(tree, "Split Tree Split Parameters")

        # Save split tree visualization if plotting is enabled
        if args.plot_flag:
            if args.file_path:
                base_name = f"Files-{os.path.basename(args.file_path)}"
                split_plot_filename = f"{base_name}_s{args.num_splits}"
            elif args.yaml_path:
                base_name = f"YAML-{os.path.splitext(os.path.basename(args.yaml_path))[0]}"
                split_plot_filename = f"{base_name}_s{args.num_splits}"
            else:
                base_name = "TreeStructure"
                split_plot_filename = f"{base_name}_l{args.max_level}_s{args.num_splits}"
                
            split_plot_filename = split_plot_filename.replace('/', '_').replace('\\', '_')
            
            print("\n" + "="*50)
            print("GENERATING SPLIT TREE VISUALIZATION")
            print("="*50)
            
            TreeVisualizer(tree, args.grid_dimension, split_plot_filename)
            print(f"Split tree visualization saved as {split_plot_filename}.pdf and {split_plot_filename}.png")
        
        time3 = time.time()
    else:
        time3 = viz_end_time

    print('='*40)
    print(f"Input Time Cost: {(time2-time1):.4f} seconds")
    if args.plot_flag:
        print(f"Visualization Time Cost: {(viz_end_time-viz_start_time):.4f} seconds")
    if args.num_splits is not None:
        print(f"Splitting Time Cost: {(split_end_time-split_start_time):.4f} seconds")
    print(f"Total Time Cost: {(time3-time1):.4f} seconds")


if __name__ == "__main__":
    main()
