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
    python3 OfflineDecomposition.py -file Input/example2 -d 3 -plot

    example for artificial generated tree (regular grid):
    python3 OfflineDecomposition.py -d 2 -level 4
    python3 OfflineDecomposition.py -d 3 -level 3 -plot

    example for artificial generated tree with finest level splitting:
    python3 OfflineDecomposition.py -d 3 -level 3 -plot -split 3
"""


parser = argparse.ArgumentParser(description='Grid Patch File Reader')
parser.add_argument("-file", 	"--file-path", 		dest="file_path", 	type=str,  help="file path" )
parser.add_argument("-plot", 	"--plot-tree", 		dest="plot_flag", 	default=False, action="store_true",  help="Plot current Tree structure" )
parser.add_argument("-d",   	"--grid-dimension", dest="dimension",  	type=int, required=True, help="Dimension" )
parser.add_argument("-level",   "--max-level", 		dest="max_level",  	type=int, default=0, help="Dimension" )
parser.add_argument("-split", "--n-splits", 	dest="n_splits", type=int, help="Number of splits to create (integer)")
parser.add_argument("-output", "--output-yaml", dest="output_yaml", type=str, default="output.yaml", help="Output YAML file for split results")

args = parser.parse_args()

time1 = time.time()

if args.file_path != None:
    name = "Files-"+args.file_path
    tree = GridPatchFileReader(name=name, file_path=args.file_path, dimension=args.dimension)
elif args.max_level > 0:
    name = "Regular-Grid-Generator"
    tree = RegularGridGenerator(name=name, max_level=args.max_level, dimension=args.dimension)
else:
    print("No input file specified nor required parameter for Generation, quiting...")
    sys.exit(1)

time2 = time.time()

print("Original tree:")
print(tree)

# Apply finest level splitting if number of splits is provided
if args.n_splits is not None:
    print("\n" + "="*50)
    print("APPLYING FINEST LEVEL SPLITTING")
    print("="*50)
    
    finest_split_start_time = time.time()
    tree = split_strategy.split_finest_level(tree, args.n_splits)
    finest_split_end_time = time.time()

    time3 = finest_split_end_time

    # Then report using the actual measured times:
    print(f"Finest Level Splitting Time Cost: {(finest_split_end_time - finest_split_start_time):.4f} seconds")

    print("\nTree after finest level splitting:")
    print(tree)
    
    # Reset tree to single subtree
    tree = split_strategy.reset_tree_to_single_subtree(tree)

    # Generate YAML output for the split results
    print("\n" + "="*50)
    print("GENERATING TREE YAML (after resetting to single subtree)")
    print("="*50)
    split_strategy.generate_split_yaml(tree, args.output_yaml)
    
else:
    time3 = time2

if args.plot_flag:
    TreeVisualizer(tree, args.dimension)
    print("Figure Saved.")

time4 = time.time()

print('='*40)
print(f"Input Time Cost: {(time2-time1):.4f} seconds")
if args.plot_flag: 
    print(f"Visualization Time Cost: {(time4-time3):.4f} seconds")