# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.SingleSweep import SingleSweep
from exahype2.solvers.fv.PDETerms    import PDETerms

import jinja2
import peano4

# from .kernels import create_source_term_kernel_for_Rusanov
# from .kernels import create_compute_Riemann_kernel_for_Rusanov
# from .kernels import create_abstract_solver_declarations
# from .kernels import create_abstract_solver_definitions
# from .kernels import create_solver_declarations
# from .kernels import create_solver_definitions
# from .kernels import create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping


class GlobalFixedTimeStep( SingleSweep ):
  """
  
  This is the simplest ClawPack-based solver one could think about. The
  minimal requirement to implement a SingleSweep (or enclave) solver
  is to inherit from the base class and set the Riemann solver to be 
  used and the right-hand side (source term).
  
  Furthermore, we have to say how to determine the admissible time 
  step size, and we have to augment the dictionary with entries that 
  are ClawPack-specific.
  
  There are two particularly important flags: 
  
  clawpack_name: String
    This is the name of the actual ClawPack solver name. We simply
    store it in a local attribute. Once we build up our dictionary
    in add_entries_to_text_replacement_dictionary(), we map this 
    local attribute onto an entry in the dict and then can use it 
    within our templates.
  
  discriminate_normal: Boolean
    This is a flag which switches between two ways how the ClawPack
    solvers are called.
    
  """
  def __init__(self,
    name, patch_size, unknowns, auxiliary_variables, min_volume_h,
    max_volume_h, time_step_size, clawpack_name, clawpack_files,
    plot_grid_properties=False, discriminate_normal=False,
    kernel_implemenatation=None
  ):
    super(ClawpackFixedTimeStep,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties)

    self._clawpack_Riemann_solver             = clawpack_name
    self._Riemann_solver_implementation_files = clawpack_files
    
    self._time_step_size = time_step_size

    # Defaults
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False

    self._preprocess_reconstructed_patch      = create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping( time_step_size )
    self._postprocess_updated_patch           = ""

    self._discriminate_normal = discriminate_normal


  def add_entries_to_text_replacement_dictionary(self,d):
    """
    
     Let the superclass befill the dictionary with stuff that then hsa
     to enter all the templates. There's a few additional entries that
     we add, which are ClawPack-specific. Those are the includes, the 
     Fortran wrapper definitions, some flags to tailor the Riemann 
     calls et al. 
    
    """  
    super(ClawpackFixedTimeStep,self).add_entries_to_text_replacement_dictionary(d)
    d["ADDITIONALINCLUDES"] = '#include "TopologyParser.h"'

    rpn2 = 'extern "C" int rpn2_(int* ixy, int* num_eqn, int* num_aux, int* num_waves,'
    rpn2 += 'const double * __restrict__ q_l, const double * __restrict__ q_r,'
    rpn2 += 'const double * __restrict__ aux_l, const double * __restrict__ aux_r,'
    rpn2 += 'double wave[3][3], double* s, double* amdq, double* apdq);\n'
    d["ADDITIONALDEFS"]     = rpn2

    d["CLAWPACK_RIEMANN_SOLVER"] = self._clawpack_Riemann_solver
    d["DISCRIMINATE_NORMAL"]     = self._Riemann_solver_implementation_files


  def add_implementation_files_to_project(self,namespace,output):
    super(ClawpackFixedTimeStep,self).add_implementation_files_to_project(namespace,output)
    for f in self.Riemann_solver_implementation_files:
      output.makefile.add_Fortran_file(f)


  def set_pointwise_constraint(self, kernel):
    """
    
    This is a routine to implement some a posteriori pointwise constraints on solution.
    So you can make kernel hold a C++
    string similar to
    
          if (Q[1]>43.2) {
            Q[2] = Q[4];
          }    
    
    """
    code  = "int postProcessingIndex = 0;\n"
    code += "dfor(volume, %i) "%self._patch_size;
    code += "{\n  double* Q = targetPatch + postProcessingIndex;\n"
    code += kernel
    code += "  postProcessingIndex += {};\n}}\n".format(self._unknowns + self._auxiliary_variables);
    self.set_postprocess_updated_patch_kernel( code )


