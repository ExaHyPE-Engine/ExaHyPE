# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np

class Tree(object):
	"""
	data structure for tree in Peano. The main content of this class 
	is a series of 2D or 3D arrays with one dimension size of 3^{1,2,3,...l},
	l is the maximum depth of the tree level. Each entry of the array would be 
	a tuple of 2 numbers, the first one is the weight of the current node (and 0 if 
	there is no node at this postion), and the second number stands for the subtree
	it belongs to.

	For a 2D example, the following arrays would be created for a uniform, 2 level and 
	4-split tree:

	{(1,0), (1,0), (1,1)
	 (1,1), (1,1), (1,3)
	 (1,2), (1,2), (1,3)}

	{(1,0), (1,0), (1,0), (1,0), (1,0), (1,0), (1,1), (1,1), (1,1),
	 (1,0), (1,0), (1,0), (1,0), (1,0), (1,0), (1,1), (1,1), (1,1),
	 (1,0), (1,0), (1,0), (1,0), (1,0), (1,0), (1,1), (1,1), (1,1),
	 (1,1), (1,1), (1,1), (1,1), (1,1), (1,1), (1,3), (1,3), (1,3),
	 (1,1), (1,1), (1,1), (1,1), (1,1), (1,1), (1,3), (1,3), (1,3),
	 (1,1), (1,1), (1,1), (1,1), (1,1), (1,1), (1,3), (1,3), (1,3),
	 (1,2), (1,2), (1,2), (1,2), (1,2), (1,2), (1,3), (1,3), (1,3),
	 (1,2), (1,2), (1,2), (1,2), (1,2), (1,2), (1,3), (1,3), (1,3),
	 (1,2), (1,2), (1,2), (1,2), (1,2), (1,2), (1,3), (1,3), (1,3)}

	
	Notice the lowest level is 1 with a size of 3x3 or 3x3x3
	"""

	def __init__(self, name, dimension, domain_offset, domain_size):

		self._name			=	name
		self._dimension		=	dimension
		self._domain_offset =	domain_offset
		self._domain_size 	=	domain_size
		
		self._tree_arrays 	= [] # contains the series of arrays
		self._leaf_count 	= 0  # not yet calculated
		self._subtree_count = 0  # not yet calculated
		self._split_pattern	= [] # not yet calculated

		self._max_level		= -1
		self._weighted		= False


	def __str__(self):
		result = (
			"""
Peano Tree Structure From """ + self._name + """
Domain offset:	"""
			+ str(self._domain_offset)
			+ """
Domain size: 	"""
			+ str(self._domain_size)
			+ """  
Dimension:		"""
			+ str(self._dimension)
			+ """
Max Level:		"""
			+ str(self._max_level)
			+ """
Leaf count: 	"""
			+ str(self._leaf_count)
			+ """  
Subtree count:	"""
			+ str(self._subtree_count)
			+ """
Split pattern:	"""
			+ str(self._split_pattern)
			+ """
"""
		)

		return result


	def read_in_tree_from_arrays(self, input_tree_arrays, max_level, weighted=False):

		self._subtree_count = len(input_tree_arrays)
		self._max_level		= max_level

		#initialization
		for level in range(self._max_level):
			size = 3**(level+1)
			if self._dimension==2:
				new_level_array = np.empty((size, size), dtype=object)
				for a in range(size):
					for b in range(size):
						new_level_array[a][b]=(0,0)
			elif self._dimension==3:
				new_level_array = np.empty((size, size, size), dtype=object)
				for a in range(size):
					for b in range(size):
						for c in range(size):
							new_level_array[a][b][c]=(0,0)

			self._tree_arrays.append(new_level_array)

		#actual read in
		for i in range(self._subtree_count):
			for level in range(self._max_level):
				self.read_in_level_array(input_tree_arrays[i][level], i, level)

		self.update_split_pattern()

		if self._weighted:
			self.update_weight()


	def read_in_level_array(self, input_level_arrays, subtree_id, level):
		if self._dimension==2:
			for a in range(3**(level+1)):
				for b in range(3**(level+1)):
					if input_level_arrays[a][b]==1:
						self._tree_arrays[level][a][b] = (input_level_arrays[a][b], subtree_id)
		elif self._dimension==3:
			for a in range(3**(level+1)):
				for b in range(3**(level+1)):
					for c in range(3**(level+1)):
						if input_level_arrays[a][b][c]==1:
							self._tree_arrays[level][a][b][c] = (input_level_arrays[a][b][c], subtree_id)


	def read_in_tree_from_artificial(self, input_artificial_arrays, max_level, weighted=False):
		self._subtree_count = 1
		self._max_level		= max_level

		self._tree_arrays 	= input_artificial_arrays

		self.update_split_pattern()

		if self._weighted:
			self.update_weight()


	def update_split_pattern(self):
		self._leaf_count 	= 0
		self._split_pattern = np.zeros(self._subtree_count)

		for level in range(self._max_level):
			if self._dimension==2:
				for a in range(3**(level+1)):
					for b in range(3**(level+1)):
						if self._tree_arrays[level][a][b][0]==1:
							if level==self._max_level-1: #the finest level
								self._leaf_count += 1
								self._split_pattern[self._tree_arrays[level][a][b][1]] += 1
							elif self._tree_arrays[level+1][3*a][3*b][0]==0: #no children
								self._leaf_count += 1
								self._split_pattern[self._tree_arrays[level][a][b][1]] += 1
			elif self._dimension==3:
				for a in range(3**(level+1)):
					for b in range(3**(level+1)):
						for c in range(3**(level+1)):
							if self._tree_arrays[level][a][b][c][0]==1:
								if level==self._max_level-1: #the finest level
									self._leaf_count += 1
									self._split_pattern[self._tree_arrays[level][a][b][c][1]] += 1
								elif self._tree_arrays[level+1][3*a][3*b][3*c][0]==0: #no children
									self._leaf_count += 1
									self._split_pattern[self._tree_arrays[level][a][b][c][1]] += 1


	def update_weight(self):
		#todo
		return 0

