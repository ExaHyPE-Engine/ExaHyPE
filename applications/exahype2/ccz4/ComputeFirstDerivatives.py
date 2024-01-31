#
# New action set for second order CCZ4 formualtion
#
import peano4
import jinja2

from exahype2.solvers.ButcherTableau import ButcherTableau
from exahype2.solvers.rkfd.OneSweepPerRungeKuttaStep import ReconstructLinearCombination


class ComputeFirstDerivativesFV(peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor):
    """!
    
    Explicit reconstruction of derivatives for Finite Volumes
    
    """
    def __init__(self,
                 solver
                 ):
        super( ComputeFirstDerivativesFD4, self ).__init__(patch=solver._patch,
                                                        patch_overlap=solver._patch_overlap_new,
                                                        functor_implementation="""
::exahype2::CellData  patchData( 
  oldQWithHalo, 
  marker.x(), marker.h(), 
  0.0, // timeStamp, 
  0.0, // timeStepSize, 
  newQ 
);
::exahype2::fd::fd4::reconstruct_first_derivatives(patchData,"""+str(solver._patch.dim[0])+","+str(int(solver._patch_overlap_new.dim[0]/2))+","+str(solver._patch.no_of_unknowns)+""",0);
""",
                                                        reconstructed_array_memory_location=solver._reconstructed_array_memory_location,
                                                        guard="not marker.willBeRefined() and not marker.hasBeenRefined()",
                                                        add_assertions_to_halo_exchange=False,
                                                        )
        pass

    
    def user_should_modify_template(self):
        return False
    

    def get_includes(self):
        return super( ComputeFirstDerivativesFV, self ).get_includes() + """
    #include "exahype2/CellData.h"
    #include "tarch/NonCriticalAssertions.h"
    """


    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

        

class ComputeFirstDerivativesFD4RK(peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor):
    """!
    
    Explicit reconstruction of first derivatives for FD4 discretisation
    
    FD4 is implemented as Runge-Kutta scheme. So we first have to recompute
    the current solution guess according to the Butcher tableau. Then we feed
    this reconstruction into the derivative computation. In theory, this is all
    simple, but in practice things require more work.
    
    We assume that developers follow @ref page_exahype_coupling "ExaHyPE's generic recommendation how to add additional traversals".
    Yet, this is only half of the story. If they follow the vanilla blueprint
    and plug into a step after the last time step
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~
      if (repositories::isLastGridSweepOfTimeStep()
      and
      repositories::StepRepository::toStepEnum( peano4::parallel::Node::getInstance().getCurrentProgramStep() ) != repositories::StepRepository::Steps::AdditionalMeshTraversal
      ) {
        peano4::parallel::Node::getInstance().setNextProgramStep(
        repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::AdditionalMeshTraversal )
      );
      continueToSolve         = true;
    }
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    then the reconstruction is only done after the last and final Runge-Kutta 
    step. Alternatively, users might plug into the solver after each 
    Runge-Kutta step. In principle, we might say "so what", as all the 
    right-hand sides after the final step are there, so we can reconstruct
    the final solution. However, there are two pitfalls:
    
    1. A solver might not hold the rhs estimates persistently in-between two 
       time steps.
    2. The solver's state always is toggled in startTimeStep() on the solver
       instance's class. Therefore, an additional grid sweep after the last
       Runge-Kutta sweep cannot be distinguished from an additional sweep
       after the whole time step. They are the same.
    
    Consequently, we need special treatment for the very last Runge-Kutta step:
    
    - If we have an additional grid sweep after an intermediate RK step, we use
      the right-hand side estimate of @f$ \partial _t Q = F(Q) @f$ to store the 
      outcome. Furthermore, we scale the derivative with 1/dt, as the linear 
      combination of RK will later on multiple this estimate with dt again.
    - If we are in the very last grid sweep, we work with the current estimate.
            
    The actual derviative calculation is "outsourced" to the routine
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      ::exahype2::CellData  patchData( 
        oldQWithHalo, 
        marker.x(), marker.h(), 
        0.0, // timeStamp => not used here 
        0.0, // timeStepSize => not used here
        newQ 
      );
      ::exahype2::fd::fd4::reconstruct_first_derivatives(
        patchData,
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},  // int                     numberOfGridCellsPerPatchPerAxis,
        {{HALO_SIZE}}, 
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}}
      );
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    Wrapping up oldQWithHalo and newQ in a CellData object is a slight 
    overkill (we could have done with only passing the two pointers, as we 
    don't use the time step size or similar anyway), but we follow the 
    general ExaHyPE pattern here - and in theory could rely on someone else
    later to deploy this to GPUs, e.g. 
    
    Right after the reconstruction, we have to project the outcome back again 
    onto the faces. Here, we again have to take into account that we work 
    with a Runge-Kutta scheme. The cell hosts N blocks of cell data for the
    N shots of Runge-Kutta. The reconstruction writes into the nth block 
    according to the current step. Consequently, the projection also has to
    work with the nth block. exahype2.solvers.rkfd.actionsets.ProjectPatchOntoFaces
    provides all the logic for this, so we assume that users add this one to 
    their script. As the present action set plugs into touchCellFirstTime()
    and the projection onto the faces plugs into touchCellLastTime(), the 
    order is always correct no matter how you prioritise between the different
    action sets.
    
    Further to the projection, we also have to roll over details. Again,
    exahype2.solvers.rkfd.actionsets.ProjectPatchOnto has more documentation 
    on this.
    
    """
    def __init__(self,
                 solver,
                 is_enclave_solver
                 ):
        super( ComputeFirstDerivativesFD4RK, self ).__init__(patch=solver._patch,
                                                        patch_overlap=solver._patch_overlap_new,
                                                        functor_implementation="""
#error Will be inserted later down in this Python file
""",
                                                        reconstructed_array_memory_location=solver._reconstructed_array_memory_location,
                                                        guard="not marker.willBeRefined() and not marker.hasBeenRefined()",
                                                        add_assertions_to_halo_exchange=False,
                                                        )
        self._solver = solver
        self._butcher_tableau = ButcherTableau(self._solver._rk_order)
        self._use_enclave_solver = is_enclave_solver
        pass


    ComputeDerivativesOverCell = jinja2.Template(  """
      double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      // Set the variable
      // double timeStepSize
      {{COMPUTE_TIME_STEP_SIZE}}

      {% for PREDICATE_NO in range(0,PREDICATES|length) %}
      if ({{PREDICATES[PREDICATE_NO]}}) {
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariablesOnReconstructedPatch( 1, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, {{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
        dfor( dof, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
          for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
            {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO]|length) %}
            {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]!=0 %}
            oldQWithHalo[ enumeratorWithAuxiliaryVariablesOnReconstructedPatch(0,dof,unknown) ] += 
              {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize * {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}} * 
              fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{PREDICATE_NO-1}},dof,unknown) ];
            {% endif %}
            {% endfor %}
          }      
        }
        
        newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + enumeratorWithoutAuxiliaryVariables({{PREDICATE_NO}},0,0);
      }
      {% endfor %} 

      assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
      assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );

      ::exahype2::fd::validatePatch(
        oldQWithHalo,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{HALO_SIZE}}, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
      ); // previous time step has to be valid
    """ + """
      // take into account that linear combination has already been computed,
      // and rhs even might not be held persistently
      if ( repositories::isLastGridSweepOfTimeStep() ) {
        newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
      }
            
      ::exahype2::CellData  patchData( 
        oldQWithHalo, 
        marker.x(), marker.h(), 
        0.0, // timeStamp => not used here 
        0.0, // timeStepSize => not used here
        newQ 
      );
      ::exahype2::fd::fd4::reconstruct_first_derivatives(
        patchData,
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},  // int                     numberOfGridCellsPerPatchPerAxis,
        {{HALO_SIZE}}, 
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}}
      );
      
      if ( not repositories::isLastGridSweepOfTimeStep() ) {
        ::exahype2::enumerator::AoSLexicographicEnumerator enumerator( 1, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
        dfor( dof, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
          for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
            newQ[ enumerator(0,dof,unknown) ] *= 1.0 / timeStepSize;
          }
        }
      }
      """)
    
    
#    """+str(solver._patch.dim[0])+","+str(int(solver._patch_overlap_new.dim[0]/2))+","+str(solver._patch.no_of_unknowns)+"""
 
    
    def user_should_modify_template(self):
        return False
    

    def get_includes(self):
        return super( ComputeFirstDerivativesFD4RK, self ).get_includes() + """
    #include "exahype2/CellData.h"
    #include "exahype2/fd/fd4/FD4.h" 
    #include "tarch/NonCriticalAssertions.h"
    """
        
    def _add_action_set_entries_to_dictionary(self, d):
        """!

        This is our plug-in point to alter the underlying dictionary

        """
        super(ComputeFirstDerivativesFD4RK, self)._add_action_set_entries_to_dictionary(d)

        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        d["BUTCHER_TABLEAU_WEIGHTS"] = self._butcher_tableau.weight_matrix()
        d["BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES"] = self._butcher_tableau.time_step_sizes()

        if self._use_enclave_solver:
            d["PREDICATES"] = self._solver._primary_sweeps_of_Runge_Kutta_step_on_cell
        else:       
            d["PREDICATES"] = [
                "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSubStep{}".format(
                    self._solver._store_cell_data_default_guard(),
                    self._solver.get_name_of_global_instance(),
                    self._solver._name,
                    step,
                )
                for step in range(0, self._solver.number_of_Runge_Kutta_steps())
            ]

        # Has to come after we've set the predicates, as we use these
        # fields in here already
        d["CELL_FUNCTOR_IMPLEMENTATION"] = self.ComputeDerivativesOverCell.render(
            **d
        )


    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

