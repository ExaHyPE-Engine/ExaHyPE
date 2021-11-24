# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep

from .PDETerms import PDETerms

from typing import TypeVar, Generic, List

T = TypeVar('T')


class UserDefinedRiemann( Generic[T] ):
  def __init__(self, 
    name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=None, 
    eigenvalues=PDETerms.User_Defined_Implementation, 
    plot_grid_properties=False, 
    stateless_pde_terms=False
  ):
    super(T,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, plot_grid_properties) 
    self._riemann_solver_implementation       = PDETerms.None_Implementation
    self.set_implementation(riemann_solver=riemann_solver, stateless_pde_terms=self._stateless_pde_terms)




  def set_implementation(self,
    riemann_solver=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False,
    stateless_pde_terms =False
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
    if stateless_pde_terms             :  self._stateless_pde_terms                       = stateless_pde_terms
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


    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    /**
     * 1d Riemann solve
     */
    virtual void solveRiemannProblem(
      const double * __restrict__ QL,
      const double * __restrict__ QR,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ FL,
      double * __restrict__ FR
    ) {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}
