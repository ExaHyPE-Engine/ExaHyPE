# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.SingleSweep import SingleSweep
from exahype2.solvers.fv.PDETerms    import PDETerms

import jinja2
import peano4

from .kernels import create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping
from .kernels import create_abstract_solver_user_declarations_for_fixed_time_stepping
from .kernels import create_abstract_solver_user_definitions_for_fixed_time_stepping
from .kernels import create_start_time_step_implementation_for_fixed_time_stepping
from .kernels import create_finish_time_step_implementation_for_fixed_time_stepping
from .kernels import create_empty_source_term_kernel
from .kernels import create_user_defined_source_term_kernel
from .kernels import create_compute_Riemann_kernel_for_ClawPack


class GlobalFixedTimeStep( SingleSweep ):
  """
  
  This is the simplest ClawPack-based solver one could think about. The
  minimal requirement to implement a SingleSweep (or enclave) solver
  is to inherit from the base class and set the Riemann solver to be 
  used and the right-hand side (source term).
  
  Furthermore, we have to say how to determine the admissible time 
  step size, and we have to augment the dictionary with entries that 
  are ClawPack-specific.
  

  ==========
  Attributes
  ==========
  
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
    
  clawpack_files: [String]
    List of Fortran files. Will be forwarded to   
    add_implementation_files_to_project(). You can always leave this 
    one empty and add files later on.
    

  ================
  Invocation order
  ================
  
  We call the superclass constructor which will establish all the finite volume
  data structures et al. See the documentation of the superclass or FV.
  
  The FV setup will automatically invoke add_entries_to_text_replacement_dictionary()
  at multiple points to translate some templates into meaningful (default) 
  C++ code. Obviously, you can always alter setups later on by altering
  the object state. In this case, you will have to regenerate all data
  structures. 
  
  Our ClawPack-specific dictionary entries have to know what the name of 
  the chosen ClawPack routine is and whether or not we use 
  
  """
  def __init__(self,
    name, patch_size, unknowns, auxiliary_variables, min_volume_h,
    max_volume_h, time_step_size, clawpack_name, clawpack_files = [],
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False, discriminate_normal=False,
    kernel_implemenatation=None
  ):
    self._clawpack_Riemann_solver             = clawpack_name
    self._discriminate_normal                 = discriminate_normal

    super(GlobalFixedTimeStep,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties)
   
    self._time_step_size = time_step_size

    # Defaults
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False

    self._preprocess_reconstructed_patch      = create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping( time_step_size )
    self._postprocess_updated_patch           = ""

    self._Riemann_solver_implementation_files = clawpack_files

    self._source_term_implementation          = PDETerms.None_Implementation

    self.set_implementation(
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      initial_conditions=initial_conditions, 
      source_term=source_term )


  def add_entries_to_text_replacement_dictionary(self,d):
    """
    
     Let the superclass befill the dictionary with stuff that then has
     to enter all the templates. There's a few additional entries that
     we add, which are ClawPack-specific. Those are the includes, the 
     Fortran wrapper definitions, some flags to tailor the Riemann 
     calls et al. 
    
    """  
    super(GlobalFixedTimeStep,self).add_entries_to_text_replacement_dictionary(d)

    rpn2 = 'extern "C" int rpn2_(int* ixy, int* num_eqn, int* num_aux, int* num_waves,'
    rpn2 += 'const double * __restrict__ q_l, const double * __restrict__ q_r,'
    rpn2 += 'const double * __restrict__ aux_l, const double * __restrict__ aux_r,'
    rpn2 += 'double wave[3][3], double* s, double* amdq, double* apdq);\n'
    d["ADDITIONALDEFS"]     = rpn2


  def add_implementation_files_to_project(self,namespace,output):
    """

      We just tell the superclass too add its implementation files, 
      and then we add our own Fortran files as handed over to the 
      constructor.
          
    """
    super(GlobalFixedTimeStep,self).add_implementation_files_to_project(namespace,output)
    for f in self._Riemann_solver_implementation_files:
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


  def set_implementation(self,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False,
    additional_includes     = ""
  ):
    """

    """
    
    if source_term          is not None:  self._source_term_implementation                = source_term

    self._abstract_solver_user_declarations = create_abstract_solver_user_declarations_for_fixed_time_stepping()
    self._abstract_solver_user_definitions  = create_abstract_solver_user_definitions_for_fixed_time_stepping()

    self._start_time_step_implementation     = create_start_time_step_implementation_for_fixed_time_stepping(False)
    self._finish_time_step_implementation    = create_finish_time_step_implementation_for_fixed_time_stepping(self._time_step_size)

    if self._source_term_implementation == PDETerms.None_Implementation:
      self._source_term_call    = create_empty_source_term_kernel()
    else:
      self._source_term_call    = create_user_defined_source_term_kernel()
    self._Riemann_solver_call   = create_compute_Riemann_kernel_for_ClawPack(self._clawpack_Riemann_solver, self._discriminate_normal)
      
    super(GlobalFixedTimeStep,self).set_implementation(boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop, additional_includes)
