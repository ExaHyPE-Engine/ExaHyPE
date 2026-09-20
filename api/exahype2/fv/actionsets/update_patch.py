# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet


class UpdatePatch(SolverActionSet):
    """Advances the patch of every leaf by one time step.

    A leaf the solver does not advance in this sweep reports its time stamp to the statistics.
    """

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{GUARD}}) {
    ::exahype2::fv::updatePatch<
      {{RECONSTRUCTION}},
      {{TIME_STEPPING}},
      {{ "true" if USE_FLUX else "false" }},
      {{ "true" if USE_NCP else "false" }},
      {{ "true" if USE_SOURCE else "false" }},
      {{ "true" if USE_DIFFUSIVE_SOURCE else "false" }},
      {{ "true" if USE_RIEMANN_SOLVER else "false" }},
      {{ "true" if COMPUTE_MAX_EIGENVALUE else "false" }},
      {{ "true" if (COMPUTE_MAX_EIGENVALUE and USE_MAX_EIGENVALUE) else "false" }},
      {{COMPUTE_PRECISION}}
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      {{CELL_METADATA_ACCESSOR}},
      {{FACE_METADATA_ACCESSOR}},
      {{CELL_OLD_ACCESSOR}}.data(),
      {{CELL_NEW_ACCESSOR}}.data(),
      {{FACE_OLD_ACCESSOR}},
      {{FACE_NEW_ACCESSOR}},
      [&](
        [[maybe_unused]] {{STORAGE_PRECISION}}* const QOut,
        [[maybe_unused]] const double                 timeStamp,
        [[maybe_unused]] const double                 timeStepSize
      ) {
{%- if POSTPROCESS_UPDATED_PATCH %}
        {{POSTPROCESS_UPDATED_PATCH | indent(8)}}
{%- endif %}
      }
    );
  }
{%- if REPORT_EXCLUDED_LEAVES %} else if ({{LEAF_GUARD}}) {
    repositories::{{SOLVER_INSTANCE}}.updateTimeStampAndPatchSize(
      {{CELL_METADATA_ACCESSOR}}.getTimeStamp(),
      tarch::la::min(marker.getH()),
      tarch::la::max(marker.getH())
    );
  }
{%- endif %}
"""

    _TEMPLATE_END_TRAVERSAL = """
  repositories::{{SOLVER_INSTANCE}}.reduceLocalUpdates();
"""

    _LEAF_GUARD = "not marker.willBeRefined() and not marker.hasBeenRefined()"

    def __init__(self, solver):
        super().__init__(solver)
        self.guard = self._LEAF_GUARD
        self._cell_old_accessor = "fineGridCell" + solver.previous_unknown_identifier()
        self._cell_new_accessor = "fineGridCell" + solver.current_unknown_identifier()
        self._face_old_accessor = "fineGridFaces" + solver.previous_unknown_identifier()
        self._face_new_accessor = "fineGridFaces" + solver.current_unknown_identifier()
        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" + solver._cell_label.name

    def includes(self):
        return (
            super().includes()
            + ["exahype2/fv/UpdatePatch.h"]
            + list(self._solver.scheme.subcell_scheme().includes)
        )

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
                GUARD=self.guard,
                LEAF_GUARD=self._LEAF_GUARD,
                REPORT_EXCLUDED_LEAVES=self.guard != self._LEAF_GUARD,
                CELL_OLD_ACCESSOR=self._cell_old_accessor,
                CELL_NEW_ACCESSOR=self._cell_new_accessor,
                FACE_OLD_ACCESSOR=self._face_old_accessor,
                FACE_NEW_ACCESSOR=self._face_new_accessor,
                FACE_METADATA_ACCESSOR=self._face_metadata_accessor,
                CELL_METADATA_ACCESSOR=self._cell_metadata_accessor,
                POSTPROCESS_UPDATED_PATCH=textwrap.dedent(
                    self.render(self._solver.postprocess_updated_patch)
                ).strip(),
            )
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self.render(self._TEMPLATE_END_TRAVERSAL)
        return ""
