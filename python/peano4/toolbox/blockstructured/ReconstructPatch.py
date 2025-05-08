# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import IntEnum

from peano4.solversteps.ActionSet import ActionSet


class ReconstructPatch(ActionSet):
    """!
    This class assumes that you have an NxNxN patch within your block. It also assumes that
    you have an 2MxNxN patch on your faces. M<N. The code interprets these face-associated
    data as overlap with the patch, hooks into touchCellLastTime, and projects the cell's
    patch data onto the overlap (simple copy). See ProjectPatchOntoFaces for details.

    If the face patches are auxiliary patches holding copies from the cell patches, then
    we can reconstruct N+2M x N+2M x N+2M patches per cell whenever we hit a cell: We
    create a new temporary array, and copy data from both the cell patch and the faces
    into this array (gather operation). After that, we can launch the passed functor
    giving it access to the temporary, large array plus the original patch data.
    """

    def __init__(
        self,
        patch,
        patch_overlap,
        patch_reconstructed,
        guard="true",
        add_assertions_to_halo_exchange=True,
    ):
        """!
      patch: peano4.datamodel.Patch
          Patch which is to be reconstructed

      patch_overlap: peano4.datamodel.Patch
          Consult remark above about how the dimensions of this overlap patch have to match

      patch_reconstructed: peano4.datamodel.Patch
          Patch which is used to reconstruct
        """
        super(ReconstructPatch, self).__init__(
            descend_invocation_order=1, parallel=False
        )

        assert(patch_overlap.dim[0] % 2 == 0)
        assert(patch.dim[0] == patch.dim[1])
        assert(patch_overlap.dim[1] == patch.dim[0])

        self.guard = guard
        self.no_of_unknowns = int(patch.no_of_unknowns)
        self.dofs_per_axis = int(patch.dim[0])
        self.total_overlap = int(patch_overlap.dim[0])
        self.overlap_name = patch_overlap.name
        self.patch_name = patch.name
        self.patch_reconstructed_name = patch_reconstructed.name
        self.add_assertions_to_halo_exchange = add_assertions_to_halo_exchange

    def _add_action_set_entries_to_dictionary(self, d):
        """!
        Befill dictionary

        If you inherit from this class and specialise this routine, you can add
        more entries to the dictionary. This is important if you change the
        actual compute kernels (templates) and use further unknowns in there. If
        you specialise the function, i.e. overwrite it in a subclass, please
        ensure that you use the superclass still.
        """
        d["GUARD"] = self.guard

        assert isinstance(self.no_of_unknowns, int)
        assert isinstance(self.dofs_per_axis, int)
        assert isinstance(self.total_overlap, int)

        d["UNKNOWNS"] = str(self.no_of_unknowns)
        d["DOFS_PER_AXIS"] = str(self.dofs_per_axis)
        d["OVERLAP"] = str(int(self.total_overlap / 2))
        d["FACES_ACCESSOR"] = "fineGridFaces" + self.overlap_name
        d["CELL_ACCESSOR"] = "fineGridCell" + self.patch_name
        d["RECONSTRUCTED_ACCESSOR"] = "fineGridCell" + self.patch_reconstructed_name

        if self.add_assertions_to_halo_exchange:
            d["ASSERTION_PREFIX_FOR_HALO"] = "false"
        else:
            d["ASSERTION_PREFIX_FOR_HALO"] = "true"

    def get_constructor_body(self):
        return """ _treeNumber = treeNumber;
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    _Template_ReconstructPatch = r"""
  logTraceInWith1Argument("touchCellFirstTime()", marker.toString());

  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {{
    int base   = 1;
    int result = 0;
    for (int d = 0; d < Dimensions; d++) {{
      result += overlapCell(d) * base;
      if (d == normal) {{
        base *= {OVERLAP} * 2;
      }} else {{
        base *= {DOFS_PER_AXIS};
      }}
    }}
    return result;
  }};

  if ({GUARD}) {{
    auto* /*const*/ oldQWithHalo = {RECONSTRUCTED_ACCESSOR}.value;

    //
    // Loop over original patch (k) and copy stuff over.
    //
    dfor(sourceCell,{DOFS_PER_AXIS}) {{
      tarch::la::Vector<Dimensions,int> destinationCell = sourceCell + tarch::la::Vector<Dimensions,int>({OVERLAP});
      int sourceCellSerialised       = peano4::utils::dLinearised(sourceCell,{DOFS_PER_AXIS});
      int destinationCellSerialised  = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
      for (int j=0; j<{UNKNOWNS}; j++) {{
        oldQWithHalo[destinationCellSerialised*{UNKNOWNS}+j] = {CELL_ACCESSOR}.value[ sourceCellSerialised*{UNKNOWNS}+j ];
        assertion4(oldQWithHalo[destinationCellSerialised*{UNKNOWNS}+j]==oldQWithHalo[destinationCellSerialised*{UNKNOWNS}+j], sourceCell, j, _treeNumber, marker.toString() );
      }}
    }}

    //
    // Bring in the auxiliary patches, i.e. befill halo
    //
    for(int d=0; d<Dimensions; d++) {{
      //
      // d-loop over all dimensions except d. The vector k's entry d is set
      // to 0. We start with the left/bottom face, i.e. the one closer to the
      // coordinate system's origin.
      //
      dfore(k,{DOFS_PER_AXIS},d,0) {{
        for (int i=0; i<{OVERLAP}; i++) {{
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>({OVERLAP});
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;

          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<{UNKNOWNS}; j++) {{
            oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ] = {FACES_ACCESSOR}(d).value[ sourceCellSerialised*{UNKNOWNS}+j ];
            assertion7({ASSERTION_PREFIX_FOR_HALO} or oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ]==oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ],
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber, marker.toString()
            );
          }}

          destinationCell(d) = i+{DOFS_PER_AXIS}+{OVERLAP};
          sourceCell(d)      = i+{OVERLAP};

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<{UNKNOWNS}; j++) {{
            oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ] = {FACES_ACCESSOR}(d+Dimensions).value[ sourceCellSerialised*{UNKNOWNS}+j ];
            assertion7({ASSERTION_PREFIX_FOR_HALO} or oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ]==oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber, marker.toString()
            );
          }}
        }}
      }}
    }}
  }}

  logTraceOut("touchCellFirstTime()");
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            d = {}
            self._add_action_set_entries_to_dictionary(d)
            result = self._Template_ReconstructPatch.format(**d)
            pass
        return result

    def get_attributes(self):
        return """int _treeNumber;
"""

    def get_includes(self):
        return """
#include <functional>
#include "peano4/utils/Loop.h"
#include "tarch/multicore/Core.h"
"""
