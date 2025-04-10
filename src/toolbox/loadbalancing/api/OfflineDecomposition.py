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
from Visualize.TreeVisualizer import TreeVisualizer


#main start here
#input parameters
parser = argparse.ArgumentParser(description='Grid Patch File Reader')
parser.add_argument("-file", "--file-path", dest="file", type=str, required="True",  help="file path" )
parser.add_argument("-plot", "--plot-tree", dest="plot_flag", default=False, action="store_true",  help="Plot current Tree structure" )

args = parser.parse_args()

tree = GridPatchFileReader(args.file)

print(tree)

TreeVisualizer(tree)