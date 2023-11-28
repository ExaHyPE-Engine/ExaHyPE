#
# New action set for second order CCZ4 formualtion
#
import peano4
import jinja2

from exahype2.solvers.ButcherTableau import ButcherTableau


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
    #include "exahype2/NonCriticalAssertions.h"
    """
        

class ComputeFirstDerivativesFD4RK(peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor):
    """!
    
    Explicit reconstruction of first derivatives for FD4 discretisation
    
    FD4 is implemented as Runge-Kutta scheme. So we first have to recompute
    the current solution guess according to the Butcher tableau. Then we feed
    this reconstruction into the derivative computation. The actual first
    reconstruction is missing in the code snippet ComputeDerivativesOverCell
    below. That's because we still use the peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor
    as a baseline and then add those Runge-Kutta guesses to this reconstruction.
    There's no need to do this for the faces. The faces already hold the 
    latest linear combination as per exahype2.solvers.rkfd.actionsets.ProjectPatchOntoFaces.
   
    This makes using this action set very easy. We can assume that the faces 
    hold the latest linear combination from the last Runge-Kutta sweep. So that
    data is consistent. We also can assume that the cell solution holds all 
    Runge-Kutta approximations at this point. So there's no need for ifs or 
    similar.
    
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


    ComputeDerivativesOverCell = jinja2.Template(
        """
      double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      // Set the variable
      //
      // double timeStepSize
      //
      // Not required for derivative calculation but required to compute the 
      // Runge-Kutta linear combination
      {{COMPUTE_TIME_STEP_SIZE}}

      {% for PREDICATE_NO in range(0,PREDICATES|length) %}
      ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariablesOnReconstructedPatch( 1, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, {{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
      ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
      if ({{PREDICATES[PREDICATE_NO]}}) {
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
      }
        
      newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + enumeratorWithoutAuxiliaryVariables({{PREDICATE_NO}},0,0);
      
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
      {% endfor %} 
      """)
    
    
#    """+str(solver._patch.dim[0])+","+str(int(solver._patch_overlap_new.dim[0]/2))+","+str(solver._patch.no_of_unknowns)+"""
 
    
    def user_should_modify_template(self):
        return False
    

    def get_includes(self):
        return super( ComputeFirstDerivativesFD4RK, self ).get_includes() + """
    #include "exahype2/CellData.h"
    #include "exahype2/fd/fd4/FD4.h" 
    #include "exahype2/NonCriticalAssertions.h"
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
