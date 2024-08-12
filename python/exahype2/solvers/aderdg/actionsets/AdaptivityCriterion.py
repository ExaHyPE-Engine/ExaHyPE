# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractAderDGActionSet import AbstractAderDGActionSet

# import peano4
import peano4.solversteps
import jinja2


class AdaptivityCriterion(AbstractAderDGActionSet):
    """

    The action set to realise AMR

    AMR is a multistep process in ExaHyPE. Most of the relevant documentation
    on it is documented in the class exahype2::RefinementControl.

    There are different phases in ExaHyPE: grid generation (either with
    refinement or without), initialisation, plotting and time stepping. It
    depends on your solver in which time stepping to use AMR, but the there
    are some things to take into account that are in common for all schemes:

    - The grid creation does not use any actual solver data, so we should not
      evalutae the adaptivity criterion here.
    - InitGrid has volumetric data and might want to evaluate the criterion.
      This is implicitly done by the t=0 check below.
    - The plotting does not alter the solution. It thus makes no sense to
      evaluate the criterion here.

    Please consult EnclaveTasking.create_action_sets for details regarding
    the handling of AMR within the enclave tasking concept.

    """

    TemplateAMR = """  
  logTraceInWith2Arguments( "touchCellFirstTime(...)", marker.willBeRefined(), marker.hasBeenRefined() );

  ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

  {% if GRID_CONSTRUCTION %}
  if ( 
    not marker.willBeRefined() 
    and
    tarch::la::greater( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MaxAdmissibleCellH) 
  ) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } 
  else if ( 
    not marker.willBeRefined() 
    and
    tarch::la::greaterEquals( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MinAdmissibleCellH )
    and
    not fineGridCell{{CELL_LABEL_NAME}}.getAMDCriterionEvaluatedThroughoutGridConstruction()
  ) { 
    refinementCriterion = repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
      nullptr,
      marker.x(),
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp()
    );
    fineGridCell{{CELL_LABEL_NAME}}.setAMDCriterionEvaluatedThroughoutGridConstruction(true);
  }
  else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
  {% else %}
  if ( 
    {{PREDICATE}}
    and
    not marker.willBeRefined() 
    and
    tarch::la::greater( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MaxAdmissibleCellH) 
  ) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } 
  else if ( 
    {{PREDICATE}}
    and
    fineGridCell{{CELL_LABEL_NAME}}.getHasUpdated()
  ) { 
    refinementCriterion = repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      marker.x(),
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp()
    );
     
    if (
      refinementCriterion==::exahype2::RefinementCommand::Refine 
      and 
      tarch::la::smallerEquals( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MinAdmissibleCellH )
    ) {
      logDebug( "touchCellFirstTime(...)", "drop refine instructions, as mesh would be too small" );
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
    else if (refinementCriterion==::exahype2::RefinementCommand::Erase and 3.0* tarch::la::max( marker.h() ) > repositories::{{SOLVER_INSTANCE}}.MaxAdmissibleCellH ) {
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
  }
  else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
  {% endif %}
    
  _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, {{EVENT_LIFETIME}} );
  logTraceOutWith1Argument( "touchCellFirstTime(...)", toString(refinementCriterion) );
  """

    def __init__(
        self,
        solver,
        guard,
        build_up_new_refinement_instructions,
        implement_previous_refinement_instructions,
        called_by_grid_construction, 
        event_lifetime=2,
    ):
        """

        :: Attributes

        _implement_previous_refinement_instructions: Boolean
          This name might be misleading. Consult exahype2::RefinementControl for
          a description of the control flow. This flag controls if instructions
          are picked up from the RefinementControl database.


        :: Arguments

        guard: C++ expression which evaluates to true or false
          A per cell decision whether we should study a cell or not.

        build_up_new_refinement_instructions: Boolean
          See remarks on multistep realisation of AMR in C++ class
          exahype2::RefinementControl.

        implement_previous_refinement_instructions: Boolean
          See remarks on multistep realisation of AMR in C++ class
          exahype2::RefinementControl.

        event_lifetime: Int
          See setter below

        """
        super(AdaptivityCriterion, self).__init__(solver)
        self.guard = guard
        self._build_up_new_refinement_instructions = (
            build_up_new_refinement_instructions
        )
        self._implement_previous_refinement_instructions = (
            implement_previous_refinement_instructions
        )
        self._event_lifetime = event_lifetime
        self._called_by_grid_construction = called_by_grid_construction

    def get_body_of_getGridControlEvents(self):
        if self._implement_previous_refinement_instructions:
            return """
    return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();
"""
        else:
            return """
    return std::vector< peano4::grid::GridControlEvent >();
"""

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            self._build_up_new_refinement_instructions
            and operation_name == peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL
        ):
            result = """
  _localRefinementControl.clear();
"""

        if (
            self._build_up_new_refinement_instructions
            and operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL
        ):
            result = """
  ::exahype2::RefinementControlService::getInstance().merge( _localRefinementControl );
"""

        if (
            self._build_up_new_refinement_instructions
            and operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d = {}

            d["CELL_LABEL_NAME"] = self._solver._cell_label.name
            d["UNKNOWNS"] = str(self._solver._current_time_step.no_of_unknowns)
            # d[ "DOFS_PER_AXIS" ]      = str(self._solver._patch.dim[0])
            # d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_2D" ] = str(self._solver._patch.no_of_unknowns * self._solver._patch.dim[0] * self._solver._patch.dim[0])
            # d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_3D" ] = str(self._solver._patch.no_of_unknowns * self._solver._patch.dim[0] * self._solver._patch.dim[0] * self._solver._patch.dim[0])
            d["CELL_ACCESSOR"] = "fineGridCell" + self._solver._current_time_step.name
            d["PREDICATE"] = (
                "not marker.willBeRefined() and not marker.hasBeenRefined() and "
                + self.guard
            )
            # @todo Falsch: Auch nur im letzten Schritt oder so evaluieren bei RK
            d["EVENT_LIFETIME"] = self._event_lifetime
            d[ "GRID_CONSTRUCTION" ]  = self._called_by_grid_construction
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(self.TemplateAMR).render(**d)

        return result

    def set_event_lifetime(self, value):
        """

        By default, a refinement/coarsening event is only alive for one grid sweep.
        After that one, the set of refine/coarsen commands is reset and we start
        again. If you work with local time stepping, subcycling, multiphysics codes,
        you might want to keep an event for more steps. In this case, you have to
        invoke this setter.

        """
        self._event_lifetime = value

    def get_attributes(self):
        return """
    ::exahype2::RefinementControl         _localRefinementControl;
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(AdaptivityCriterion, self).get_includes()
            + """
#include "exahype2/RefinementControlService.h"
      """
        )
