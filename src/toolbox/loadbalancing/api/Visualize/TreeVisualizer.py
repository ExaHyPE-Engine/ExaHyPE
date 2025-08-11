# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import matplotlib.pyplot as plt
import sys, os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from Tree import Tree


def TreeVisualizer(input_tree, dimension):
    fig = plt.figure(figsize=(9*input_tree._max_level, 8))
    axes=[]
    for level in range(input_tree._max_level):
        plot_array=input_tree._tree_arrays[level]

        indices = []
        subtree_ids = []
        # extrat the existing nodes and their indices from the tree arrays and 
        # plot them in a 3D/2D scatter
        if dimension==3:
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
            scatter = axes[level].scatter(indices[:, 0], indices[:, 1], indices[:, 2], c=subtree_ids, cmap='tab10', s=1000/(level+1)**3, depthshade=True, marker='s')

            axes[level].set_xlabel('X')
            axes[level].set_ylabel('Y')
            axes[level].set_zlabel('Z')

            axes[level].set_xlim([0,3**(1+level)-1])
            axes[level].set_ylim([0,3**(1+level)-1]) 
            axes[level].set_zlim([0,3**(1+level)-1]);
            axes[level].set_title("Level="+str(level+1))

        elif dimension==2:
            for a in range(3**(level+1)):
                for b in range(3**(level+1)):
                    node, subtree_id = plot_array[a][b]
                    if node != 0:
                        indices.append((a, b))
                        subtree_ids.append(subtree_id)

            indices = np.array(indices)
            subtree_ids = np.array(subtree_ids)

            axes.append(fig.add_subplot(1, input_tree._max_level, level+1))
            scatter = axes[level].scatter(indices[:, 0], indices[:, 1], c=subtree_ids, cmap='tab10', s=5000/(level+1)**4, marker='s')

            axes[level].set_xlabel('X')
            axes[level].set_ylabel('Y')

            axes[level].set_xlim([-1,3**(1+level)])
            axes[level].set_ylim([-1,3**(1+level)]) 
            axes[level].set_title("Level="+str(level+1))

    # Save the figure BEFORE showing it
    plt.savefig("TreeStructure.pdf")
    plt.savefig("TreeStructure.png")
    
    # Show the plot after saving
    plt.show()
