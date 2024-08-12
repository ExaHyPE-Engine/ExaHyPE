# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2

from .AbstractRKFDActionSet import AbstractRKFDActionSet

from peano4.solversteps.ActionSet import ActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau


class ProjectPatchOntoFaces(AbstractRKFDActionSet):
    """!

    Project patch data onto faces, so the faces hold valid data which can feed into the subsequent Runge-Kutta step

    This class is based upon peano4.toolbox.blockstructured.ProjectPatchOntoFaces.
    Yet, it differs fundamentally in the way the inner-most loop is realised:
    The standard projection from the toolbox takes a solution from a patch
    and copies parts of this solution onto the faces. This is not what we want
    to have here: we have the solution plus a number of estimates, and we want
    to write exactly the type of Runge-Kutta linear combination onto the face
    that we need to set the proper halo data on the adjacent cells in the next
    grid sweep. Therefore, this class expects the Butcher tableau and first 
    computes the Runge-Kutta linear combination. The outcome of this is then 
    projected onto the faces.

    The routine is always called as one of the last things, i.e. we have a
    valid estimate in. To some degree, we anticipate what will happening next
    in the Runge-Kutta scheme.
    
    Once more: Each cell hosts the p predictions created by Runge-Kutta of the 
    pth order. These are stored in the field rhs estimates. The projection 
    combines these predictions using the Butcher tableau and projects the 
    linear combination onto the face. The action set never ever uses what's 
    stored in the value array. It always computes this linear combination.
    
    ## Auxiliary parameters

    Auxiliary parameters are not subject to the PDE in ExaHyPE's jargon. They
    typically carry material parameters or similar. We project them onto the
    faces, but here we don't do linear combinations due to the Butcher 
    tableau, as the rhs estimates do not hold any auxiliary parameters at all.
    
    ## Last Runge-Kutta step

    The last Runge-Kutta step is slightly different. After the last
    Runge-Kutta estimate, the solver combines all the estimates into a new
    solution. Consult the action set ComputeFinalLinearCombination for details.
    After this has happened, we could take the plain solution stored within the
    cell and write this guy onto the faces. 
    
    Instead, we ignore the face that the action set ComputeFinalLinearCombination
    would do all of this. Instead, we compute the final linear combination 
    from the rhs guesses (redundantly) and project the outcome onto the faces.
    
    ## Invocation order
    
    To be able to recompute the linear combination of rhs guesses from the
    Runge-Kutta scheme, this projection has to be invoked before we determine
    the final linear combination per cell and overwrite the solution in the
    patch. This means its descend_invocation_order has to be higher than 
    the one from the linear combination computation, as touchCellLastTime()
    inverts this order. 
    
    ## Write data
    
    The data is written onto the faces' QUpdate data field. Faces in the 
    Runge-Kutta scheme hold three different Q representations:
    
    1. The old solution from the previous time step.
    2. The current solution.
    3. The update solution, i.e. the one we have just projected.
    
    It is the your job to ensure that the updated solution is later on copied
    over into the new solution, so it is taken into account by the halo/patch
    reconstruction.

    ## Realisation
    
    I did not find a convenient, simple way how to tweak the projection from
    the toolbox according to this scheme. So I ended up with copying the whole
    thing over and change the critical things myself.

    ## Boundary flags

    Another difference compared to the toolbox projection is that the projection also sets the isUpdated() flag and the
    UpdatedTimeStamp(). As the projection writes to these two updated records, it is
    important that you roll it over afterwards. This is done via the mapping
    RollOverUpdatedFace.

    It is important to study this action set in combination with DynamicAMR. In the
    documentation of the latter I explain why we need the guard

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    not marker.hasBeenRefined()
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    if we want to support dynamic coarsening.
    
    ## Use action set in additional helper sweeps
    
    The action set expects users to specify one predicate per Runge-Kutta step.
    This means there is one if that checks which Runge-Kutta step is active. 
    This one is typically combined with two other checks: Is the solver 
    currently handling an unrefined cell (we only solve on the finest level) 
    and are we in the right phase of an enclave solve. The latter is relevant
    if and only if we work with enclave tasking.
    
    If the action set is used by an additional grid sweep outside of the actual
    time stepping, the checks become simpler: 
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      project_patch_onto_faces  = exahype2.solvers.rkfd.actionsets.ProjectPatchOntoFaces(my_solver)
      project_patch_onto_faces.guards  = [ my_solver._store_cell_data_default_guard() for x in range(0,my_solver.number_of_Runge_Kutta_steps()+1) ]
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    Here, we basically say "for each unrefined cell always for each Runge-Kutta
    step as well as for the final linear combination". The latter one is 
    reflected by the +1.
    
    Besides this, you also have to roll over the data:
    
    
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      roll_over_projected_faces = exahype2.solvers.rkfd.actionsets.RollOverUpdatedFace(my_solver, 
                                                                                       my_solver._store_face_data_default_guard(),
                                                                                       )
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    All solvers at the moment send out their face data in the suspended mode,
    so you should be fine here.


    """



    __Template_TouchCellLastTime = """
  logTraceIn( "touchCellLastTime(...)---ProjectPatchOntoFaces" );
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables( 1, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );

    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
        
    for(int d=0; d<Dimensions; d++) {
      /**
       * d-loop over all dimensions except d. The vector k's entry d is set
       * to 0. We start with the left/bottom face, i.e. the one closer to the 
       * coordinate system's origin.
       */
      dfore(k,{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},d,0) {
        for (int i=0; i<{{OVERLAP}}; i++) {
          tarch::la::Vector<Dimensions,int> patchCell   = k;
          tarch::la::Vector<Dimensions,int> overlapCell = k;
          patchCell(d)   = i;
          overlapCell(d) = i+{{OVERLAP}};
          
          // I could use the enumerator as well. Doesn't really matter here. One 
          // thing is tied to ExaHyPE, the other one to the blockstructured toolbox.
          int overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{OVERLAP}},d);
          logDebug(
            "touchCellLastTime(....)",
            "project " << patchCell << "->" << overlapCell <<
            "(" << enumeratorWithAuxiliaryVariables(0,patchCell,0) << "->" << overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}) << "): " <<
            fineGridCellEulerRKFDQ.value[ enumeratorWithAuxiliaryVariables(0,patchCell,0) ]
          );
          for (int j=0; j<{{NUMBER_OF_UNKNOWNS}}; j++) {
            {{FACES_ACCESSOR}}(d).value[overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j] = 
              {{CELL_ACCESSOR}}.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];

            {% if PREDICATE_NO<PREDICATES|length %}
              {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO+1]|length) %}
              {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO+1][WEIGHT_NO]!=0 %}
                {{FACES_ACCESSOR}}(d).value[overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j] += 
                  timeStepSize * {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO+1][WEIGHT_NO]}} * 
                  fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},patchCell,j) ];
              {% endif %}
              {% endfor %}
            {% endif %}
          }

          for (int j={{NUMBER_OF_UNKNOWNS}}; j<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; j++) {
            {{FACES_ACCESSOR}}(d).value[overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j] = 
              {{CELL_ACCESSOR}}.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];
          }
  
          patchCell(d)   = i+{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}-{{OVERLAP}};
          overlapCell(d) = i;
          
          overlapCellSerialised = toolbox::blockstructured::serialiseVoxelIndexInOverlap(overlapCell,{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{OVERLAP}},d);
          logDebug(
            "touchCellLastTime(....)",
            "project " << patchCell << "->" << overlapCell <<
            "(" << enumeratorWithAuxiliaryVariables(0,patchCell,0) << "->" << overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}) << "): " <<
            fineGridCellEulerRKFDQ.value[ enumeratorWithAuxiliaryVariables(0,patchCell,0) ]
          );
          for (int j=0; j<{{NUMBER_OF_UNKNOWNS}}; j++) {
            {{FACES_ACCESSOR}}(d+Dimensions).value[overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j] = 
              {{CELL_ACCESSOR}}.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];

            {% if PREDICATE_NO<PREDICATES|length %}
              {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO+1]|length) %}
              {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO+1][WEIGHT_NO]!=0 %}
                {{FACES_ACCESSOR}}(d+Dimensions).value[overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j] += 
                   timeStepSize * {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO+1][WEIGHT_NO]}} * 
                  fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},patchCell,j) ];
              {% endif %}
              {% endfor %}
            {% endif %}
          }

          for (int j={{NUMBER_OF_UNKNOWNS}}; j<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; j++) {
            {{FACES_ACCESSOR}}(d+Dimensions).value[overlapCellSerialised*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+j] = 
              {{CELL_ACCESSOR}}.value[ enumeratorWithAuxiliaryVariables(0,patchCell,j) ];
          }
        } // overlap depth loop
      } // dfore, i..e loop over submanifold
    } // loop over dimensions
  } // if predicate holds, i.e. right RK step
  {% endfor %} // all the predicates are done now
    
  for (int d=0; d<Dimensions; d++) {
    {{FACE_METADATA_ACCESSOR}}(d).setUpdated(1,true);
    {{FACE_METADATA_ACCESSOR}}(d).setUpdatedTimeStamp(1,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());
    {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdated(0,true);
    {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdatedTimeStamp(0,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());
  }  
  logTraceOut( "touchCellLastTime(...)---ProjectPatchOntoFaces" );
"""

    def __init__(self, solver):
        AbstractRKFDActionSet.__init__(self, solver)
        self._guards = []
        self._butcher_tableau = ButcherTableau(self._solver._rk_order)

    @property
    def guards(self):
        if self._guards == []:
            raise Exception("Guards are not initialised")
        return self._guards

    @guards.setter
    def guards(self, new_guards):
        if (
            new_guards != []
            and len(new_guards) != self._solver.number_of_Runge_Kutta_steps() + 1
        ):
            raise Exception(
                "Expect one guard per Runge Kutta step and a projection throughout the initialisation. Have {} steps but got guards {}".format(
                    self._solver.number_of_Runge_Kutta_steps(), new_guards
                )
            )
        self._guards = new_guards

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            d = {}
            if self._solver._patch_overlap_update.dim[0] % 2 != 0:
                print(
                    "Error: Patch associated to face has to have even number of cells. Otherwise, it is not a symmetric overlap."
                )
                assert patch_overlap.dim[0] % 2 == 0
            if self._solver._patch.dim[0] != self._solver._patch.dim[1]:
                print("Error: Can only handle square patches.")
                assert patch.dim[0] == patch.dim[1]
            if self._solver._patch_overlap_update.dim[1] != self._solver._patch.dim[0]:
                print("Error: Patch of overlap and patch of cell have to match")
                assert (
                    self._solver._patch_overlap_update.dim[1]
                    == self._solver._patch.dim[0]
                )

            d["PREDICATES"] = self.guards
            d["FACE_METADATA_ACCESSOR"] = (
                "fineGridFaces" + self._solver._face_label.name
            )
            d["CELL_METADATA_ACCESSOR"] = (
                "fineGridCell" "" + self._solver._cell_label.name
            )
            d["FACES_ACCESSOR"] = (
                "fineGridFaces" + self._solver._patch_overlap_update.name
            )
            d["CELL_ACCESSOR"] = "fineGridCell" + self._solver._patch.name
            d["BUTCHER_TABLEAU_WEIGHTS"] = self._butcher_tableau.weight_matrix()
            d[
                "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES"
            ] = self._butcher_tableau.time_step_sizes()

            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)

            result = jinja2.Template(self.__Template_TouchCellLastTime).render(**d)
            pass
        return result

    def get_includes(self):
        return """
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "exahype2/fd/BoundaryConditions.h"
#include "exahype2/enumerator/enumerator.h"
""" + AbstractRKFDActionSet.get_includes(
            self
        )
