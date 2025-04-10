import numpy as np
import matplotlib.pyplot as plt
import math
import sys
import os
import matplotlib.cm as cm

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from Tree import Tree


def TreeVisualizer(input_tree):
    fig = plt.figure(figsize=(12, 4))
    axes=[]
    for level in range(input_tree._max_level):
        plot_array=input_tree._tree_arrays[level]

        indices = []
        subtree_ids = []
        for a in range(3**(level+1)):
            for b in range(3**(level+1)):
                for c in range(3**(level+1)):
                    node, subtree_id = plot_array[a][b][c]
                    if node != 0:
                        indices.append((a, b, c))
                        subtree_ids.append(subtree_id)

        indices = np.array(indices)
        subtree_ids = np.array(subtree_ids)

        axes.append(fig.add_subplot(1, input_tree._max_level, level+1, projection='3d'))
        scatter = axes[level].scatter(indices[:, 0], indices[:, 1], indices[:, 2], c=subtree_ids, cmap='tab10', s=200/(level+1)**2, depthshade=True, marker='s')

        axes[level].set_xlabel('X')
        axes[level].set_ylabel('Y')
        axes[level].set_zlabel('Z')

        axes[level].set_xlim([0,3**(1+level)-1])
        axes[level].set_ylim([0,3**(1+level)-1]) 
        axes[level].set_zlim([0,3**(1+level)-1]);
        axes[level].set_title("Level="+str(level+1))

    # Show the 3D plot
    plt.show()
    plt.savefig("TreeStructure.pdf")
    plt.savefig("TreeStrucutre.png")
