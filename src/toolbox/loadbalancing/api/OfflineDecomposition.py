# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, time, os
import argparse

from Tree import Tree
from Input.GridPatchFileReader import *
from Input.RegularGridGenerator import *
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


def main():
    parser = argparse.ArgumentParser(description='Offline Tree Decomposition for Load Balancing')
    parser.add_argument("-f", 	"--file", 		dest="file_path", 	    type=str,                           help="Path to input grid patch file (e.g., Input/example2)" )
    parser.add_argument("-p", 	"--plot", 		dest="plot_flag", 	    default=False, action="store_true", help="Generate and save visualization plots of the tree structure" )
    parser.add_argument("-d",   "--dimension",  dest="grid_dimension",  type=int, required=True,            help="Spatial dimension of the grid (2 for 2D, 3 for 3D)" )
    parser.add_argument("-l",   "--max-level",  dest="max_level",  	    type=int, default=0,                help="Maximum refinement level for artificial regular grid generation" )
    parser.add_argument("-s",   "--splits", 	dest="num_splits",      type=int,                           help="Number of additional subtrees to create via splitting")
    parser.add_argument("-o",   "--output",     dest="output_yaml",     type=str, default="output.yaml",    help="Output YAML file path for storing split results and tree structure")
    args = parser.parse_args()

    time1 = time.time()

    if args.file_path != None:
        name = "Files-"+args.file_path
        filename = args.file_path
        tree = GridPatchFileReader(name=name, file_path=args.file_path, dimension=args.grid_dimension)
    elif args.max_level > 0:
        name = "Regular-Grid-Generator"
        filename="TreeStructure"
        tree = RegularGridGenerator(name=name, max_level=args.max_level, dimension=args.grid_dimension)
    else:
        print("No input file specified nor required parameter for Generation, quiting...")
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

    # Save original tree visualization if plotting is enabled
    viz_start_time = time.time()
    if args.plot_flag:
        # Generate filename for original tree
        if args.file_path:
            base_name = f"Files-{os.path.basename(args.file_path)}"
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
        tree = split_strategy.split_leaves(tree, args.num_splits)
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
        else:
            split_yaml_filename = f"TreeStructure_l{args.max_level}_s{args.num_splits}_split.yaml"

        # Replace path separators to create a safe filename
        split_yaml_filename = split_yaml_filename.replace('/', '_').replace('\\', '_')

        # Use the generated filename instead of args.output_yaml
        split_strategy.generate_tree_yaml(tree, split_yaml_filename)
        print(f"Split tree YAML saved as {split_yaml_filename}")

        # Save split tree visualization if plotting is enabled
        if args.plot_flag:
            if args.file_path:
                base_name = f"Files-{os.path.basename(args.file_path)}"
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
