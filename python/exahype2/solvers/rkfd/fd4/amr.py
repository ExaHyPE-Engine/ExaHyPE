# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.rkfd.CellCenteredFiniteDifferences import CellCenteredFiniteDifferences


#
# We use the render operations here, too
#
from exahype2.solvers.LagrangeBasis import render_tensor_2

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


  