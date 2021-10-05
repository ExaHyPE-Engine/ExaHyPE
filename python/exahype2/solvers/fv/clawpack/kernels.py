# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
def create_compute_Riemann_kernel_for_ClawPack(clawpack_Riemann_solver, discriminate_normal):
  """
  
  clawpack_Riemann_solver: String
    Name of the ClawPack solver that is to be used.
    
  discriminate_normal: Boolean
    There are two different versions of the kernels. One discriminates the directions,
    the other one not.
      
  """
  preamble = """
          double wave[{{NUMBER_OF_UNKNOWNS}}][{{NUMBER_OF_UNKNOWNS}}];
          double speed[{{NUMBER_OF_UNKNOWNS}}];

          int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
          int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
          int num_waves = {{NUMBER_OF_UNKNOWNS}};
        """
        
  arguments = ""
  if discriminate_normal:
    arguments = """(
          &normal,
          &num_eqn,
          &num_aux,
          &num_waves,
          QL,                                 // double* q_l
          QR,                                 // double* q_r
          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
          wave,
          speed,
          FL,                                 // double* amdq
          FR                                  // double* apdq
        );
    """
  else:
    arguments = """(
          &num_eqn,
          &num_aux,
          &num_waves,
          QL,                                 // double* q_l
          QR,                                 // double* q_r
          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
          wave,
          speed,
          FL,                                 // double* amdq
          FR                                  // double* apdq
        );
    """

  epilogue = """
          for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
            FL[i] = -FL[i];
          }
"""
  return preamble + clawpack_Riemann_solver + "_" + arguments + epilogue


