# This file is part of the Peano load balancing toolbox. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org


def real_mesh_size_and_level(prescribed_h,
                             domain_size,
                             ):
    """!

    Returns the real mesh size plus the level used

    Peano uses three-partitioning. That is, it will neve be able to match
    given refinement instructions exactly. All it can do is to approximate
    them. 

    This routine is a copy of swift2.Project.real_mesh_size(), but I didn't 
    want to have dependencies between the two packages, i.e. swift2 and 
    the load balancing. Therefore, copying seems to be appropriate.

    Return a tuple of mesh size and mesh levels

    """
    h = domain_size
    level = 0
    while h > prescribed_h:
        h = h / 3.0
        level += 1
    return (h, level)


def compute_regular_mesh_construction_grid_construction_sweeps(level):
    """!
    
    Determine after which step to split
    
    This routine is given the level of a mesh and then determines how many
    mesh construction sweeps Peano will need to get to this level. Usually,
    people use this information to determine when to issue split instructions.
    The routine is typically used with the second argument returned by
    real_mesh_size_and_level().

    The calculation is close to trivial: As Peano realises a strict 
    flag-then-refine paradigm, it needs two mesh traversals to add a completely
    new mesh level to a spacetree grid. The first one flags all the fine grid
    cells, the second one then does the actual refinement.
    
    When we have just completed the very last refinement, we have to wait for 
    one more step to ensure that all the stats are 
    
    """
    return 2*level+0
