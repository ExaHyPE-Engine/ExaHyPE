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


def main():
    #main start here
    #input parameters
    #python3  ccz4.py -impl fd4-rk1-adaptive -s gauge -maxh 0.05 -minh 0.02 -ps 3 -plt 0.1 -et 0.05 -exn test --domain_r 0.5 --ReSwi 1 -cfl 0.1 --KOSigma 8.0

    folder_path = 'case2'
    prefix = 'grid-'
    # List all files in the folder with the specified prefix
    datafiles = [f for f in os.listdir(folder_path) if f.startswith(prefix)]

    dimension=3

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

        matrix=np.zeros((matrix_size, matrix_size, matrix_size), dtype=bool)
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


    #plotting stuff
    fig = plt.figure(figsize=(12, 4))

    for i in range(total_level):
        ax = fig.add_subplot(1,total_level,i+1, projection='3d')
        x, y, z = np.where(tree_matrix[i])
        ax.scatter(x, y, z, c='black', marker='s',s=50*27/3**(1+i))  # 'c' controls color, 'marker' controls shape

        # Set labels
        ax.set_xlabel("X"); ax.set_ylabel("Y"); ax.set_zlabel("Z") 
        ax.set_xlim([0,3**(1+i)-1]); ax.set_ylim([0,3**(1+i)-1]); ax.set_zlim([0,3**(1+i)-1]);
        ax.set_title("Level="+str(i)+", Entries="+str(tree_patches[i]))

    # Show the 3D plot
    plt.show()
    plt.savefig("TreeStructure.pdf")
    plt.savefig("TreeStrucutre.png")


if __name__ == "__main__":
    main()