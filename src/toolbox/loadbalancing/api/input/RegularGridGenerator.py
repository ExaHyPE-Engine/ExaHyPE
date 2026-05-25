# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys, os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from Tree import Tree

"""
Regular Grid Generator for Peano Load Balancing API

This module provides functionality to create artificial regular grids for testing and benchmarking 
the offline load balancing API. It serves as one of the primary input generators for controlled 
experiments and algorithm development.

## Purpose and Context

The RegularGridGenerator is designed for testing and development scenarios where controlled, 
predictable mesh structures are needed. Instead of reading complex adaptive meshes from files, 
this module enables:

1. **Algorithm Testing**: Create predictable test cases for splitting strategy development
2. **Performance Benchmarking**: Generate grids of known complexity for performance analysis
3. **Controlled Experiments**: Ensure reproducible test conditions across different scenarios
4. **Development Support**: Provide simple test cases during API development and debugging

## Integration with Load Balancing API

This module integrates with other components of the load balancing API:

- **Tree Class**: Generates Tree objects with uniform structure (see Tree.py)
- **OfflineDecomposition**: Used by the main script for artificial grid generation
- **SplitTrees**: Provides uniform tree data for splitting strategy testing
- **TreeVisualizer**: Enables visualization of generated regular structures

## Workflow Integration

This module fits into the standard load balancing workflow:

1. **Development Phase**: Create controlled test cases for algorithm development
2. **Grid Generation**: Generate uniform grids with specified refinement levels
3. **Baseline Establishment**: Provide known starting conditions for experiments
4. **Strategy Testing**: Apply splitting strategies to predictable structures
5. **Performance Analysis**: Measure algorithm performance on uniform loads
6. **Comparison Studies**: Compare different strategies on identical input structures

## Usage Examples

```python
# Generate a 2D regular grid with 4 refinement levels
tree_2d = RegularGridGenerator(
    name="Test2D_Uniform", 
    max_level=4, 
    dimension=2
)

# Generate a 3D regular grid for performance testing
tree_3d = RegularGridGenerator(
    name="Benchmark3D", 
    max_level=3, 
    dimension=3
)
```

## Advantages for Testing

- **Reproducibility**: Same parameters always generate identical trees
- **Scalability**: Easy to create grids of different complexities
- **Predictability**: Known cell counts and distribution patterns
- **Simplicity**: No file dependencies or complex input processing
- **Speed**: Fast generation for rapid testing cycles

## Comparison with GridPatchFileReader

| Aspect | RegularGridGenerator | GridPatchFileReader |
|--------|---------------------|-------------------|
| **Data Source** | Artificially generated | Real simulation checkpoints |
| **Structure** | Uniform regular grids | Adaptive mesh refinement |
| **Predictability** | Completely predictable | Depends on simulation |
| **Complexity** | Simple uniform patterns | Complex adaptive patterns |
| **Use Case** | Testing and benchmarking | Real-world optimization |

## Integration with Main Script

The `src/toolbox/loadbalancing/api/OfflineDecomposition.py` script uses this generator when:

```bash
# Generate 2D regular grid with 4 levels
python3 OfflineDecomposition.py -d 2 -l 4

# Generate 3D regular grid with visualization
python3 OfflineDecomposition.py -d 3 -l 3 -p

# Generate and split regular grid
python3 OfflineDecomposition.py -d 3 -l 3 -p -s 4
```

See the load balancing API documentation for complete workflow integration details.
"""

def RegularGridGenerator(name=None, max_level=None, dimension=None):
    """
    Generate an artificial regular grid with uniform refinement for testing purposes.
    
    This function creates a complete regular grid structure where all regions are
    refined uniformly to the specified maximum level. All cells are initially
    assigned to subtree 0, providing a clean starting point for load balancing
    experiments and algorithm testing.
    
    ## Grid Structure
    
    The generated grid follows Peano's hierarchical structure:
    - Level 0: 3^1 cells per dimension (3x3 for 2D, 3x3x3 for 3D)
    - Level k: 3^(k+1) cells per dimension
    - All cells have weight=1 (exist) and subtree_id=0 (single resource)
    
    ## Use Cases
    
    - **Algorithm Development**: Test splitting strategies on predictable structures
    - **Performance Benchmarking**: Measure algorithm performance with known complexity
    - **Regression Testing**: Ensure consistent behavior across code changes
    - **Educational Examples**: Demonstrate API functionality with simple cases
    
    Args:
        name (str, optional): Descriptive name for the generated tree (e.g., "TestGrid_L4").
            If not provided, a deterministic default name is generated from dimension and level.
        max_level (int): Maximum refinement level (must be > 0)
        dimension (int): Spatial dimension (2 for 2D, 3 for 3D)
        
    Returns:
        Tree: Tree object with complete regular grid structure
        
    Example:
        ```python
        # Create a 2D regular grid for testing
        test_tree = RegularGridGenerator(
            name="Uniform_Test_2D",
            max_level=4,
            dimension=2
        )
        
        print(f"Generated tree: {test_tree._name}")
        print(f"Total cells: {test_tree._leaf_count}")
        print(f"Levels: {test_tree._max_level}")
        
        # Apply splitting for load balancing test
        from Strategies.SplitTrees import split_tree
        split_tree(test_tree, number_of_splits=3)
        ```
        
    Note:
        - Domain offset and size are set to placeholder values (origin and unit size)
        - For large max_level values, memory usage grows as 3^(dimension * max_level)
        - Generated trees are deterministic - same parameters always produce identical results
    """
    if max_level is None:
        raise ValueError("max_level must be provided")

    if dimension is None:
        raise ValueError("dimension must be provided")

    if max_level <= 0:
        raise ValueError(f"max_level must be > 0, got {max_level}")
    
    if dimension not in [2, 3]:
        raise ValueError(f"dimension must be 2 or 3, got {dimension}")
    
    if name is None:
        name = f"regular-grid-{dimension}d-l{max_level}"

    print(f"Generating {dimension}D regular grid '{name}' with {max_level} levels")
    
    # Calculate the tree matrices
    artificial_arrays = []
    total_cells = 0
    
    for level in range(max_level):
        size = 3**(level+1)
        level_cells = size ** dimension
        total_cells += level_cells
        
        print(f"  Level {level+1}: {size}^{dimension} = {level_cells:,} cells")
        
        if dimension == 3:
            array = np.empty((size, size, size), dtype=object)
            for a in range(size):
                for b in range(size):
                    for c in range(size):
                        array[a][b][c] = (1, 0)  # weight=1, subtree_id=0
        elif dimension == 2:
            array = np.empty((size, size), dtype=object)
            for a in range(size):
                for b in range(size):
                    array[a][b] = (1, 0)  # weight=1, subtree_id=0

        artificial_arrays.append(array)

    print(f"Total cells generated: {total_cells:,}")

    # Create the Tree class and assign properly
    # Domain offset and size are placeholder values for artificial grids
    if dimension == 3:
        offset = [0, 0, 0]
        size = [1, 1, 1]
    elif dimension == 2:
        offset = [0, 0]
        size = [1, 1]

    # Create and populate the tree
    peano_tree = Tree(name, dimension, offset, size)
    peano_tree.read_in_tree_from_artificial(
        input_artificial_arrays=artificial_arrays, 
        max_level=max_level, 
        weighted=False
    )

    print(f"Regular grid generation completed successfully")
    print(f"  Tree: {peano_tree._name}")
    print(f"  Leaf count: {peano_tree._leaf_count}")
    print(f"  Subtree count: {peano_tree._subtree_count}")

    return peano_tree