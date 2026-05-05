# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFDActionSet import AbstractFDActionSet

import peano4
import jinja2


class RollOverUpdatedFace(AbstractFDActionSet):
    """

    This action set takes the updated data per face and writes it into
    new. So it takes the outcome from ProjectPatchOntoFaces and writes
    it onto the (persistent) face data. Before it does so, the action
    set also rolls over the new data to the old one.

    We may not roll over data if no data has been written onto the
    face. So we check the updated flag. It is reset by the UpdateFaceLabel
    action set in the grid subpackage and set again by either the face
    projection or the dynamic amr handling.

    """

    TemplateHandleFace = jinja2.Template(
        """
    if ( {{GUARD}} ) {
      const int normal = marker.getSelectedFaceNumber() % DIMENSIONS;
      
      
      // Left half
      if ({{FACE_METADATA_ACCESSOR}}.getUpdated(0)) {
        
        ::exahype2::fv::copyHalfOfHalo(
          {{UNKNOWNS}},
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal,
          false,    // it is the left layer, i.e. not right
          {{NEW_ACCESSOR}}.data(),
          {{OLD_ACCESSOR}}.data()
        );
        ::exahype2::fv::copyHalfOfHalo(
          {{UNKNOWNS}},
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal,
          false,    // it is the left layer, i.e. not right
          {{UPDATE_ACCESSOR}}.data(),
          {{NEW_ACCESSOR}}.data()
        );
        
        {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp(0, {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(0) );
        {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp(0, {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp(0) );
        
      }
      else {
      }
      
      // Right half
      if ({{FACE_METADATA_ACCESSOR}}.getUpdated(1)) {
        
        ::exahype2::fv::copyHalfOfHalo(
          {{UNKNOWNS}},
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal,
          true,    // it is the left layer, i.e. not right
          {{NEW_ACCESSOR}}.data(),
          {{OLD_ACCESSOR}}.data()
        );
        ::exahype2::fv::copyHalfOfHalo(
          {{UNKNOWNS}},
          {{DOFS_PER_AXIS}},
          {{OVERLAP}},
          normal,
          true,    // it is the left layer, i.e. not right
          {{UPDATE_ACCESSOR}}.data(),
          {{NEW_ACCESSOR}}.data()
        );

        
        {{FACE_METADATA_ACCESSOR}}.setOldTimeStamp(1, {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(1) );
        {{FACE_METADATA_ACCESSOR}}.setNewTimeStamp(1, {{FACE_METADATA_ACCESSOR}}.getUpdatedTimeStamp(1) );
        
      }
      else {
      }
      
      int index = 0;
    }
"""
    )

    def __init__(self, solver, guard):
        """

        ## Attributes

        overwrite_old_and_new_solution_with_update: Boolean
          This flag should be set if you use the action set throughout the
          initialisation, where you determine an update on the face and
          this update should then be used for old and new face data (as
          they are the same). If it is set to false, then the new solution
          is backuped in old, and the update overwrites the new data field.

        """
        AbstractFDActionSet.__init__(self, solver)
        self.d = {}
        self.d["GUARD"] = guard
        self.d["UNKNOWNS"] = str(solver._patch_overlap_update.no_of_unknowns)
        self.d["DOFS_PER_AXIS"] = str(solver._patch.dim[0])
        self.d["OVERLAP"] = str(int(solver._patch_overlap_update.dim[0] / 2))
        self.d["UPDATE_ACCESSOR"] = "fineGridFace" + solver._patch_overlap_update.name
        self.d["OLD_ACCESSOR"] = "fineGridFace" + solver._patch_overlap_old.name
        self.d["NEW_ACCESSOR"] = "fineGridFace" + solver._patch_overlap_new.name
        self.d["FACE_METADATA_ACCESSOR"] = "fineGridFace" + solver._face_label.name

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            self._solver._init_dictionary_with_default_parameters(self.d)
            self._solver.add_entries_to_text_replacement_dictionary(self.d)
            result = self.TemplateHandleFace.render(**self.d)
            pass
        return result

    def get_includes(self):
        return """
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "exahype2/fv/PatchUtils.h"
""" + AbstractFDActionSet.get_includes(
            self
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
