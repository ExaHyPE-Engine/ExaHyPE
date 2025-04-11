# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, os, math
import re
from collections import defaultdict

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from Tree import Tree

"""
	script to read peano grid patch file and convert it into the 
	peano tree strcutre.

"""

def round_sig(x, sig):
    return float(np.format_float_positional(x, precision=sig, unique=False, trim='k'))

def read_patches(file_path, size_group, dimension):
    with open(file_path, "r") as f:
        content = f.read()
    
    # Regular expression to extract offset and size
    if dimension==3:
	    patch_pattern = re.compile(
	        r'begin patch.*?offset ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+).*?'
	        r'size ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+)', 
	        re.DOTALL
	    )
    elif dimension==2:
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

def GridPatchFileReader(name, file_path, dimension):
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
	    read_patches(folder_path+'/'+file_path, size_group, dimension)
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

	if dimension==3:
		leftmost_offset=[999,999,999]
	elif dimension==2:
		leftmost_offset=[999,999]
	for size_group in size_groups:
	    for offset in size_group[max_size_key]:
	        offset_tuple = tuple(map(float, offset.split()))
	        for m in range(dimension):
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

	        matrix=np.zeros((matrix_size, matrix_size, matrix_size), dtype=int)
	        for offset in offsets:
	            offset_tuple = tuple(map(float, offset.split()))
	            if dimension==3:
	                index=[-1,-1,-1]
	            elif dimension==2:
	                index=[-1,-1]
	            for i in range(len(offset_tuple)):
	                index[i]=round((offset_tuple[i]-domain_offset[i])/size_tuple[i])
	            matrix[tuple(index)]=1

	        #print(len(tree_matrix))
	        tree_matrix[level-1]=matrix
	    tree_matrices.append(tree_matrix)

	#create the Tree class and assign properly
	peano_tree = Tree(name, dimension, domain_offset, domain_size)
	peano_tree.read_in_tree_from_arrays(input_tree_arrays=tree_matrices, max_level=total_level, weighted=False)

	return peano_tree