import numpy as np
import matplotlib.pyplot as plt
from scipy import io
import os
import math
import argparse
import matplotlib.cm as cm

import re
from collections import defaultdict

def round_sig(x, sig):
    return float(np.format_float_positional(x, precision=sig, unique=False, trim='k'))

def read_patches(file_path, size_group):
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
        size_group[size].append(offset)  # Group offsets by size

    return size_group


#main start here
#input parameters
parser = argparse.ArgumentParser(description='Tree Reader')
parser.add_argument("-file", "--file-path", dest="file", type=str, required="True",  help="file path" )

args = parser.parse_args()

#folder_path = 'case1'
#input parameters, case5 is bottomup(one big tree), case4 is topdown(bad pattern)
folder_path=args.file
prefix = 'grid-'
# List all files in the folder with the specified prefix
datafiles = [f for f in os.listdir(folder_path) if f.startswith(prefix)]

dimension=3

#extract grid infomation
size_groups = []   # Dictionary to store {size: [offsets]}
for file_path in datafiles: 
    size_group = defaultdict(list)
    read_patches(folder_path+'/'+file_path, size_group)
    size_groups.append(size_group)


total_level=-1
for size_group in size_groups:
    total_level=max(total_level, len(size_group.keys()))


patch_number_for_each_level = defaultdict(list)
for size_group in size_groups:
    for size, offsets in size_group.items():
        print(f"Size: {size} -> Count: {len(offsets)}")
        patch_number_for_each_level[size].append(len(offsets))
    #print("Offsets:")
    #for offset in offsets:
    #    print(f"  {offset}")
    print("-" * 40)
#print(total_patch_count)
print("Total Cell Count:")
for size, count in patch_number_for_each_level.items():
    print(f"Size: {size} -> Count: {sum(count)}")


#find the domain info directly from grid plot
max_size=0

for size_group in size_groups:
    for size in size_group.keys():
    	size_tuple = tuple(map(float, size.split()))
    	if size_tuple[0]>max_size:
    		max_size=size_tuple[0]
    		max_size_key=size
#print(max_size)

leftmost_offset=[999,999,999]
for size_group in size_groups:
    for offset in size_group[max_size_key]:
        offset_tuple = tuple(map(float, offset.split()))
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
#total_level=len(size_group.keys())
#for i in range(total_level):
#	print("level "+str(i)+" Matrix total entries:", (3*(3**i))**dimension,"size: "+str(domain_size[0]/3/3**i))
tree_matrices = []
tree_patches  = []

#print(len(tree_matrix))

#calculate the tree matrices
index=0
for size_group in size_groups:
    tree_matrix   = [None]*total_level
    tree_patch    = [None]*total_level
    for size, offsets in size_group.items():
        size_tuple = tuple(map(float, size.split()))
        level=int( math.log( int(round_sig(domain_size[0]/size_tuple[0],1)), 3) )
        matrix_size=(3**level)
        #print(level, matrix_size)

        matrix=np.zeros((matrix_size, matrix_size, matrix_size), dtype=bool)
        for offset in offsets:
            offset_tuple = tuple(map(float, offset.split()))
            index=[-1,-1,-1]
            for i in range(len(offset_tuple)):
                index[i]=round((offset_tuple[i]-domain_offset[i])/size_tuple[i])
            matrix[tuple(index)]=1

        #print(len(tree_matrix))
        tree_matrix[level-1]=matrix
        tree_patch[level-1]=len(offsets)
    tree_matrices.append(tree_matrix)
    tree_patches.append(tree_patch)

#for i in range(len(datafiles)):
#    print(tree_patches[i])

#colors=["black","red"]
colors = plt.colormaps['tab10'] 

#plotting stuff
fig = plt.figure(figsize=(12, 4))
axes=[]
for k in range(total_level):
    axes.append(fig.add_subplot(1,total_level,k+1, projection='3d'))

for j in range(len(datafiles)):
    color=colors(j)
    for i in range(total_level):
        x, y, z = np.where(tree_matrices[j][i])
        axes[i].scatter(x, y, z, color=color, marker='s',s=50*27/3**(1+i), depthshade=False)  # 'c' controls color, 'marker' controls shape

        # Set labels
        axes[i].set_xlabel("X"); axes[i].set_ylabel("Y"); axes[i].set_zlabel("Z") 
        axes[i].set_xlim([0,3**(1+i)-1]); axes[i].set_ylim([0,3**(1+i)-1]); axes[i].set_zlim([0,3**(1+i)-1]);
        axes[i].set_title("Level="+str(i))

# Show the 3D plot
plt.show()
plt.savefig("TreeStructure.pdf")
plt.savefig("TreeStrucutre.png")
