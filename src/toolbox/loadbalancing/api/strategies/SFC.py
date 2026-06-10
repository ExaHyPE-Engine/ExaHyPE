# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
        
    
def Cartesian_to_Peano_2d(
    coordinates: tuple[int,int],
    size: int
) -> int:
    """!

    Map Cartesian index to Peano SFC index.

    This operation accepts a Cartesian index and returns the corresponding
    Peano SFC index.  In the example below

    @image html PeanoSFC.png

    we can hand in (4,2) and get a 10 back.

    The mapping is defined recursively.  The grid is divided into a 3x3
    arrangement of sub-blocks.  The 9 coarse blocks are numbered via a
    row-major serpentine (even rows left-to-right, odd rows right-to-left).
    Within each coarse block the same rule is applied recursively.  If the
    coarse-block index is odd the local sub-block index is reversed so that
    the global 1-D ordering remains spatially coherent.

    Args:
        coordinates: (x, y) index with entries in [0, size-1]
        size:        Side length of the square grid; must be a power of 3.

    Returns:
        Integer Peano SFC index in [0, size*size - 1].

    """
    x, y = coordinates
    if size == 1:
        return 0

    sub_size = size // 3

    cx = x // sub_size
    cy = y // sub_size

    # Row-major serpentine: even rows go left-to-right, odd rows right-to-left.
    if cy % 2 == 0:
        coarse_idx = cy * 3 + cx
    else:
        coarse_idx = cy * 3 + (2 - cx)

    lx = x % sub_size
    ly = y % sub_size

    local_idx = Cartesian_to_Peano_2d((lx, ly), sub_size)

    # Odd coarse index: reverse the local ordering so the curve is spatially coherent.
    if coarse_idx % 2 == 1:
        local_idx = sub_size * sub_size - 1 - local_idx

    return coarse_idx * sub_size * sub_size + local_idx



def Cartesian_to_Peano_3d(
    coordinates: tuple[int,int,int],
    size: int
) -> int:
    """!

    Map 3-D Cartesian index to Peano SFC index.

    Extends `Cartesian_to_Peano_2d` to three dimensions.  The 27 coarse
    blocks are ordered by stacking xy-planes along z.  Even z-planes use
    the forward 2-D coarse index; odd z-planes use the reversed 2-D coarse
    index so that the end of one z-plane connects to the start of the next.
    The same odd-index reversal rule is then applied to local sub-block
    indices recursively.

    Args:
        coordinates: (x, y, z) index with entries in [0, size-1]
        size:        Side length of the cubic grid; must be a power of 3.

    Returns:
        Integer Peano SFC index in [0, size**3 - 1].

    """
    x, y, z = coordinates
    if size == 1:
        return 0

    sub_size = size // 3

    cx = x // sub_size
    cy = y // sub_size
    cz = z // sub_size

    # 2-D coarse index for the xy-plane.
    if cy % 2 == 0:
        coarse_2d = cy * 3 + cx
    else:
        coarse_2d = cy * 3 + (2 - cx)

    # 3-D coarse index: even z-planes use forward 2-D order,
    # odd z-planes use reversed 2-D order so consecutive planes connect.
    if cz % 2 == 0:
        coarse_idx = cz * 9 + coarse_2d
    else:
        coarse_idx = cz * 9 + (8 - coarse_2d)

    lx = x % sub_size
    ly = y % sub_size
    lz = z % sub_size

    local_idx = Cartesian_to_Peano_3d((lx, ly, lz), sub_size)

    if coarse_idx % 2 == 1:
        local_idx = sub_size ** 3 - 1 - local_idx

    return coarse_idx * sub_size ** 3 + local_idx



def Peano_to_Cartesian_2d(
    peano_index: int,
    size: int
) -> tuple[int,int]:
    """!

    Inverse of `Cartesian_to_Peano_2d`.

    Args:
        peano_index: SFC index in [0, size*size - 1].
        size:        Side length of the square grid; must be a power of 3.

    Returns:
        (x, y) Cartesian coordinates.

    """
    if size == 1:
        return (0, 0)

    sub_size = size // 3
    sub_area = sub_size * sub_size

    coarse_idx = peano_index // sub_area
    local_idx  = peano_index % sub_area

    if coarse_idx % 2 == 1:
        local_idx = sub_area - 1 - local_idx

    cy          = coarse_idx // 3
    cx_in_row   = coarse_idx % 3
    cx = cx_in_row if cy % 2 == 0 else (2 - cx_in_row)

    lx, ly = Peano_to_Cartesian_2d(local_idx, sub_size)

    return (cx * sub_size + lx, cy * sub_size + ly)


def Peano_to_Cartesian_3d(
    peano_index: int,
    size: int
) -> tuple[int,int,int]:
    """!

    Inverse of `Cartesian_to_Peano_3d`.

    Args:
        peano_index: SFC index in [0, size**3 - 1].
        size:        Side length of the cubic grid; must be a power of 3.

    Returns:
        (x, y, z) Cartesian coordinates.

    """
    if size == 1:
        return (0, 0, 0)

    sub_size   = size // 3
    sub_volume = sub_size ** 3

    coarse_idx = peano_index // sub_volume
    local_idx  = peano_index % sub_volume

    if coarse_idx % 2 == 1:
        local_idx = sub_volume - 1 - local_idx

    cz        = coarse_idx // 9
    coarse_2d = coarse_idx % 9
    if cz % 2 == 1:
        coarse_2d = 8 - coarse_2d

    cy        = coarse_2d // 3
    cx_in_row = coarse_2d % 3
    cx = cx_in_row if cy % 2 == 0 else (2 - cx_in_row)

    lx, ly, lz = Peano_to_Cartesian_3d(local_idx, sub_size)

    return (cx * sub_size + lx, cy * sub_size + ly, cz * sub_size + lz)
