# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

from .DoF import DoFAssociation


class FloatTypes:
    """Maps a configured spelling to a generated C++ scalar type."""

    def __init__(self, float_type):
        if float_type == "float":
            self.cpp = "float"
        elif float_type == "double":
            self.cpp = "double"
        elif float_type == "long double":
            self.cpp = "long double"
        elif float_type == "_Float16":
            self.cpp = "_Float16"
        elif float_type == "std::float16_t":
            self.cpp = "std::float16_t"
        elif float_type == "__bf16":
            self.cpp = "__bf16"
        elif float_type == "std::bfloat16_t":
            self.cpp = "std::bfloat16_t"
        else:
            raise ValueError("Float type " + float_type + " not recognised")


class PatchToData:
    """Maps a patch onto a generated data class.

    A boundary or periodic exchange copies the inbound halo half from the
    exchanged snapshot. halo_layout selects overlapping volume layers or
    contiguous projection halves; None disables the merge.
    """

    def __init__(self, patch, float_type="double"):
        self.data = patch
        self.halo_layout = None
        self.send_condition = "true"
        self.receive_and_merge_condition = "true"
        self.can_send = "true"
        self.can_receive = "true"
        self.includes = ""
        self.float_type = FloatTypes(float_type)

    def __str__(self):
        result = (
            """Map to data
  - Send condition:          """
            + self.send_condition
            + """
  - Receive/merge condition: """
            + self.receive_and_merge_condition
            + """
  - Halo layout:             """
        )
        if self.halo_layout is not None:
            result += self.halo_layout + "\n"
        else:
            result += """none
"""
        return result

    def _create_merge_method_definition(self):
        if self.halo_layout is None:
            return ""
        if self.halo_layout == "projection_halves":
            return """
  static_assert(
    Cardinality % 2 == 0,
    "Projection data requires two equal face halves."
  );
  constexpr int FaceSideCardinality = Cardinality / 2;

  auto* const       destination = this->data();
  const auto* const source      = neighbour.data();
  assertion(destination != nullptr);
  assertion(source != nullptr);

  const int normalDimension = marker.getSelectedFaceNumber() % DIMENSIONS;
  const int neighbourSide   = marker.outerNormal()[normalDimension] > 0.0 ? 1 : 0;

  std::copy_n(
    &source[neighbourSide * FaceSideCardinality],
    FaceSideCardinality,
    &destination[neighbourSide * FaceSideCardinality]
  );
"""
        if self.halo_layout == "overlapping_volumes":
            overlapCellCount = int(self.data.dim[0])
            degreesOfFreedomPerAxis = int(self.data.dim[1])
            unknownCount = int(self.data.no_of_unknowns)
            assert overlapCellCount > 0, "The overlap direction contains cells."
            assert (
                overlapCellCount % 2 == 0
            ), "The overlap direction holds both adjacent patches."
            assert degreesOfFreedomPerAxis > 0, "A patch axis contains cells."
            assert unknownCount > 0, "A patch cell contains values."
            templateData = {
                "OVERLAP": overlapCellCount // 2,
                "DOFS_PER_AXIS": degreesOfFreedomPerAxis,
                "UNKNOWNS": unknownCount,
            }
            return """
  const auto serialisePatchIndex = [](
    const tarch::la::Vector<DIMENSIONS, int>& overlapCell,
    const int                                 normalDimension
  ) {{
    int stride         = 1;
    int serialisedCell = 0;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {{
      serialisedCell += overlapCell(dimension) * stride;
      if (dimension == normalDimension) {{
        stride *= {OVERLAP} * 2;
      }} else {{
        stride *= {DOFS_PER_AXIS};
      }}
    }}
    return serialisedCell;
  }};

  auto* const       destination = this->data();
  const auto* const source      = neighbour.data();
  assertion(destination != nullptr);
  assertion(source != nullptr);

  const int  normalDimension = marker.getSelectedFaceNumber() % DIMENSIONS;
  const bool copyUpperHalf   = marker.outerNormal()(normalDimension) > 0.0;
  ::peano4::dForEachExclusive(
    {DOFS_PER_AXIS},
    normalDimension,
    0,
    [&](const tarch::la::Vector<DIMENSIONS, int>& tangentialCell) {{
      for (int overlapIndex = 0; overlapIndex < {OVERLAP}; overlapIndex++) {{
        auto overlapCell = tangentialCell;
        overlapCell(normalDimension) +=
          copyUpperHalf ? overlapIndex + {OVERLAP} : overlapIndex;
        const int serialisedCell = serialisePatchIndex(overlapCell, normalDimension);
        for (int unknown = 0; unknown < {UNKNOWNS}; unknown++) {{
          const int valueIndex = serialisedCell * {UNKNOWNS} + unknown;
          assertion(source[valueIndex] == source[valueIndex], source[valueIndex]);
          destination[valueIndex] = source[valueIndex];
        }}
      }}
    }}
  );
""".format(
                **templateData
            )
        raise ValueError("Unknown halo layout " + str(self.halo_layout))

    def _create_includes(self):
        if self.halo_layout == "overlapping_volumes":
            return (
                self.includes
                + """
#include "peano4/Loop.h"
"""
            )
        return self.includes

    def get_stack_container(self):
        return "::peano4::grid::Stack<" + self.data.get_full_qualified_type() + ">"

    def get_header_file_include(self):
        """Return the include statement for the generated data container."""
        return (
            """
#include \""""
            + self.data.namespace[-1]
            + "/"
            + self.data.name
            + """.h"
"""
        )

    def _get_dictionary_for_output(self):
        assert self.data.association in (
            DoFAssociation.Face,
            DoFAssociation.Cell,
        ), "Patch data is face- or cell-associated."
        templateData = {
            "CARDINALITY_2D": str(
                int(self.data.no_of_unknowns * self.data.dim[0] * self.data.dim[1])
            ),
            "CARDINALITY_3D": str(
                int(
                    self.data.no_of_unknowns
                    * self.data.dim[0]
                    * self.data.dim[1]
                    * self.data.dim[2]
                )
            ),
            "SEND_CONDITION": self.send_condition,
            "RECEIVE_AND_MERGE_CONDITION": self.receive_and_merge_condition,
            "CAN_SEND": self.can_send,
            "CAN_RECEIVE": self.can_receive,
            "INCLUDES": self._create_includes(),
            "DATA_ASSOCIATION": int(self.data.association),
            "MERGE_METHOD_DEFINITION": self._create_merge_method_definition(),
            "FLOAT_TYPE": self.float_type.cpp,
        }
        return templateData

    def construct_output(self, output):
        templateData = self._get_dictionary_for_output()
        output.makefile.add_cpp_file(
            self.data.namespace[-1] + "/" + self.data.name + ".cpp",
            generated=True,
        )
        headerfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Data.template.h"
        )
        cppfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Data.template.cpp"
        )
        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headerfile_template,
            cppfile_template,
            self.data.name,
            self.data.namespace,
            self.data.namespace[-1],
            templateData,
        )
        output.add(generated_files)
