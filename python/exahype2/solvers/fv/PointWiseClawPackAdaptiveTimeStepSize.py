# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor



class UpdateCell(ReconstructPatchAndApplyFunctor):
  RiemannCallOverPatch = """
    {{PREPROCESS_RECONSTRUCTED_PATCH}}
    
    ::exahype2::fv::copyPatch(
      reconstructedPatch,
      originalPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1 // halo size
    );

    {% if USE_SPLIT_LOOP %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
    #endif
    {% else %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
    {% endif %}
      [&](
        const double * __restrict__                  QL,
        const double * __restrict__                  QR,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        double wave[{{NUMBER_OF_UNKNOWNS}}][{{NUMBER_OF_UNKNOWNS}}]; 
        double speed[{{NUMBER_OF_UNKNOWNS}}]; 

        int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
        int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
        int num_waves = {{NUMBER_OF_UNKNOWNS}}; 

        {{CLAWPACK_RIEMANN_SOLVER}}_(
          {%if DISCRIMINATE_NORMAL %}
            &normal,
          {% endif %}   
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

        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
          FL[i] = -FL[i];
        }
      },
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        double * __restrict__                        S
      ) -> void {
        repositories::{{SOLVER_INSTANCE}}.sourceTerm(
          Q,
          x, dx, t, dt, 
          S
        );
      },
      marker.x(),
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      originalPatch
    );

    
    double maxEigvenvalue = ::exahype2::fv::maxEigenvalue_AoS(
      [] (
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> double {
      
      //return max Eigenvalue
      //approximation: s = u +- sqrt(g * h)
      return std::max(std::abs(Q[1+normal]) + std::sqrt(9.81*Q[0]), std::abs(Q[1+normal]) - std::sqrt(9.81*Q[0]));
      },
      marker.x(),
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      originalPatch
    );
    
    repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigvenvalue );
    
    {{POSTPROCESS_UPDATED_PATCH}}
  """ 


  def __init__(self,solver):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)   
    d[ "USE_SPLIT_LOOP" ] = solver._use_split_loop

    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.RiemannCallOverPatch ).render(**d),
      solver._reconstructed_array_memory_location,
      "not marker.isRefined()"
    )
    
    self._solver = solver
  
  
  def get_includes(self):
    return """
#include "exahype2/fv/BoundaryConditions.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 



#! =====================================================
#subroutine rp1_ptwise(num_eqn, num_aux, num_waves, q_l, q_r, aux_l, aux_r,    &
#                       wave, s, amdq, apdq)
#! =====================================================###

#!     # Pointwise Riemann solver for the acoustics equations in 1d, with
#!     #  variable coefficients (heterogeneous media)

#! waves:     2
#! equations: 2
#! aux fields: 2


class PointWiseClawPackAdaptiveTimeStepSize(  FV ):
  """
  
    Very simple solver that can use ClawPack's point-wise Riemann solver.
  

   The actual call of the Riemann solver has to be a simple function call
   which has the following variables available. The FL and FR is the only
   out array. All the other data are in.
   
      double QL[],
      double QR[],
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double                                       FL[],
      double                                       FR[]
      
   My generic kernel requires FR and FL along the normal of the respective
   cell you look at F. That is, I need -FR compared to what ClawPack gives 
   me. Quick trial and error with the acoustics equations tells you that
   it indeed has to be -FR and not -FL.
   
   There's also the opportunity to misuse a degenerated 1d Riemann solver, i.e.
   a solver specified over a 1d array as point-wise solver.  
      
  """
  
  

  
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, clawpack_Riemann_solver, Riemann_solver_implementation_files = [], plot_grid_properties=False, kernel_implementation = None, discriminate_normal=False, time_step_relaxation = 0.9):
    """
    
      Instantiate a generic FV scheme with an overlap of 1.
      
      It is important that the superclass constructor is invoked late. It triggers
      the create_action_sets() routine which in return expect that a lot of the 
      solver-specific fields are properly set beforehand.
      
      
      Riemann_solver_implementation_files: list of strings
        These are the Fortran files the code should add to the compile.
        
        
      is_degenerated_1d_solver: Boolean
        I prefer ClawPack solvers which are written point-wisely. Alternatively, I 
        can misuse 1d solvers.  
      
    """        
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._source_term_implementation          = PDETerms.Empty_Implementation

    self.clawpack_Riemann_solver             = clawpack_Riemann_solver 
    self.Riemann_solver_implementation_files = Riemann_solver_implementation_files
    
    self._discriminate_normal = discriminate_normal
    
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False
    
    self._time_step_relaxation = time_step_relaxation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation

    FV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)

    self.set_implementation()


  def create_data_structures(self):
    FV.create_data_structures(self)

    self._patch_overlap.generator.store_persistent_condition   = self._store_face_data_default_predicate()
    self._patch_overlap.generator.load_persistent_condition    = self._load_face_data_default_predicate()

    self._patch_overlap.generator.send_condition               = "true"
    self._patch_overlap.generator.receive_and_merge_condition  = "true"



  def set_implementation(self,
    boundary_conditions=None,eigenvalues=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False
  ):
    """
    
      Pass in None if you don't want to overwrite any default.
      
    """
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions
    if source_term!=None:
      self._source_term_implementation                = source_term
    if eigenvalues!=None:    
      self._eigenvalues_implementation                = eigenvalues
    if memory_location!=None:
      self._reconstructed_array_memory_location = memory_location
    if use_split_loop!=None:
      self._use_split_loop = use_split_loop

      
    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

    self.create_action_sets()


  def create_action_sets(self):
    FV.create_action_sets(self)
    self._action_set_update_cell = UpdateCell(self)     

  
  def set_postprocess_updated_patch_kernel_point_wisely(self,kernel):
    """
  
    This is a routine to implement some a posteriori constraints on 
    the solution point-wisely. So you can make kernel holds a C++
    string similar to
        
if (Q[1]>43.2) {
 Q[2] = Q[4];
}
  
    """
    self.set_postprocess_updated_patch_kernel( """
int postProcessingIndex = 0;
dfor( volume, """ + str(self._patch_size) + """ ) {
  double* Q = originalPatch + postProcessingIndex;
  """ + kernel + """
  postProcessingIndex += """ + str(self._unknowns) + """;
  postProcessingIndex += """ + str(self._auxiliary_variables) + """;
}
""")

  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    d[ "TIME_STAMP" ]                    = "repositories::"+d[ "SOLVER_INSTANCE" ] + ".getMinTimeStamp()"
    d[ "TIME_STEP_SIZE" ]                = "repositories::"+d[ "SOLVER_INSTANCE" ] + ".getMinTimeStepSize()"
    d[ "CLAWPACK_RIEMANN_SOLVER"]        = self.clawpack_Riemann_solver

    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    d[ "SOURCE_TERM_IMPLEMENTATION"]          = self._source_term_implementation
    d[ "DISCRIMINATE_NORMAL"]                 = self._discriminate_normal
    d[ "TIME_STEP_RELAXATION" ]               = self._time_step_relaxation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation

  
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
"""    
  
  
  def add_implementation_files_to_project(self,namespace,output):
    """
    
      Invoke add_implementation_files_to_project() of superclass and
      then add all the Fortran files to the makefile.
      
    """
    FV.add_implementation_files_to_project(self,namespace,output)
    for f in self.Riemann_solver_implementation_files:
      output.makefile.add_Fortran_file(f)
  
    
