# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2


class RollOverUpdates(AbstractFVActionSet):
    """
    Back the current QNew face and cell data up into QOld.

    The backup happens before the next time step overwrites QNew. A face half is
    normally rolled over only once data has been written onto it, which the
    updated flag records. That flag is reset by the UpdateFaceLabel action set in
    the grid subpackage and set again by either the face projection or the
    dynamic AMR handling. The first time step after a realised topology change is
    the exception: there the mesh-update sweeps cleared the flag on halves whose
    data is still valid, so those halves roll over too.
    """

    TemplateHandleFace = jinja2.Template(
        """
    if ({{FACE_GUARD}}) {
      const int normal = marker.getSelectedFaceNumber() % DIMENSIONS;

      // The mesh-update sweeps clear the Updated entries, which would silently
      // cancel the pending roll-over of every face their projection did not
      // reach; the two sides of such a face would then solve Riemann problems
      // on halo data one step older than the cell interiors and mass would
      // leak. The first time step after a realised topology change therefore
      // rolls every half whose data no change invalidated.
      const bool rollAfterTopologyChange =
        ::exahype2::RefinementControlService::getInstance().hasRealisedTopologyChangesSinceLastTimeStep();

      // Left half
      if ({{FACE_METADATA_ACCESSOR}}.getUpdated(0)
          or (rollAfterTopologyChange and not {{FACE_METADATA_ACCESSOR}}.getRequiresDataTransfer(0))) {
        ::exahype2::fv::copyHalfOfHalo(
          {{UNKNOWNS}},
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal,
          false, // Left layer
          {{FACE_NEW_ACCESSOR}}.data(),
          {{FACE_OLD_ACCESSOR}}.data()
        );
        {{FACE_METADATA_ACCESSOR}}.setUpdated(0, false);
        {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp(0, {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(0));
        {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp(0, {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp(0));
      } else {
        {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp(0, {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(0));
        {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp(0, {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp(0));
      }

      // Right half
      if ({{FACE_METADATA_ACCESSOR}}.getUpdated(1)
          or (rollAfterTopologyChange and not {{FACE_METADATA_ACCESSOR}}.getRequiresDataTransfer(1))) {
        ::exahype2::fv::copyHalfOfHalo(
          {{UNKNOWNS}},
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal,
          true, // Right layer
          {{FACE_NEW_ACCESSOR}}.data(),
          {{FACE_OLD_ACCESSOR}}.data()
        );

        {{FACE_METADATA_ACCESSOR}}.setUpdated(1, false);
        {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp(1, {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(1));
        {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp(1, {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp(1));
      } else {
        {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp(1, {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(1));
        {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp(1, {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp(1));
      }
    }
"""
    )

    TemplateHandleCell = jinja2.Template(
        """
    if ({{CELL_METADATA_ACCESSOR}}.getHasUpdated() and {{CELL_GUARD}}) {
      assertion({{CELL_OLD_ACCESSOR}}.Cardinality == {{CELL_NEW_ACCESSOR}}.Cardinality, {{CELL_OLD_ACCESSOR}}.Cardinality, {{CELL_NEW_ACCESSOR}}.Cardinality);
      {{CELL_METADATA_ACCESSOR}}.setHasUpdated(false);
      {{CELL_OLD_ACCESSOR}}.swap({{CELL_NEW_ACCESSOR}});
    }
"""
    )

    def __init__(self, solver, face_guard, cell_guard):
        """
        Set up the action set with its two roll-over guards.

        ## Arguments

        face_guard: String
          C++ predicate deciding whether a face is rolled over in this sweep.

        cell_guard: String
          C++ predicate deciding whether a cell is rolled over. The action set
          appends a check for the solver's TimeStep state, so a roll-over never
          happens during grid construction, initialisation or a mesh update.
        """
        AbstractFVActionSet.__init__(self, solver)
        cell_guard += (
            " and "
            + f"repositories::{solver.get_name_of_global_instance()}.getSolverState() == Abstract{solver.name}::SolverState::TimeStep"
        )
        self.d = {}
        self.d["FACE_GUARD"] = face_guard
        self.d["CELL_GUARD"] = cell_guard
        self.d["UNKNOWNS"] = str(solver._patch_overlap_new.no_of_unknowns)
        self.d["DOFS_PER_AXIS"] = str(solver._patch.dim[0])
        self.d["OVERLAP"] = str(int(solver._patch_overlap_new.dim[0] / 2))
        self.d["FACE_OLD_ACCESSOR"] = (
            "fineGridFace" + solver.previous_unknown_identifier()
        )
        self.d["FACE_NEW_ACCESSOR"] = (
            "fineGridFace" + solver.current_unknown_identifier()
        )
        self.d["CELL_OLD_ACCESSOR"] = (
            "fineGridCell" + solver.previous_unknown_identifier()
        )
        self.d["CELL_NEW_ACCESSOR"] = (
            "fineGridCell" + solver.current_unknown_identifier()
        )
        self.d["FACE_METADATA_ACCESSOR"] = "fineGridFace" + solver._face_label.name
        self.d["CELL_METADATA_ACCESSOR"] = "fineGridCell" + solver._cell_label.name

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            self._solver._init_dictionary_with_default_parameters(self.d)
            self._solver.add_entries_to_text_replacement_dictionary(self.d)
            result = self.TemplateHandleFace.render(**self.d)
            pass
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            self._solver._init_dictionary_with_default_parameters(self.d)
            self._solver.add_entries_to_text_replacement_dictionary(self.d)
            result = self.TemplateHandleCell.render(**self.d)
            pass
        return result

    def get_includes(self):
        return """
#include "peano4/Loop.h"
#include "exahype2/Enumeration.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/RefinementControlService.h"
""" + AbstractFVActionSet.get_includes(
            self
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
