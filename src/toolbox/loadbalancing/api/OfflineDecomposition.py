import numpy as np
import sys
import os
import math
import argparse
import matplotlib.cm as cm
import re
from collections import defaultdict

from Tree import Tree
from Input.GridPatchFileReader import *
from Input.RegularGridGenerator import *
from Visualize.TreeVisualizer import TreeVisualizer


"""
	The script for doing the offline decomposition (tree splitting)

	example for file input tree:
	python3 OfflineDecomposition.py -file Input/example2 -d 3 -plot

	example for artificial generated tree (regular grid):
	python3 OfflineDecomposition.py -d 3 -level 4

"""


parser = argparse.ArgumentParser(description='Grid Patch File Reader')
parser.add_argument("-file", 	"--file-path", 		dest="file_path", 	type=str,  help="file path" )
parser.add_argument("-plot", 	"--plot-tree", 		dest="plot_flag", 	default=False, action="store_true",  help="Plot current Tree structure" )
parser.add_argument("-d",   	"--grid-dimension", dest="dimension",  	type=int, required=True, help="Dimension" )
parser.add_argument("-level",   "--max-level", 		dest="max_level",  	type=int, default=0, help="Dimension" )

args = parser.parse_args()

if args.file_path != None:
	name = "Files-"+args.file_path
	tree = GridPatchFileReader(name=name, file_path=args.file_path, dimension=args.dimension)
elif args.max_level > 0:
	name = "Regular-Grid-Generator"
	tree = RegularGridGenerator(name=name, max_level=args.max_level, dimension=args.dimension)
else:
	print("No input file specified nor required parameter for Generation, quiting...")
	sys.exit(1)

print(tree)

if args.plot_flag:
	TreeVisualizer(tree, args.dimension)
	print("Figure Saved.")