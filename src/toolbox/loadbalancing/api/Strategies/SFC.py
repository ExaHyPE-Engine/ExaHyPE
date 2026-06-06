# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
def Cartesian_to_lexicographic_2d(
    coordinates: tuple[int,int],
    size: int
):
    """!
    
    Takes a Cartesian index and maps it onto a sequence along the
    standard lexicographic order.
    
    Args:
        coordinates: Index with entries between 0 and size-1
        size:        Specifies a Cartesian mesh of size [size]x[size]

    """
    result = 0
    base   = 1
    for i in range(0,2):
        result += coordinates[i] * base
        base   *= size
    return result
              
    
def Cartesian_to_lexicographic_3d(
    coordinates: tuple[int,int,int],
    size: int
):
    """!
    
    Takes a Cartesian index and maps it onto a sequence along the
    standard lexicographic order.
    
    
    Args:
        coordinates: Index with entries between 0 and size-1
        size:        Specifies a Cartesian mesh of size [size]x[size]x[size]

    """
    result = 0
    base   = 1
    for i in range(0,3):
        result += coordinates[i] * base
        base   *= size
    return result
        
    
def Cartesian_to_Peano_2d(
    coordinates: tuple[int,int],
    size: int
) -> int:
    """!
    
    Map Peano index to integer
    
    This operation accepts a Cartesian index and returns the corresponding
    Peano index. In the example below
    
    @image html PeanoSFC.png
    
    we can hand in (4,2) and get a 10 back.
    
    """
    return [1,1]



def Cartesian_to_Peano_3d(
    coordinates: tuple[int,int,int],
    size: int
) -> int:
    return [1,1,1]



def Peano_to_Cartesian_2d(
    peano_index: int,
    size: int
) -> tuple[int,int]:
    """!
    
    Takes an index 
    """
    return [1,1]


def Peano_to_Cartesian_3d(
    peano_index: int,
    size: int
) -> tuple[int,int,int]:
    return [1,1,1]
