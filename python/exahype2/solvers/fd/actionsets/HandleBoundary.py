# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFDActionSet import AbstractFDActionSet

import peano4
import jinja2


class HandleBoundary(AbstractFDActionSet):
    """

    The global periodic boundary conditions are set in the Constants.h.

    There is no need to update QOld along the faces. QOld is always rolled
    over from QNew, i.e. we may assume it is already set correctly. It is
    not correctly set in the very first time step (after InitGrid, we haven't
    yet set QOld), so apply the boundary conditions here would even confuse
    the assertions.

    """

    TemplateHandleBoundary_Prologue = """
    if (
      {{PREDICATE}}
      and
      not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%DIMENSIONS]
      and
      not marker.hasBeenRefined() 
      and 
      fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary()
    ) {
"""

    TemplateHandleBoundary_KernelCalls = """
      kernels::{{SOLVER_NAME}}::applyBoundaryConditions<{{STORAGE_PRECISION}}>(
        [&](
          const {{STORAGE_PRECISION}} * NOALIAS   Qinside,
          {% for i in range(ORDER) -%}
          {{COMPUTE_PRECISION}} * NOALIAS         Qoutside{{i}},
          {% endfor %}
          const tarch::la::Vector<DIMENSIONS,double>&  x,
          const tarch::la::Vector<DIMENSIONS,double>&  h,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions( Qinside, {%- for i in range(ORDER) -%}Qoutside{{i}},{%- endfor -%} x, h, t, normal );
        },
        marker.getX(),
        marker.getH(),
        {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(marker.getSelectedFaceNumber()<DIMENSIONS ? 1 : 0),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_NODES_PER_AXIS}},
        {{ORDER}},
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        fineGridFace{{UNKNOWN_IDENTIFIER}}New.data()
      );
      kernels::{{SOLVER_NAME}}::applyBoundaryConditions<{{STORAGE_PRECISION}}>(
        [&](
          const {{STORAGE_PRECISION}} * NOALIAS   Qinside,
          {% for i in range(ORDER) -%}
          {{COMPUTE_PRECISION}} * NOALIAS         Qoutside{{i}},
          {% endfor %}
          const tarch::la::Vector<DIMENSIONS,double>&  x,
          const tarch::la::Vector<DIMENSIONS,double>&  h,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions( Qinside, {%- for i in range(ORDER) -%}Qoutside{{i}}, {%- endfor -%} x, h, t, normal );
        },
        marker.getX(),
        marker.getH(),
        {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(marker.getSelectedFaceNumber()<DIMENSIONS ? 1 : 0),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_NODES_PER_AXIS}},
        {{ORDER}},
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        fineGridFace{{UNKNOWN_IDENTIFIER}}Old.data()
      );
"""

    TemplateHandleBoundary_Epilogue = """

      bool isLeftEntryOutside = marker.getSelectedFaceNumber() < DIMENSIONS;
      double innerTimeStamp;
      
      innerTimeStamp = {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp( isLeftEntryOutside ? 1 : 0 );
      {{FACE_METADATA_ACCESSOR}}.setUpdatedTimeStamp( isLeftEntryOutside ? 0 : 1, innerTimeStamp );
      
      innerTimeStamp = {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp( isLeftEntryOutside ? 1 : 0 );
      {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp( isLeftEntryOutside ? 0 : 1, innerTimeStamp );

      innerTimeStamp = {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp( isLeftEntryOutside ? 1 : 0 );
      {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp( isLeftEntryOutside ? 0 : 1, innerTimeStamp );

    }
    else {
      logDebug("skip face " << marker.toString());
    }
"""

    def __init__(self, solver, guard):
        AbstractFDActionSet.__init__(self, solver)
        self.guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            self.d = {}
            self._solver._init_dictionary_with_default_parameters(self.d)
            self._solver.add_entries_to_text_replacement_dictionary(self.d)
            self.d["PREDICATE"] = self.guard
            self.d["FACE_METADATA_ACCESSOR"] = (
                "fineGridFace" + self._solver._face_label.name
            )
            result = jinja2.Template(self.TemplateHandleBoundary_Prologue).render(
                **self.d
            )
            result += jinja2.Template(self.TemplateHandleBoundary_KernelCalls).render(
                **self.d
            )
            result += jinja2.Template(self.TemplateHandleBoundary_Epilogue).render(
                **self.d
            )
            pass
        return result

    def get_includes(self):
        return AbstractFDActionSet.get_includes(self)

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
