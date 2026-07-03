# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, os, math
import re
from collections import defaultdict

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from Tree import Tree

"""
Grid Patch File Reader for Peano Load Balancing API

This module provides functionality to read existing Peano grid patch files and convert them 
into the Tree data structure used by the offline load balancing API. It serves as one of the 
primary input generators for the load balancing framework.

## Purpose and Context

The GridPatchFileReader is part of the offline load balancing approach described in the 
load balancing API documentation. Instead of making splitting decisions during mesh 
construction, this module enables:

1. **Post-construction Analysis**: Read checkpointed grid files after complete mesh construction
2. **Tree Structure Recovery**: Convert Peano patch files into analyzable tree representations  
3. **Load Balancing Preparation**: Prepare tree data for splitting strategy application
4. **Offline Optimization**: Enable informed load balancing decisions based on complete mesh knowledge

## Integration with Load Balancing API

This module integrates with other components of the load balancing API:

- **Tree Class**: Converts patch data into Tree objects (see Tree.py)
- **OfflineDecomposition**: Used by the main script for file-based input processing
- **SplitTrees**: Provides tree data for splitting strategy application
- **TreeVisualizer**: Enables visualization of read tree structures

## File Format Support

The reader supports Peano grid patch files with the following characteristics:

- **File Naming**: Files with 'grid-' prefix in the specified directory
- **Patch Format**: Standard Peano patch format with 'begin patch' blocks
- **Spatial Information**: Extracts offset and size data for each patch
- **Multi-tree Support**: Handles multiple subtrees from separate files
- **Dimension Support**: Supports both 2D and 3D grid structures

## Workflow Integration

This module fits into the standard offline decomposition workflow:

1. **Mesh Construction**: Peano constructs complete mesh without load balancing
2. **Checkpoint Creation**: Grid files are written with plot_grid_properties=True
3. **File Reading**: GridPatchFileReader processes the checkpoint files
4. **Tree Analysis**: Resulting Tree objects are analyzed for load distribution
5. **Strategy Application**: Splitting strategies are applied to optimize load balance
6. **Result Export**: Optimized decomposition is exported in YAML format

## Usage Examples

```python
# Read from checkpoint directory
tree = GridPatchFileReader(
	name="ExperimentData", 
	file_path="path/to/grid/files", 
	dimensions=3
)

# Integrate with splitting strategies
from strategies.SplitTrees import split_tree
split_tree(tree, number_of_subtrees=4)

# Generate the optimised tree structure in YAML format
from strategies.SplitTrees import generate_tree_yaml
generate_tree_yaml(tree, "optimized_decomposition.yaml")
```

See the load balancing API documentation for complete workflow details.
"""

def round_sig(x, sig):
	"""
	Round a number to a specified number of significant figures.
	
	Args:
		x (float): Number to round
		sig (int): Number of significant figures
		
	Returns:
		float: Rounded number
	"""
	return float(np.format_float_positional(x, precision=sig, unique=False, trim='k'))

def read_patches(file_path, size_group, dimensions):
	"""
	Parse a single Peano grid patch file and extract spatial information.
	
	This function reads a grid patch file and extracts offset and size information
	for each patch using regular expressions. Patches are grouped by size to 
	determine refinement levels.
	
	Args:
		file_path (str): Path to the grid patch file
		size_group (defaultdict): Dictionary to store {size: [offsets]} mapping
		dimensions (int): Spatial dimensions count (2 or 3)
		
	Returns:
		defaultdict: Updated size_group with patch information
		
	Note:
		The regular expression patterns match the standard Peano patch format:
		- 'begin patch' blocks with offset and size specifications
		- Floating point numbers in scientific notation
		- Space-separated coordinate values
	"""
	with open(file_path, "r") as f:
		content = f.read()
	
	# Regular expression to extract offset and size
	if dimensions==3:
		patch_pattern = re.compile(
			r'begin patch.*?offset ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+).*?'
			r'size ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+)', 
			re.DOTALL
		)
	elif dimensions==2:
		patch_pattern = re.compile(
			r'begin patch.*?offset ([\d.eE+\-]+ [\d.eE+\-]+).*?'
			r'size ([\d.eE+\-]+ [\d.eE+\-]+)', 
			re.DOTALL
		)

	# Find all matches
	for match in patch_pattern.findall(content):
		offset, size = match
		size_group[size].append(offset)  # Group offsets by size

	return size_group

def GridPatchFileReader(name=None, file_path=None, dimensions=None):
	"""
	Main function to read Peano grid patch files and convert to Tree structure.
	
	This function processes a directory of Peano grid patch files and constructs
	a Tree object suitable for load balancing analysis. It handles multiple subtrees,
	determines domain boundaries, and creates the hierarchical array structure.
	
	## Processing Steps
	
	1. **File Discovery**: Scans directory for files with 'grid-' prefix
	2. **Patch Extraction**: Parses each file to extract spatial information  
	3. **Level Determination**: Groups patches by size to identify refinement levels
	4. **Domain Analysis**: Determines overall domain boundaries from largest patches
	5. **Tree Construction**: Builds hierarchical array structure
	6. **Subtree Assignment**: Assigns patches to subtrees based on source files
	
	## Multi-subtree Handling
	
	Each 'grid-' file represents a different subtree in the domain decomposition.
	The function automatically:
	- Processes all subtree files in the directory
	- Assigns appropriate subtree IDs based on file order
	- Merges spatial information into unified tree structure
	- Reports distribution statistics for analysis
	
	Args:
		name (str, optional): Descriptive name for the resulting tree.
			If not provided, a deterministic default name is generated.
		file_path (str): Path to directory containing grid patch files
		dimensions (int): Spatial dimensions count (2 for 2D, 3 for 3D)
		
	Returns:
		Tree: Tree object with complete spatial structure and subtree assignments
		
	Example:
		```python
		# Read 3D grid data from checkpoint
		tree = GridPatchFileReader(
			name="CCZ4_Simulation_T100", 
			file_path="/path/to/checkpoint/grid/files",
			dimensions=3
		)
		
		print(f"Loaded tree with {tree._leaf_count} leaves")
		print(f"Domain: {tree._domain_offset} to {tree._domain_size}")
		print(f"Subtrees: {tree._subtree_count}")
		```
	"""
	if file_path is None:
		raise ValueError("file_path must be provided")

	if dimensions is None:
		raise ValueError("dimensions must be provided")

	if dimensions not in [2, 3]:
		raise ValueError(f"dimensions must be 2 or 3, got {dimensions}")

	if name is None:
		name = f"grid-patch-reader-{dimensions}d"

	folder_path=file_path

	if folder_path[-1] != '/':
		folder_path += '/'

	prefix = 'grid-'
	# List all files in the folder with the specified prefix
	datafiles = [f for f in os.listdir(folder_path) if f.startswith(prefix)]

	#extract grid infomation
	size_groups = []   # Dictionary to store {size: [offsets]}
	for file_path in datafiles: 
		size_group = defaultdict(list)
		read_patches(folder_path+'/'+file_path, size_group, dimensions)
		size_groups.append(size_group)

	total_level=-1
	for size_group in size_groups:
		total_level=max(total_level, len(size_group.keys()))

	patch_number_for_each_level = defaultdict(list)
	subtree_NO=0
	for size_group in size_groups:
		print("Subtree", subtree_NO, "from File")
		for size, offsets in size_group.items():
			print(f"Size: {size} -> Count: {len(offsets)}")
			patch_number_for_each_level[size].append(len(offsets))
		print("-" * 40)
		subtree_NO += 1
	print("Total Cell Count:")
	for size, count in patch_number_for_each_level.items():
		print(f"Size: {size} -> Count: {sum(count)}")
	print("=" * 40)

	#find the domain info directly from grid plot
	max_size=0

	for size_group in size_groups:
		for size in size_group.keys():
			size_tuple = tuple(map(float, size.split()))
			if size_tuple[0]>max_size:
				max_size=size_tuple[0]
				max_size_key=size
	#print(max_size)

	if dimensions==3:
		leftmost_offset=[999,999,999]
	elif dimensions==2:
		leftmost_offset=[999,999]
	for size_group in size_groups:
		for offset in size_group[max_size_key]:
			offset_tuple = tuple(map(float, offset.split()))
			for m in range(dimensions):
				if offset_tuple[m]<leftmost_offset[m]:
					leftmost_offset[m]=offset_tuple[m]
					leftmost_offset_key=offset

	domain_offset = tuple(leftmost_offset)
	domain_size = tuple(map(float, max_size_key.split()))
	domain_size = tuple( round_sig(x * 3,5) for x in domain_size)

	tree_matrices = []

	#calculate the tree matrices
	index=0
	for size_group in size_groups:
		tree_matrix   = [None]*total_level
		for size, offsets in size_group.items():
			size_tuple = tuple(map(float, size.split()))
			level=int( math.log( int(round_sig(domain_size[0]/size_tuple[0],1)), 3) )
			matrix_size=(3**level)
			#print(level, matrix_size)

			if dimensions == 3:
				matrix = np.zeros((matrix_size, matrix_size, matrix_size), dtype=int)
			elif dimensions == 2:
				matrix = np.zeros((matrix_size, matrix_size), dtype=int)
			else:
				raise ValueError(f"Unsupported dimensions: {dimensions}")

			for offset in offsets:
				offset_tuple = tuple(map(float, offset.split()))
				if dimensions==3:
					index=[-1,-1,-1]
				elif dimensions==2:
					index=[-1,-1]
				for i in range(len(offset_tuple)):
					index[i]=round((offset_tuple[i]-domain_offset[i])/size_tuple[i])
				matrix[tuple(index)]=1

			#print(len(tree_matrix))
			tree_matrix[level-1]=matrix
		tree_matrices.append(tree_matrix)

	#create the Tree class and assign properly
	peano_tree = Tree(name, dimensions, domain_offset, domain_size)
	peano_tree.read_in_tree_from_arrays(input_tree_arrays=tree_matrices, max_level=total_level, weighted=False)

	return peano_tree