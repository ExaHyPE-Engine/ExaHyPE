"""
Reusable tree builder functions for integration tests.

Each function constructs and returns a fresh :class:`Tree` instance with all
cells initially assigned to subtree 0.  The trees are intentionally kept small
and correspond to the reference diagrams under
``documentation/toolbox/loadbalancing/``.
"""

import os
import sys

import numpy as np

# Ensure the loadbalancing API is importable when running pytest from repo root
sys.path.insert(0, os.path.abspath("src/toolbox/loadbalancing/api"))

from Tree import Tree
from input.RegularGridGenerator import RegularGridGenerator


def build_tree3_adaptive_2d() -> Tree:
    """
    Build the adaptive 2D tree described by the tree3_stage*_fine_first_bfs.dot
    documentation files.

    The tree starts with a **single root node on Level 0** and then embeds the
    1D ternary structure in the first row of deeper 2D levels (all cells outside
    row 0 have weight=0).

    Mapping:
        Level 0  (3x3):   (0,0)=Root (single node)
        Level 1  (9x9):   (0,0)=L1A  (0,1)=L1B  (0,2)=L1C
        Level 2  (27x27): (0,0)=L2A1 (0,1)=L2A2 (0,2)=L2A3 [childless leaf]
                          (0,3)=L2B1 (0,4)=L2B2 (0,5)=L2B3 [childless leaf]
                          (0,6)=L2C1 (0,7)=L2C2 (0,8)=L2C3
        Level 3  (81x81): (0,0-2)=L3A1*  (0,3-5)=L3A2*
                          (0,6-8)=weight=0  [L2A3 has no children]
                          (0,9-11)=L3B1*  (0,12-14)=L3B2*
                          (0,15-17)=weight=0  [L2B3 has no children]
                          (0,18-20)=L3C1*  (0,21-23)=L3C2*  (0,24-26)=L3C3*

    Total leaves initially (all in subtree 0):
        21 finest-level leaves + 2 childless level-1 leaves = 23

    The tree uses 4 total subtrees in the tests below.
    """
    tree = Tree("tree3-adaptive", 2, (0.0, 0.0), (1.0, 1.0))
    tree._max_level = 4

    # Initialise all levels with non-existent cells (weight=0)
    for level in range(4):
        size = 3 ** (level + 1)
        arr = np.empty((size, size), dtype=object)
        for a in range(size):
            for b in range(size):
                arr[a][b] = (0, 0)
        tree._tree_arrays.append(arr)

    # Level 0: single root node
    tree._tree_arrays[0][0][0] = (1, 0)

    # Level 1: L1A, L1B, L1C in first row (all subtree 0)
    tree._tree_arrays[1][0][0] = (1, 0)
    tree._tree_arrays[1][0][1] = (1, 0)
    tree._tree_arrays[1][0][2] = (1, 0)

    # Level 2: 9 nodes in first row (all subtree 0).
    # L2A3=(0,2) and L2B3=(0,5) are childless leaves (no level-2 children).
    for col in range(9):
        tree._tree_arrays[2][0][col] = (1, 0)

    # Level 3: finest leaves in first row, skipping children of the childless
    # L2 nodes (columns 6-8 under L2A3 and columns 15-17 under L2B3 stay at
    # weight=0 so that L2A3 and L2B3 are detected as childless leaves).
    active_level2_cols = list(range(0, 6)) + list(range(9, 15)) + list(range(18, 27))
    for col in active_level2_cols:
        tree._tree_arrays[3][0][col] = (1, 0)

    tree._subtree_count = 1
    tree.update_split_pattern()

    return tree


def build_tree3_all_rows_2d() -> Tree:
    """
    Build a 2D variant where each depicted node in the tree3 diagram is
    represented by all rows with respect to its parent (3 cells in 2D), while
    Level 0 still starts from a single root node.

    Representation rule in 2D:
    - For each conceptual parent-child edge, one conceptual child node is
      represented as a vertical stripe of 3 cells in the parent's 3x3 child
      patch.
    - Conceptual child indices 0,1,2 map to local child columns 0,1,2.

    This means all represented non-leaf parents have all 9 children present,
    which allows Stage III to optimise mixed parents at Level 1.
    """
    tree = Tree("tree3-all-rows", 2, (0.0, 0.0), (1.0, 1.0))
    tree._max_level = 4

    # Initialise all levels with non-existent cells (weight=0)
    for level in range(4):
        size = 3 ** (level + 1)
        arr = np.empty((size, size), dtype=object)
        for a in range(size):
            for b in range(size):
                arr[a][b] = (0, 0)
        tree._tree_arrays.append(arr)

    def _fill_child_stripes(parent_level: int, parent_coords: tuple[int, int], child_cols: tuple[int, ...]):
        """Fill 3-row stripes for selected conceptual child columns."""
        child_level = parent_level + 1
        parent_a, parent_b = parent_coords
        base_a = 3 * parent_a
        base_b = 3 * parent_b
        for local_col in child_cols:
            for local_row in range(3):
                a = base_a + local_row
                b = base_b + local_col
                tree._tree_arrays[child_level][a][b] = (1, 0)

    # Level 0: single root
    tree._tree_arrays[0][0][0] = (1, 0)

    # Root -> L1A/L1B/L1C as full-row stripes (all 9 children exist at level 1)
    _fill_child_stripes(parent_level=0, parent_coords=(0, 0), child_cols=(0, 1, 2))

    # Level 1 -> Level 2 (L2*): all three conceptual children exist for each L1 node
    for a in range(3):
        for b in range(3):
            if tree._tree_arrays[1][a][b][0] == 1:
                _fill_child_stripes(parent_level=1, parent_coords=(a, b), child_cols=(0, 1, 2))

    # Level 2 -> Level 3 (L3*):
    # - For A3 and B3 conceptual nodes (parent group A/B with local child index 2),
    #   keep them childless by not creating level-3 children.
    # - All other represented L2 nodes are refined with three conceptual children,
    #   each expanded to a 3-row stripe.
    for a in range(9):
        for b in range(9):
            if tree._tree_arrays[2][a][b][0] != 1:
                continue

            parent_group = b // 3    # 0=A, 1=B, 2=C
            local_index = b % 3      # 0=1, 1=2, 2=3

            is_childless = parent_group in (0, 1) and local_index == 2
            if not is_childless:
                _fill_child_stripes(parent_level=2, parent_coords=(a, b), child_cols=(0, 1, 2))

    tree._subtree_count = 1
    tree.update_split_pattern()

    return tree


def build_stage3_mixed_parent_case_2d() -> tuple[Tree, tuple[int, int]]:
    """
    Build a 2D tree where Stage III can promote a mixed parent.

    The returned parent coordinates identify the level-0 parent whose children
    are configured with a dominant subtree assignment.
    """
    tree = RegularGridGenerator(name="pytest-stage3-integration-2d", max_level=2, dimensions=2)

    parent_level = 0
    child_level = 1
    parent_coords = (0, 0)

    # Parent starts in subtree 0.
    tree._tree_arrays[parent_level][parent_coords] = (1, 0)

    children = [
        (a, b)
        for a in range(3 * parent_coords[0], 3 * parent_coords[0] + 3)
        for b in range(3 * parent_coords[1], 3 * parent_coords[1] + 3)
    ]

    # Mixed children with subtree 2 dominant.
    for coords in children[:5]:
        tree._tree_arrays[child_level][coords] = (1, 2)
    for coords in children[5:8]:
        tree._tree_arrays[child_level][coords] = (1, 0)
    tree._tree_arrays[child_level][children[8]] = (1, 1)

    return tree, parent_coords
