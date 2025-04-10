import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import math

import re
from collections import defaultdict

def round_sig(x, sig):
    return float(np.format_float_positional(x, precision=sig, unique=False, trim='k'))

def read_patches(file_path, size_groups):
    with open(file_path, "r") as f:
        content = f.read()
    
    # Regular expression to extract offset and size
    patch_pattern = re.compile(
        r'begin patch.*?offset ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+).*?'
        r'size ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+)', 
        re.DOTALL
    )

    # Find all matches
    for match in patch_pattern.findall(content):
        offset, size = match
        size_groups[size].append(offset)  # Group offsets by size

    return size_groups

def read_SFC(file_path, domain_offset, domain_size):
    with open(file_path, "r") as f:
        content = f.read()
    
    # Regular expression to extract offset and size
    patch_pattern = re.compile(
        r'begin patch.*?offset ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+).*?'
        r'size ([\d.eE+\-]+ [\d.eE+\-]+ [\d.eE+\-]+)', 
        re.DOTALL
    )

    index_map=[]
    # Find all matches
    for match in patch_pattern.findall(content):
        index_entry=[0,0,0,0] #level and three dimensions
        offset, size = match
        size_tuple = tuple(map(float, size.split()))
        offset_tuple = tuple(map(float, offset.split()))

        index_entry[0]=int( math.log( int(round_sig(domain_size[0]/size_tuple[0],1)), 3) )
        for i in range(len(offset_tuple)):
            index_entry[i+1]=round((offset_tuple[i]-domain_offset[i])/size_tuple[i])

        index_map.append(index_entry)

    return index_map


#main start here

#python3  ccz4.py -impl fd4-rk1-adaptive -s gauge -maxh 0.05 -minh 0.02 -ps 3 -plt 0.1 -et 0.05 -exn test --domain_r 0.5 --ReSwi 1 -cfl 0.1 --KOSigma 8.0
#=========================================================
#input parameters
folder_path = 'case2'
prefix = 'grid-'
dimension=3
subtree_number=8
#=========================================================


# List all files in the folder with the specified prefix
datafiles = [f for f in os.listdir(folder_path) if f.startswith(prefix)]

#extract grid infomation
size_groups = defaultdict(list)   # Dictionary to store {size: [offsets]}
for file_path in datafiles: 
	read_patches(folder_path+'/'+file_path, size_groups)

for size, offsets in size_groups.items():
    print(f"Size: {size} -> Count: {len(offsets)}")
    #print("Offsets:")
    #for offset in offsets:
    #    print(f"  {offset}")
    #print("-" * 40)

#find the domain info directly from grid plot
max_size=0
for size in size_groups.keys():
	size_tuple = tuple(map(float, size.split()))
	if size_tuple[0]>max_size:
		max_size=size_tuple[0]
		max_size_key=size
#print(max_size)

leftmost_offset=[999,999,999]
for offset in size_groups[max_size_key]:
    offset_tuple = tuple(map(float, offset.split()))
    #print(offset_tuple)
    for m in range(dimension):
        if offset_tuple[m]<leftmost_offset[m]:
            leftmost_offset[m]=offset_tuple[m]
            leftmost_offset_key=offset
#print(leftmost_offset, leftmost_offset_key) 

domain_offset = tuple(leftmost_offset)#tuple(map(float, leftmost_offset_key.split()))
domain_size = tuple(map(float, max_size_key.split()))
domain_size = tuple( round_sig(x * 3,5) for x in domain_size)

print(domain_offset, domain_size)

#preparing tree matrices
total_level=len(size_groups.keys())
#for i in range(total_level):
#	print("level "+str(i)+" Matrix total entries:", (3*(3**i))**dimension,"size: "+str(domain_size[0]/3/3**i))
tree_matrix=[None]*total_level
tree_patches=[None]*total_level
#print(tree_matrix)

#calculate the tree matrices
for size, offsets in size_groups.items():
    size_tuple = tuple(map(float, size.split()))
    level=int( math.log( int(round_sig(domain_size[0]/size_tuple[0],1)), 3) )
    matrix_size=(3**level)
    #print(level, matrix_size)

    matrix=np.zeros((matrix_size, matrix_size, matrix_size), dtype=np.ubyte)
    for offset in offsets:
        offset_tuple = tuple(map(float, offset.split()))
        index=[-1,-1,-1]
        for i in range(len(offset_tuple)):
            index[i]=round((offset_tuple[i]-domain_offset[i])/size_tuple[i])
        matrix[tuple(index)]=1
    tree_matrix[level-1]=matrix
    tree_patches[level-1]=len(offsets)

for i in range(total_level):
    print("level "+str(i)+":",tree_matrix[i].shape)

#We calculate the total amount of leaves, we set tree entry with max (255) to show it is a leaf
print("="*40)
leaves_count=0
for level in range(total_level):
    for i in range(3**(level+1)):
        for j in range(3**(level+1)):
            for k in range(3**(level+1)):
                if tree_matrix[level][i][j][k]==1:
                    if level == total_level-1:
                        leaves_count=leaves_count+1
                        tree_matrix[level][i][j][k]=255
                    elif tree_matrix[level+1][3*i][3*j][3*k]==0:
                        leaves_count=leaves_count+1
                        tree_matrix[level][i][j][k]=255
    #print("Cumulated leaves:", leaves_count)

print("Total leaves:", leaves_count)
target_size=int(leaves_count/subtree_number)
print("Subtree number:", subtree_number, "Expected subtree size:",target_size)

index_map=read_SFC(folder_path+'/'+file_path, domain_offset, domain_size)

'''
for i in range(len(index_map)):
    print(index_map[i], end=' ')
    if (i<len(index_map)-1):
        if index_map[i][0] != index_map[i+1][0]:
            print(" ")
'''

current_subtree_label=2 #0 for no entry, 1 for non-leaf node, 255 for leaf node, so start with 2
current_subtree_size=0
for current_node in index_map:
    current_node_flag=tree_matrix[current_node[0]-1][current_node[1]][current_node[2]][current_node[3]]
    if current_node_flag==1:
        tree_matrix[current_node[0]-1][current_node[1]][current_node[2]][current_node[3]]=current_subtree_label
    elif current_node_flag==255:
        tree_matrix[current_node[0]-1][current_node[1]][current_node[2]][current_node[3]]=current_subtree_label
        current_subtree_size=current_subtree_size+1

    if current_subtree_size >= target_size:
        current_subtree_label=current_subtree_label+1

