# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np

class Tree(object):
	"""
	Data structure for adaptive mesh trees in Peano load balancing.
	
	This class represents adaptive mesh trees as sequences of grid levels,
	where each level is stored as a 2D or 3D array. The tree structure is
	designed to support offline decomposition and load balancing analysis.
	
	## Tree Structure Overview
	
	The main content of this class is a series of 2D or 3D arrays with dimension sizes
	of 3^(level+1), where level ranges from 1 to the maximum depth of the tree. Each entry
	in the arrays is a tuple of 2 numbers:
	
	- **Weight**: 1 if the cell exists, 0 if it doesn't (enables representation of adaptive meshes)
	- **Subtree ID**: Integer indicating which compute resource/subtree owns the cell
	
	## Data Representation Examples
	
	### 2D Example: Uniform 2-level tree with 4 subtrees
	
	Level 1 (3x3 array):
	```
	{(1,0), (1,0), (1,1)
	 (1,1), (1,1), (1,3)
	 (1,2), (1,2), (1,3)}
	```
	
	Level 2 (9x9 array):
	```
	{(1,0), (1,0), (1,0), (1,0), (1,0), (1,0), (1,1), (1,1), (1,1),
	 (1,0), (1,0), (1,0), (1,0), (1,0), (1,0), (1,1), (1,1), (1,1),
	 (1,0), (1,0), (1,0), (1,0), (1,0), (1,0), (1,1), (1,1), (1,1),
	 (1,1), (1,1), (1,1), (1,1), (1,1), (1,1), (1,3), (1,3), (1,3),
	 (1,1), (1,1), (1,1), (1,1), (1,1), (1,1), (1,3), (1,3), (1,3),
	 (1,1), (1,1), (1,1), (1,1), (1,1), (1,1), (1,3), (1,3), (1,3),
	 (1,2), (1,2), (1,2), (1,2), (1,2), (1,2), (1,3), (1,3), (1,3),
	 (1,2), (1,2), (1,2), (1,2), (1,2), (1,2), (1,3), (1,3), (1,3),
	 (1,2), (1,2), (1,2), (1,2), (1,2), (1,2), (1,3), (1,3), (1,3)}
	```
	
	### Adaptive Mesh Representation
	
	For adaptive meshes where some regions are not refined, entries have weight 0:
	```
	Level 1: {(1,0), (0,0), (1,0)    # Middle cell not refined
			  (1,0), (1,0), (1,0)
			  (1,0), (1,0), (1,0)}
			  
	Level 2: Corresponding 9x9 array with zeros in the middle 3x3 block
	```
	
	## Key Properties
	
	- **Hierarchical Structure**: Each level refines the previous by factor of 3 in each dimension
	- **Adaptive Support**: Zero weights represent non-existent cells in adaptive meshes
	- **Load Balancing Ready**: Subtree IDs enable domain decomposition analysis
	
	## Usage in Load Balancing
	
	This data structure supports various load balancing operations:
	
	1. **Tree Analysis**: Count leaf nodes, analyze refinement patterns
	2. **Splitting Strategies**: Redistribute subtree assignments for load balancing
	3. **Visualization**: Generate spatial representations of tree structure
	4. **Export**: Create YAML outputs for integration with Peano's hardcoded load balancer
	
	## Coordinate System
	
	- **2D**: Arrays use (a,b) indexing where a,b ∈ [0, 3^(level+1)-1]
	- **3D**: Arrays use (a,b,c) indexing where a,b,c ∈ [0, 3^(level+1)-1]
	- **Level Numbering**: Levels start from 0 (coarsest) to max_level-1 (finest)
	- **Array Sizes**: Level k has size 3^(k+1) in each dimension
	
	## Important Notes
	
	- The lowest level is 1 with size 3x3 (2D) or 3x3x3 (3D)
	- All arrays are stored as numpy arrays with dtype=object for tuple storage
	- Tree metadata (leaf count, split patterns) is automatically updated after modifications
	- Domain offset and size define the physical coordinate mapping
	
	## Related Components
	
	This class works with:
	- `GridPatchFileReader`: Reads Peano grid files into Tree format
	- `RegularGridGenerator`: Creates artificial uniform trees for testing
	- `SplitTrees`: Implements splitting algorithms for load balancing
	- `TreeVisualizer`: Generates visual representations of tree structures
	
	## Example Usage
	
	```python
	# Create a tree from artificial regular grid
	tree = RegularGridGenerator("test", max_level=3, dimension=2)
	
	# Apply load balancing splitting
	split_tree = split_tree(tree, number_of_splits=3)
	
	# Generate visualization
	TreeVisualizer(split_tree, dimension=2, filename="balanced_tree")
	
	# Export to YAML
	generate_tree_yaml(split_tree, "output.yaml")
	```
	"""

	def __init__(self, name, dimension, domain_offset, domain_size):
		"""
		Initialize a new Tree object.
		
		Args:
			name (str): Descriptive name for the tree (e.g., "RegularGrid" or filename)
			dimension (int): Spatial dimension (2 for 2D, 3 for 3D)
			domain_offset (tuple): Physical coordinates of domain origin
			domain_size (tuple): Physical size of domain in each dimension
		"""
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

