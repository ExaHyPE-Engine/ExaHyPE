# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor



class UpdateCell(ReconstructPatchAndApplyFunctor):
#  RiemannSolverCall_ClawPack = """
#        double wave[{{NUMBER_OF_UNKNOWNS}}][{{NUMBER_OF_UNKNOWNS}}]; 
#        double speed[{{NUMBER_OF_UNKNOWNS}}]; 
#
#        int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
#        int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
#        int num_waves = {{NUMBER_OF_UNKNOWNS}}; 
#
#        {{CLAWPACK_RIEMANN_SOLVER}}_(
#          {%if DISCRIMINATE_NORMAL %}
#            &normal,
#          {% endif %}   
#          &num_eqn,
#          &num_aux,
#          &num_waves, 
#          QL,                                 // double* q_l 
#          QR,                                 // double* q_r
#          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
#          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
#          wave,
#          speed,
#          FL,                                 // double* amdq
#          FR                                  // double* apdq
#        );
#
#        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
#          FL[i] = -FL[i];
#        }
#"""


  SolveRiemannProblemsOverPatch = jinja2.Template( """
    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    double cellTimeStepSize = marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMaxMeshSize() * repositories::{{SOLVER_INSTANCE}}.getMaxTimeStepSize();
    double cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    repositories::{{SOLVER_INSTANCE}}.setTimeStepSize(cellTimeStepSize);
    repositories::{{SOLVER_INSTANCE}}.setTimeStamp(cellTimeStamp + cellTimeStepSize);
 
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(cellTimeStamp + cellTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(cellTimeStepSize);

    ::exahype2::fv::copyPatch(
      reconstructedPatch,
      targetPatch,
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
        {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" and RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>"%}
        repositories::{{SOLVER_INSTANCE}}.solveRiemannProblem(
          QL, QR,
          x, dx, t, dt, 
          normal,
          FL, FR
        );
        {% elif RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
          {{RIEMANN_SOLVER_IMPLEMENTATION}}
        {% else %}
        ::exahype2::fv::splitRusanov1d(
          [] (
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
            {% if FLUX_IMPLEMENTATION=="<none>" %}
            for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
            {% else %}
            repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
            {% endif %}
          },
          [] (
            const double * __restrict__                  Q,
            const double * __restrict__                  deltaQ,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       BgradQ[]
          ) -> void {
            {% if NCP_IMPLEMENTATION!="<none>" %}
            repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
            {% endif %}
          },
          [] (
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
          },
          QL, QR, x, dx, t, dt, normal,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          FL,FR,
          {% if FLUX_IMPLEMENTATION=="<none>" %}
          true,
          {% else %}
          false,
          {% endif %}
          {% if NCP_IMPLEMENTATION=="<none>" %}
          true
          {% else %}
          false
          {% endif %}
        );
        {% endif %}
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
      cellTimeStepSize,
      cellTimeStamp,
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      targetPatch
    );

    {{POSTPROCESS_UPDATED_PATCH}}
  """ )


  def __init__(self, solver):
    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      patch_overlap = solver._patch_overlap,
      functor_implementation = """
#error please switch to your Riemann solver of choice
""",
      reconstructed_array_memory_location = solver._reconstructed_array_memory_location,
      guard = "not marker.isRefined()",
      add_assertions_to_halo_exchange = False
    )

    self._solver = solver
    self.d[ "USE_SPLIT_LOOP" ] = solver._use_split_loop
    self.update_compute_kernel()


  def update_compute_kernel(self):
    """
    
     Whenever you change the operators/types of Riemann solvers
     that you want to use, you have to invoke this routine.
    
    """
    self._solver._init_dictionary_with_default_parameters(self.d)
    self._solver.add_entries_to_text_replacement_dictionary(self.d)
    self.d[ "CELL_FUNCTOR_IMPLEMENTATION" ] = self.SolveRiemannProblemsOverPatch.render(**self.d)


  def enable_Rusanov_solver(self,flux=None,ncp=None,eigenvalues=None):
    self.d[ "RIEMANN_SOLVER_TYPE"]  = "RUSANOV"
    self.d[ "RIEMANN_SOLVER"]  = """
    """
    self.d[ "CELL_FUNCTOR_IMPLEMENTATION" ] = self.SolveRiemannProblemsOverPatch.render(**d)

  
  def get_includes(self):
    return """
#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/NonCriticalAssertions.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes()





class FixedTimeStepSize( FV ):
  """
    Probably the simplest solver you could think off. There's a few
    interesting things to try out with this one nevertheless: You
    can inject symbolic flux/pde term implementations, or you can
    alter the implementation variant of the cell update kernel. The
    latter can be combined with different dynamic memory allocation
    schemes and thus offers quite some opportunities to tweak and
    tune things.
  """


  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, 
    flux=PDETerms.User_Defined_Implementation, eigenvalues=PDETerms.User_Defined_Implementation, ncp=None, riemann_solver=None, plot_grid_properties=False, use_gpu=False):
    """

      Instantiate a generic FV scheme with an overlap of 1.

    """
    self._time_step_size = time_step_size

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._source_term_implementation          = PDETerms.Empty_Implementation
    self._riemann_solver_implementation       = PDETerms.None_Implementation

    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False
    self._use_gpu = use_gpu

    super(FixedTimeStepSize, self).__init__(name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
    self.set_implementation(flux=flux, ncp=ncp, eigenvalues=eigenvalues, riemann_solver=riemann_solver, use_gpu=self._use_gpu)


  def create_data_structures(self):
    FV.create_data_structures(self)

    self._patch_overlap.generator.store_persistent_condition   = self._store_face_data_default_predicate()
    self._patch_overlap.generator.load_persistent_condition    = self._load_face_data_default_predicate()

    self._patch_overlap.generator.send_condition               = "true"
    self._patch_overlap.generator.receive_and_merge_condition  = "true"


  def create_action_sets(self):
    FV.create_action_sets(self)
    self._action_set_update_cell = UpdateCell(self)


  def set_implementation(self,
    flux=None,ncp=None,
    eigenvalues=None,
    riemann_solver=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False,
    use_gpu =False
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.

      Please note that not all options are supported by all solvers. You
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.

      This routine should be the very last invoked by the constructor.
    """
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if boundary_conditions  is not None:  self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:  self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions   is not None:  self._initial_conditions_implementation         = initial_conditions
    if source_term          is not None:  self._source_term_implementation                = source_term
    if memory_location      is not None:  self._reconstructed_array_memory_location       = memory_location
    if use_split_loop                  :  self._use_split_loop                            = use_split_loop
    if use_gpu                         :  self._use_gpu                                   = use_gpu
    if riemann_solver       is not None:  self._riemann_solver_implementation             = riemann_solver

    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )
  
    if self._riemann_solver_implementation!=PDETerms.None_Implementation and self._flux_implementation!=PDETerms.None_Implementation:
      raise Exception( "you cannot specify a Riemann solver and a flux function" )
    if self._riemann_solver_implementation!=PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
      raise Exception( "you cannot specify a Riemann solver and an ncp term" )
    if self._riemann_solver_implementation!=PDETerms.None_Implementation and self._eigenvalues_implementation!=PDETerms.None_Implementation:
      raise Exception( "you cannot specify a Riemann solver and an eigenvalue implementation (as you use a fixed time stepping scheme)" )

    self.create_action_sets()


  def create_action_sets(self):
    """
      Call superclass routine and then reconfigure the update cell call
    """
    FV.create_action_sets(self)
    self._action_set_update_cell = UpdateCell(self)


  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
"""


  def add_entries_to_text_replacement_dictionary(self,d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "RIEMANN_SOLVER_IMPLEMENTATION"]       = self._riemann_solver_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    d[ "SOURCE_TERM_IMPLEMENTATION"]          = self._source_term_implementation
    d[ "USE_GPU"]                             = self._use_gpu

    d[ "TIME_STEP_SIZE" ]                     = self._time_step_size

#  def add_implementation_files_to_project(self,namespace,output):
#    """
#    
#      Invoke add_implementation_files_to_project() of superclass and
#      then add all the Fortran files to the makefile.
#      
#    """
#    FV.add_implementation_files_to_project(self,namespace,output)
#    for f in self.Riemann_solver_implementation_files:
#      output.makefile.add_Fortran_file(f)
#
#
# Discriminate normal fehlt noch
#
