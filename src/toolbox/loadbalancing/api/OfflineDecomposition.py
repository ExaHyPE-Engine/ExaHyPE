# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, time
import argparse

from Tree import Tree
from Input.GridPatchFileReader import *
from Input.RegularGridGenerator import *
from Visualize.TreeVisualizer import TreeVisualizer
import Strategies.SplitTrees as split_strategy


"""
    The script for doing the offline decomposition (tree splitting)

    example for file input tree:
    python3 OfflineDecomposition.py -f Input/example2 -d 3 -p

    example for artificial generated tree (regular grid):
    python3 OfflineDecomposition.py -d 2 -l 4
    python3 OfflineDecomposition.py -d 3 -l 3 -p

    example for artificial generated tree with finest level splitting:
    python3 OfflineDecomposition.py -d 3 -l 3 -p -s 3
"""


parser = argparse.ArgumentParser(description='Offline Tree Decomposition for Load Balancing')
parser.add_argument("-f", 	"--file", 		dest="file_path", 	    type=str,                           help="Path to input grid patch file (e.g., Input/example2)" )
parser.add_argument("-p", 	"--plot", 		dest="plot_flag", 	    default=False, action="store_true", help="Generate and save visualization plots of the tree structure" )
parser.add_argument("-d",   "--dimension",  dest="grid_dimension",  type=int, required=True,            help="Spatial dimension of the grid (2 for 2D, 3 for 3D)" )
parser.add_argument("-l",   "--max-level",  dest="max_level",  	    type=int, default=0,                help="Maximum refinement level for artificial regular grid generation" )
parser.add_argument("-s",   "--splits", 	dest="num_splits",      type=int,                           help="Number of additional subtrees to create via finest-level splitting")
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

# Apply finest level splitting if number of splits is provided
if args.num_splits is not None:
    print("\n" + "="*50)
    print("INITIALLY RESETTING TREE TO SINGLE SUBTREE")
    print("="*50)
    
    # Reset tree to single subtree
    tree = split_strategy.reset_tree_to_single_subtree(tree)

    print("\n" + "="*50)
    print("APPLYING FINEST LEVEL SPLITTING")
    print("="*50)
    
    finest_split_start_time = time.time()
    tree = split_strategy.split_finest_level(tree, args.num_splits)
    finest_split_end_time = time.time()

    time3 = finest_split_end_time

    # Then report using the actual measured times:
    print(f"Finest Level Splitting Time Cost: {(finest_split_end_time - finest_split_start_time):.4f} seconds")

    print("\nTree after finest level splitting:")
    print(tree)
    
    # Generate YAML output for the split results
    print("\n" + "="*50)
    print("GENERATING SPLIT RESULTS YAML")
    print("="*50)
    split_strategy.generate_split_yaml(tree, args.output_yaml)
    
else:
    time3 = time2

if args.plot_flag:
    # Generate a descriptive filename based on the tree configuration
    if args.num_splits is not None:
        plot_filename = f"{filename}_l{args.max_level}_s{args.num_splits}"
    else:
        plot_filename = f"{filename}_l{args.max_level}"
    
    TreeVisualizer(tree, args.grid_dimension, plot_filename)
    print(f"Figure Saved as {plot_filename}.pdf and {plot_filename}.png")

time4 = time.time()

print('='*40)
print(f"Input Time Cost: {(time2-time1):.4f} seconds")
if args.plot_flag: 
    print(f"Visualization Time Cost: {(time4-time3):.4f} seconds")