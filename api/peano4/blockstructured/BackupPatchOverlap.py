# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


class BackupPatchOverlap(ActionSet):
    """Copies data between face-overlap patches with identical layouts.

    ``invoke_in_touch_first`` selects the first or last face touch, and ``guard``
    supplies the C++ predicate for the generated copy.
    """

    def __init__(
        self,
        patch_overlap_in,
        patch_overlap_out,
        invoke_in_touch_first,
        guard="",
        additional_includes="",
    ):
        super().__init__(descend_invocation_order=1)
        self._dictionary = {}
        self._dictionary["UNKNOWNS"] = str(patch_overlap_in.no_of_unknowns)
        self._dictionary["DOFS_PER_AXIS"] = str(patch_overlap_in.dim[1])
        self._dictionary["OVERLAP"] = str(patch_overlap_in.dim[0] // 2)
        self._dictionary["FACES_ACCESSOR_IN"] = "fineGridFace" + patch_overlap_in.name
        self._dictionary["FACES_ACCESSOR_OUT"] = "fineGridFace" + patch_overlap_out.name
        self._dictionary["GUARD_PREDICATE"] = guard

        self._invoke_in_touch_first = invoke_in_touch_first
        self._additional_includes = additional_includes

    def set_guard_predicate(self, guard):
        self._dictionary["GUARD_PREDICATE"] = guard

    def get_constructor_body(self):
        return ""

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<::peano4::grid::GridControlEvent>();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    __Template = """
  if ({GUARD_PREDICATE}) {{
    int destinationIndex = 0;
    ::peano4::dForEachExclusive(
      {DOFS_PER_AXIS},
      0,
      0,
      [&]([[maybe_unused]] const ::tarch::la::Vector<DIMENSIONS, int>& overlapCell) {{
        for (int overlapIndex = 0; overlapIndex < {OVERLAP} * 2; overlapIndex++) {{
          for (int unknown = 0; unknown < {UNKNOWNS}; unknown++) {{
            {FACES_ACCESSOR_OUT}.data()[destinationIndex] =
              {FACES_ACCESSOR_IN}.data()[destinationIndex];
            ++destinationIndex;
          }}
        }}
      }}
    );
  }}
"""

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            self._invoke_in_touch_first
            and operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            result = self.__Template.format(**self._dictionary)
        elif (
            not self._invoke_in_touch_first
            and operation_name == ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            result = self.__Template.format(**self._dictionary)
        return result

    def get_attributes(self):
        return ""

    def get_includes(self):
        return (
            """
#include "peano4/Loop.h"
"""
            + self._additional_includes
        )
