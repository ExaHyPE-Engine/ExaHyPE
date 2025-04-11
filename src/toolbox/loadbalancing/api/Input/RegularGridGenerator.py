# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from Tree import Tree

"""
	script to create an artificial 2D/3D regular grid given user-specified
	parameters.

"""

def RegularGridGenerator(name, max_level, dimension):
	#calculate the tree matrices
	index=0
	artificial_arrays=[]
	for level in range(max_level):
		size=3**(level+1)
		if dimension==3:
			array = np.empty((size, size, size), dtype=object)
			for a in range(size):
				for b in range(size):
					for c in range(size):
						array[a][b][c]=(1, 0) #all belong to the single tree as default
		if dimension==2:
			array = np.empty((size, size), dtype=object)
			for a in range(size):
				for b in range(size):
					array[a][b]=(1, 0) #all belong to the single tree as default

		artificial_arrays.append(array)

	#create the Tree class and assign properly
	#offset and size is meaningless for artificial tree
	if dimension==3:
		offset=[0,0,0]; size=[1,1,1]
	elif dimension==2:
		offset=[0,0]; size=[1,1]

	peano_tree = Tree(name, dimension, offset, size)
	peano_tree.read_in_tree_from_artificial(input_artificial_arrays=artificial_arrays, max_level=max_level, weighted=False)

	return peano_tree