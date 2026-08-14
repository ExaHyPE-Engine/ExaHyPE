# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""!

Legacy lexicographic Cartesian ordering helpers.

This module provides the standard row-major lexicographic mapping that was
used by an earlier version of the leaf distribution before the traversal was
switched to Peano SFC order in `SplitTrees.py`.

This strategy runs through the mesh bottom up. It first assigns all leaves
on the finest level to partitions, then the leaves on the next coarser 
level, and so forth. This strategy is biased towards the finest mesh and 
particularly valuable for strategies which do more work on the finest level 
than on other cells. This holds for example for some p-multigrid solvers or 
ExaHyPE with limiter where the limiter typically kicks in on the finest 
mesh resolution only.

We do assign indices per level along the Peano space filling curve. However,
whenever we encounter an AMR transition, we stop assigning cells to a rank.
That is, if there are 27 connected fine grid cells and each subtree should 
have 15 cells, then we assign the first 15 cells to tree 0. Tree 1 then 
get the remaining 27-15 cells. After that, we continue with tree 2. Through
this, we avoid that AMR partitions become disconnected:

@image html distribute_leaves_bottom_up.png

The ordering itself is straightforward:
- 2-D: `(x, y) -> x + y * size`
- 3-D: `(x, y, z) -> x + y * size + z * size^2`

This module keeps the legacy helpers isolated from the SFC utilities so the
Peano-specific code can stay focused on SFC order.
"""


def Cartesian_to_lexicographic_2d(
    coordinates: tuple[int, int],
    size: int,
):
    """Map a 2-D Cartesian index to standard lexicographic order."""
    result = 0
    base = 1
    for i in range(0, 2):
        result += coordinates[i] * base
        base *= size
    return result


def Cartesian_to_lexicographic_3d(
    coordinates: tuple[int, int, int],
    size: int,
):
    """Map a 3-D Cartesian index to standard lexicographic order."""
    result = 0
    base = 1
    for i in range(0, 3):
        result += coordinates[i] * base
        base *= size
    return result