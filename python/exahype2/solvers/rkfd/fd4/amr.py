# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.rkfd.CellCenteredFiniteDifferences import CellCenteredFiniteDifferences


#
# We use the render operations here, too
#
import numpy as np
import scipy.sparse as sp
import itertools
from exahype2.solvers.LagrangeBasis import render_tensor_2
from exahype2.solvers.LagrangeBasis import render_tensor_1

def linearise_index(index, dimensions):
    linear = index[0]
    factor = 1
    for i in range(1, dimensions.size):
        factor *= dimensions[-i]
        linear += factor * index[i]
    return linear

def linearise_index_reverse(index, dimensions):
    linear = index[0]
    factor = 1
    for i in range(1, dimensions.shape[0]):
        factor *= dimensions[i - 1]
        linear += factor * index[i]
    return linear


def __compute_interpolation_matrix_nd_csr(patch_size, halo_size, patch_index_linear = None, d=3):
    """
    
    Compute the interpolation matrix, which used to prepare halo layer data for fine cells at the refinement boundary.
    The matrix is an nd mapping, so each axis is interpolated by one matrix.

    For a matrix P_ij that is generated, the mapping between coarse and fine values is
    
    u^f_i = P_ij u^c_j
    
    """

    fine_face_dimensions = np.full(d, 3 * patch_size, dtype=np.uint32) # Set one dimension to k, and the rest to 3p
    fine_face_dimensions[-1] = halo_size
    coarse_face_dimensions = np.full(d, patch_size, dtype=np.uint32) # Set one dimension to 2k, and the rest to p
    coarse_face_dimensions[-1] = 2 * halo_size

    patch_index = np.empty(d - 1, dtype=np.int32)
    if patch_index_linear is not None:
        for i in range(d - 1):
            patch_index[i] = patch_index_linear % 3
            patch_index_linear = patch_index_linear // 3

    matrix_size = (np.prod(fine_face_dimensions), np.prod(coarse_face_dimensions))
    matrix = np.zeros(matrix_size)

    fine_index = np.zeros(d, dtype=np.uint32)

    while (fine_index[-1] < fine_face_dimensions[0]):
        coarse_index = fine_index // 3
        coarse_index[0] += halo_size
        position_in_coarse_cell = fine_index % 3

        # Build local interpolation matrix, storing contribution weight from each coarse cell directly surrounding the current coarse cell
        local_interpolation = np.ones(np.full(d, 3, dtype=np.uint32))
        for dimension in range(d):
            index = position_in_coarse_cell[-(dimension + 1)]

            local_interpolation = np.moveaxis(local_interpolation, dimension, 0)
            if index == 0:
                if coarse_index[-(dimension + 1)] != 0:# or dimension == d - 1:
                    local_interpolation[0] /= 3.0
                    local_interpolation[1] *= 2.0 / 3.0
                    local_interpolation[2] = 0.0
                else:
                    local_interpolation[0] = 0.0
                    local_interpolation[1] *= 4.0 / 3.0
                    local_interpolation[2] /= -3.0
            elif index == 2:
                if coarse_index[-(dimension + 1)] != patch_size - 1:#  or dimension == d - 1:
                    local_interpolation[0] = 0.0
                    local_interpolation[1] *= 2.0 / 3.0
                    local_interpolation[2] /= 3.0
                else:
                    local_interpolation[0] /= -3.0
                    local_interpolation[1] *= 4.0 / 3.0
                    local_interpolation[2] = 0.0
            else:
                local_interpolation[0] = 0.0
                local_interpolation[2] = 0.0
            local_interpolation = np.moveaxis(local_interpolation, 0, dimension)    

        matrix_row = linearise_index(fine_index, fine_face_dimensions)

        offset = 3
        if halo_size == 1:
            offset = 2
            local_interpolation = local_interpolation[:, :, 0:2]

        if d == 1:
            matrix[matrix_row, coarse_index[0] - 1 : coarse_index[0] - 1 + offset] = local_interpolation
        elif d == 2:
            matrix[matrix_row, linearise_index(coarse_index, coarse_face_dimensions) - 1 : linearise_index(coarse_index, coarse_face_dimensions) - 1 + offset] = local_interpolation[1]
            if coarse_index[1] != 0:
                matrix[matrix_row, linearise_index(coarse_index - np.array([0, 1]), coarse_face_dimensions) - 1 : linearise_index(coarse_index - np.array([0, 1]), coarse_face_dimensions) - 1 + offset] = local_interpolation[0]
            if coarse_index[1] != patch_size - 1:
                matrix[matrix_row, linearise_index(coarse_index + np.array([0, 1]), coarse_face_dimensions) - 1 : linearise_index(coarse_index + np.array([0, 1]), coarse_face_dimensions) - 1 + offset] = local_interpolation[2]
        elif d == 3:
            matrix[matrix_row, linearise_index(coarse_index + np.array([-1, 0, 0]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, 0, 0]), coarse_face_dimensions) + offset] = local_interpolation[1, 1]

            if coarse_index[2] != 0:
                matrix[matrix_row, linearise_index(coarse_index + np.array([-1, 0, -1]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, 0, -1]), coarse_face_dimensions) + offset] = local_interpolation[0, 1]
            if coarse_index[2] != patch_size - 1:
                matrix[matrix_row, linearise_index(coarse_index + np.array([-1, 0, 1]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, 0, 1]), coarse_face_dimensions) + offset] = local_interpolation[2, 1]

            if coarse_index[1] != 0:
                if coarse_index[2] != 0:
                    matrix[matrix_row, linearise_index(coarse_index + np.array([-1, -1, -1]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, -1, -1]), coarse_face_dimensions) + offset] = local_interpolation[0, 0]
                matrix[matrix_row, linearise_index(coarse_index + np.array([-1, -1, 0]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, -1, 0]), coarse_face_dimensions) + offset] = local_interpolation[1, 0]
                if coarse_index[2] != patch_size - 1:
                    matrix[matrix_row, linearise_index(coarse_index + np.array([-1, -1, 1]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, -1, 1]), coarse_face_dimensions) + offset] = local_interpolation[2, 0]
            if coarse_index[1] != patch_size - 1:
                if coarse_index[2] != 0:
                    matrix[matrix_row, linearise_index(coarse_index + np.array([-1, 1, -1]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, 1, -1]), coarse_face_dimensions) + offset] = local_interpolation[0, 2]
                matrix[matrix_row, linearise_index(coarse_index + np.array([-1, 1, 0]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, 1, 0]), coarse_face_dimensions) + offset] = local_interpolation[1, 2]
                if coarse_index[2] != patch_size - 1:
                    matrix[matrix_row, linearise_index(coarse_index + np.array([-1, 1, 1]), coarse_face_dimensions) : linearise_index(coarse_index + np.array([-1, 1, 1]), coarse_face_dimensions) + offset] = local_interpolation[2, 2]
        
        # Iterate to next fine cell
        fine_index[0] += 1
        for i in range(d - 1):
            if fine_index[i] >= fine_face_dimensions[-(i + 1)]:
                fine_index[i] = 0
                fine_index[i + 1] += 1

    # Extract specific fine patch data
    if patch_index_linear is not None:
        patch_matrix = np.zeros((halo_size * patch_size ** (d - 1), matrix_size[1]))
        if d == 3:
            current_row = patch_index[0] * halo_size * 3 * patch_size ** 2 + patch_index[1] * halo_size * patch_size
            patch_matrix_row = 0

            for i in range(patch_size):
                for j in range(halo_size * patch_size):
                    patch_matrix[patch_matrix_row] = matrix[current_row + j]
                    patch_matrix_row += 1
                current_row += 3 * halo_size * patch_size

        elif d == 2:
            patch_matrix = matrix[halo_size * patch_size * patch_index[0] : halo_size * patch_size * (patch_index[0] + 1)]
        else:
            patch_matrix = matrix
    else:
        patch_matrix = matrix

    sparse_matrix = sp.csr_matrix(patch_matrix)
    return sparse_matrix.data, sparse_matrix.indices, sparse_matrix.indptr


def __compute_restriction_matrix_nd_csr(patch_size, halo_size, patchIndexLinear, d=3):
    """
    
    Compute the interpolation matrix, which used to prepare halo layer data for fine cells at the refinement boundary.
    The matrix is an nd mapping, so each axis is interpolated by one matrix.

    For a matrix P_ij that is generated, the mapping between coarse and fine values is
    
    u^f_i = P_ij u^c_j
    
    """
    patch_index = np.zeros(d - 1)
    patch_index[0] = patchIndexLinear % 3
    if d == 3:
        patch_index[1] = patchIndexLinear // 3

    fine_start = np.zeros(d)
    fine_start[1:] = patch_size * patch_index
    fine_face_dimensions = np.full(d, patch_size, dtype=np.int32)
    fine_face_dimensions[0] = 2 * halo_size

    coarse_start = np.zeros(d, dtype=np.int32)
    coarse_start[1:] = np.floor((patch_size * patch_index + np.ones(d - 1)) / 3)
    coarse_start[0] = halo_size

    coarse_end = np.zeros(d, dtype=np.int32)
    coarse_end[0] = 2 * halo_size
    coarse_end[1:] = np.floor((patch_size * (patch_index + np.ones(d - 1)) + np.ones(d - 1)) / 3)

    coarse_face_dimensions = coarse_end - coarse_start

    matrix_size = (np.prod(coarse_face_dimensions), np.prod(fine_face_dimensions))
    matrix = np.zeros(matrix_size)

    coarse_index = np.zeros(d, dtype=np.int32)

    while (coarse_index[-1] < coarse_face_dimensions[-1]):
        row = linearise_index_reverse(coarse_index, coarse_face_dimensions)
        
        local_restriction = np.zeros((3,) * d)

        if (coarse_index[0] + 1) * 3 <= halo_size:
            if d == 3:
                local_restriction[1, 1] = np.array([1/3, 1/3, 1/3])
            if d == 2:
                local_restriction[1] = np.array([1/3, 1/3, 1/3])
        else:
            ghost_fine_index = coarse_index * 3
            distance = ghost_fine_index[0] - halo_size + 3
            contribution1 = 1 - distance
            contribution2 = distance
            if d == 3:
                local_restriction[1, 1] = np.array([0, contribution1, contribution2])
            if d == 2:
                local_restriction[1] = np.array([0, contribution1, contribution2])

        
        ghost_fine_index = ((coarse_start + coarse_index) * 3 - fine_start).astype(np.int32)
        ghost_fine_index[0] = halo_size# + coarse_index[0]

        if d == 3:
            if ghost_fine_index[1] >= 0 and ghost_fine_index[1] + 2 < fine_face_dimensions[1]:
                local_restriction[1, 1] /= 3
                local_restriction[1, 0] = np.copy(local_restriction[1, 1])
                local_restriction[1, 2] = np.copy(local_restriction[1, 1])

            if ghost_fine_index[2] >= 0 and ghost_fine_index[2] + 2 < fine_face_dimensions[2]:
                local_restriction[1] /= 3
                local_restriction[0] = np.copy(local_restriction[1])
                local_restriction[2] = np.copy(local_restriction[1])

        if d == 2:
            if ghost_fine_index[1] >= 0 and ghost_fine_index[1] + 2 < fine_face_dimensions[1]:
                local_restriction[1] /= 3
                local_restriction[0] = np.copy(local_restriction[1])
                local_restriction[2] = np.copy(local_restriction[1])

        if d == 3:
            for j in range(3):
                for i in range(3):
                    index = linearise_index_reverse(ghost_fine_index + np.array([0, j, i]), fine_face_dimensions)
                    if index >= 0 and index < np.prod(fine_face_dimensions) - 2:
                        matrix[row, index : index + 3] = local_restriction[i, j]

        if d == 2:
            for i in range(3):
                index = linearise_index_reverse(ghost_fine_index + np.array([0, i]), fine_face_dimensions)
                if index >= 0 and index < np.prod(fine_face_dimensions) - 2:
                    matrix[row, index : index + 3] = local_restriction[i]

        # Iterate to next fine cell
        coarse_index[0] += 1
        for i in range(d - 1):
            if coarse_index[i] >= coarse_face_dimensions[i]:
                coarse_index[i] = 0
                coarse_index[i + 1] += 1

    sparse_matrix = sp.csr_matrix(matrix)
    return sparse_matrix.data, sparse_matrix.indices, sparse_matrix.indptr


def __compute_interpolation_matrices_1d(value, patch_size):
    """

    compute the interpolation matrix, which used to prepare halo layer data for fine cells at the refinement boundary.
    the matrix formulation depends on the interpolation scheme.

    The matrix is only 1d mapping, say, the matrix is P_ij (it is a 3 patch_size \times patch_size matrix)
    and u^f_i and u^c_j are solution at fine and coarse level, respectively. we have 

    u^f_i = P_ij u^c_j

    The interpolation routine for high dimensions can be found in a tensor-manner

    u^f_ij = P_ik P_jl u^c_kl

    The interpolation along the normal direction will use a slight difference matrix as we can always interpolate rather than extrapolate.
    In fd4 scheme we require the interpolations for three halo layers, so the matrix P'_ij have a size of 3*6, 6 is for 6 layers of coarse volume.

    Please note this method is limited due to no access to the diagonal volumes and the high order interpolation is achieved by tensor product way. 
    We use TP to indicate this in our interpolation scheme name.

    TP_constant: constant interpolation in both surface direction and normal direction

    Tangential:                       Normal:
    1  0  0  0  0  0 ...              0  0  1  0  0  0
    1  0  0  0  0  0 ...              0  0  1  0  0  0
    1  0  0  0  0  0 ...              0  0  1  0  0  0
    0  1  0  0  0  0 ...
    0  1  0  0  0  0 ...
    0  1  0  0  0  0 ...
    ...    

    TP_linear_with_linear_extrap_normal_interp:
        linear interpolation within the patch and along the normal direction, and extrapolate at the boundary of the surface

    Tangential:                       Normal:
    4/3  -1/3  0    0  0  0 ...       0  1/3  2/3  0    0  0  
    1    0     0    0  0  0 ...       0  0    1    0    0  0 
    2/3  1/3   0    0  0  0 ...       0  0    2/3  1/3  0  0 
    1/3  2/3   0    0  0  0 ...
    0    1     0    0  0  0 ...
    0    2/3   1/3  0  0  0 ...
    ...     

    """
    #patch_size = 3
    
    interp_matrix_T = [ [0.0 for _ in range(0,patch_size)] for _ in range(0,3*patch_size) ]
    interp_matrix_N = [ [0.0 for _ in range(0,6)] for _ in range(0,3) ]
    if value=="TP_constant":
      for col in range(0,patch_size):
        for index in range(0,3):
          interp_matrix_T[col*3+index][col]=1.0
      interp_matrix_N[0][2]=1.0
      interp_matrix_N[1][2]=1.0
      interp_matrix_N[2][2]=1.0
    elif value=="TP_linear_with_linear_extrap_normal_interp":
      interp_matrix_T[0][0]=4.0/3.0; interp_matrix_T[0][1]=-1.0/3.0
      interp_matrix_T[1][0]=1.0
      interp_matrix_T[2][0]=2.0/3.0; interp_matrix_T[2][1]= 1.0/3.0
      interp_matrix_T[-1][-1]=4.0/3.0; interp_matrix_T[-1][-2]=-1.0/3.0
      interp_matrix_T[-2][-1]=1.0
      interp_matrix_T[-3][-1]=2.0/3.0; interp_matrix_T[-3][-2]= 1.0/3.0
      for index in range(0,patch_size-2):
        interp_matrix_T[(1+index)*3  ][index  ]=1.0/3.0; interp_matrix_T[(1+index)*3  ][index+1]=2.0/3.0;
        interp_matrix_T[(1+index)*3+1][index+1]=1.0;       
        interp_matrix_T[(1+index)*3+2][index+1]=2.0/3.0; interp_matrix_T[(1+index)*3+2][index+2]=1.0/3.0; 
      interp_matrix_N[0][1]=1.0/3.0;  interp_matrix_N[0][2]=2.0/3.0
      interp_matrix_N[1][2]=1.0
      interp_matrix_N[2][2]=2.0/3.0;  interp_matrix_N[2][3]=1.0/3.0
    else:
      assert False, "value {} not supported".format(value)
    
    return interp_matrix_T, interp_matrix_N


def __compute_restriction_matrices_1d(value, patch_size):
    """

    compute the restriction matrix, which used to prepare halo layer data for coarse cells at the refinement boundary.
    the matrix formulation depends on the interpolation scheme.

    The matrix is only 1d mapping, say, the matrix is Q_ij (it is a patch_size \times 3 patch_size matrix)and u^f_i and u^c_j
    are solution at fine and coarse level, respectively. we have 

    u^c_i = Q_ij u^f_j

    The interpolation routine for high dimensions can be found in a tensor-manner

    u^c_ij = Q_ik Q_jl u^f_kl

    The interpolation along the normal direction will use a slight difference matrix as we need some extrapolation in linear scheme.
    In fd4 scheme we require the interpolations for three halo layers, so the matrix Q'_ij have a size of 6*3, 6 is for 6 layers of fine volume.

    Please note this method is limited due to no access to the diagonal volumes and the high order restriction is achieved by tensor product way. 
    We use TP to indicate this in our restriction scheme name.

    TP_inject_normal_extrap: directly use the central-surface fine cell to assign the coarse cell, but extrapolate along normal direction.
                              Notice we can still do the average for the outmost layer to improve accuracy. 

    Tangential:                                                              Normal:
    0  1  0  0  0  0  0  0  0  0  0  0...                                    0  0  0   1/3  1/3  1/3   
    0  0  0  0  1  0  0  0  0  0  0  0...                                    0  0  0     0    -2   3
    0  0  0  0  0  0  0  0  0  0  0  0...                                    0  0  0     0    -5   6
    0  0  0  0  0  0  0  1  0  0  0  0...
    ...    

    TP_average_normal_extrp: average value in fine cells if possible, but extrapolate along normal direction.

    Tangential:                                                              Normal:
    1/3  1/3  1/3  0    0    0    0    0    0    0    0    0...              0  0  0   1/3  1/3  1/3
    0    0    0    1/3  1/3  1/3  0    0    0    0    0    0...              0  0  0     0    -2   3
    0    0    0    0    0    0    1/3  1/3  1/3  0    0    0...              0  0  0     0    -5   6  
    0    0    0    0    0    0    0    0    0    1/3  1/3  1/3...
    ...    

    """
    #patch_size = 3
    
    restrict_matrix_T = [ [0.0 for _ in range(0,3*patch_size)] for _ in range(0,patch_size) ]
    restrict_matrix_N = [ [0.0 for _ in range(0,6)] for _ in range(0,3) ]
    if value=="TP_inject_normal_extrap":
      for index in range(0,patch_size):
        restrict_matrix_T[index][3*index+1]=1.0
      restrict_matrix_N[2][5]=6.0;  restrict_matrix_N[2][4]=-5.0;
      restrict_matrix_N[1][5]=3.0;  restrict_matrix_N[1][4]=-2.0;
      restrict_matrix_N[0][3]=1.0/3.0;  restrict_matrix_N[0][4]=1.0/3.0; restrict_matrix_N[0][5]=1.0/3.0;
    elif value=="TP_average_normal_extrap":
      for index in range(0,patch_size):
        restrict_matrix_T[index][3*index  ]=1.0/3.0  
        restrict_matrix_T[index][3*index+1]=1.0/3.0 
        restrict_matrix_T[index][3*index+2]=1.0/3.0 
      restrict_matrix_N[2][5]=6.0;  restrict_matrix_N[2][4]=-5.0;
      restrict_matrix_N[1][5]=3.0;  restrict_matrix_N[1][4]=-2.0;
      restrict_matrix_N[0][3]=1.0/3.0;  restrict_matrix_N[0][4]=1.0/3.0; restrict_matrix_N[0][5]=1.0/3.0;
    else:
      assert False, "value {} not supported".format(value)
      
    return restrict_matrix_T, restrict_matrix_N


def switch_to_FD4_second_order_interpolation( solver: CellCenteredFiniteDifferences ):
  solver.interpolation = "second_order<" + solver.name + ">"

def switch_to_FD4_second_order_restriction( solver: CellCenteredFiniteDifferences ):
  solver.restriction = "second_order<" + solver.name + ">"


def switch_to_FD4_matrix_interpolation( solver: CellCenteredFiniteDifferences ):
  max_row_indices_size = solver._patch_size * solver._patch_size * solver._overlap + 1
  largest_data, _, _ = __compute_interpolation_matrix_nd_csr(solver._patch_size, solver._overlap, 4)
  max_data_size = len(largest_data)

  data_array = np.zeros((9, max_data_size))
  column_indices_array = np.zeros((9, max_data_size), dtype=np.int32)
  row_indices_array = np.zeros((9, max_row_indices_size), dtype=np.int32)

  for patch_index_linear in range(9):
    data, column_indices, row_indices = __compute_interpolation_matrix_nd_csr(solver.patch_size, solver._overlap, patch_index_linear)
    data_array[patch_index_linear][0:len(data)] = data
    column_indices_array[patch_index_linear][0:len(column_indices)] = column_indices
    row_indices_array[patch_index_linear][0:len(row_indices)] = row_indices

  solver.add_solver_constants("""static constexpr int InterpolationDataOffset = """ + str(max_data_size) + """;\n""")
  solver.add_solver_constants("""static constexpr int InterpolationRowIndicesOffset = """ + str(max_row_indices_size) + """;\n""")
  solver.add_solver_constants("""static constexpr double InterpolationData[] = {};\n\n""".format( render_tensor_2( tensor=data_array, use_multidimensional_arrays=False )))
  solver.add_solver_constants("""static constexpr int InterpolationColumnIndices[] = {};\n\n""".format( render_tensor_2( tensor=column_indices_array, use_multidimensional_arrays=False )))
  solver.add_solver_constants("""static constexpr int InterpolationRowIndices[] = {};\n\n""".format( render_tensor_2( tensor=row_indices_array, use_multidimensional_arrays=False )))

  solver.interpolation = "matrix<" + solver.name + ">"


def switch_to_FD4_matrix_restriction( solver: CellCenteredFiniteDifferences ):
  data_offset = 0;
  row_indices_offset = 0;
  for i in range(9):
    data, _, row_indices = __compute_restriction_matrix_nd_csr(solver._patch_size, solver._overlap, i)
    data_offset = np.maximum(len(data), data_offset)
    row_indices_offset = np.maximum(len(row_indices), row_indices_offset)

  data_array = np.zeros((9, data_offset))
  column_indices_array = np.zeros((9, data_offset), dtype=np.int32)
  row_indices_array = np.zeros((9, row_indices_offset), dtype=np.int32)

  for patch_index_linear in range(9):
    data, column_indices, row_indices = __compute_restriction_matrix_nd_csr(solver.patch_size, solver._overlap, patch_index_linear)
    data_array[patch_index_linear][0:len(data)] = data
    column_indices_array[patch_index_linear][0:len(column_indices)] = column_indices
    row_indices_array[patch_index_linear][0:len(row_indices)] = row_indices

  solver.add_solver_constants("""static constexpr int RestrictionDataOffset = """ + str(data_offset) + """;\n""")
  solver.add_solver_constants("""static constexpr int RestrictionRowIndicesOffset = """ + str(row_indices_offset) + """;\n""")
  solver.add_solver_constants("""static constexpr double RestrictionData[] = {};\n\n""".format( render_tensor_2( tensor=data_array, use_multidimensional_arrays=False )))
  solver.add_solver_constants("""static constexpr int RestrictionColumnIndices[] = {};\n\n""".format( render_tensor_2( tensor=column_indices_array, use_multidimensional_arrays=False )))
  solver.add_solver_constants("""static constexpr int RestrictionRowIndices[] = {};\n\n""".format( render_tensor_2( tensor=row_indices_array, use_multidimensional_arrays=False )))

  solver.restriction = "matrix<" + solver.name + ">"


def switch_to_FD4_tensor_product_interpolation( solver: CellCenteredFiniteDifferences,
                                                variant ):
  """
  
  This routine accepts the solver object and an identifier object and then
  enables this interpolation. To do this, we have to set the tensor product
  interpolation, and we have to export the right matrices into the solver.
  Please do not switch the interpolation scheme more than once.
  
  """
  interp_matrix_T, interp_matrix_N=__compute_interpolation_matrices_1d(variant, solver._patch_size)
  
  solver.add_solver_constants("""static constexpr double  TangentialInterpolationMatrix1d[] = {};\n\n""".format( render_tensor_2( tensor=interp_matrix_T, use_multidimensional_arrays=False ) ) )
  solver.add_solver_constants("""static constexpr double  NormalInterpolationMatrix1d[] = {};\n\n""".format(     render_tensor_2( tensor=interp_matrix_N, use_multidimensional_arrays=False ) ) )
  
  solver.interpolation = "tensor_product<" + solver.name + ">"


def switch_to_FD4_tensor_product_restriction( solver: CellCenteredFiniteDifferences,
                                              variant):
  """
  
  Consult the docu of switch_to_FD4_tensor_product_interpolation().
  
  """
  restrict_matrix_T, restrict_matrix_N=__compute_restriction_matrices_1d(variant, solver._patch_size)
    
  solver.add_solver_constants("""static constexpr double TangentialRestrictionMatrix1d[] = {};\n\n""".format( render_tensor_2( tensor=restrict_matrix_T, use_multidimensional_arrays=False ) ) )
  solver.add_solver_constants("""static constexpr double NormalRestrictionMatrix1d[] = {};\n\n""".format(     render_tensor_2( tensor=restrict_matrix_N, use_multidimensional_arrays=False ) ) )
  
  solver.restriction = "tensor_product<" + solver.name + ">"


  
