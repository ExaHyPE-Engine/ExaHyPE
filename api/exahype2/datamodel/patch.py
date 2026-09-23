# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen

from exahype2.precision import cpp_type

from .dof import DoF
from .dof import DoFAssociation


class PatchGenerator:
    """Writes the data class of a patch and holds its exchange conditions.

    A boundary or periodic exchange copies the inbound halo half from the exchanged snapshot;
    ``halo_layout`` selects overlapping volume layers or contiguous projection halves and
    ``None`` disables the merge.
    """

    def __init__(self, patch, float_type="double"):
        self.data = patch
        self.halo_layout = None
        self.send_condition = "true"
        self.receive_and_merge_condition = "true"
        self.can_send = "true"
        self.can_receive = "true"
        self.includes = []
        self.float_type = cpp_type(float_type)

    def __str__(self):
        return (
            "Map to data\n  - Send condition:          "
            + self.send_condition
            + "\n  - Receive/merge condition: "
            + self.receive_and_merge_condition
            + "\n  - Halo layout:             "
            + (self.halo_layout if self.halo_layout is not None else "none")
            + "\n"
        )

    def _create_merge_method_definition(self):
        if self.halo_layout is None:
            return ""
        if self.halo_layout == "projection_halves":
            return "::exahype2::mergeProjectionHalves<Cardinality>(marker, this->data(), neighbour.data());"
        if self.halo_layout == "overlapping_volumes":
            overlap_cells = int(self.data.dim[0])
            if overlap_cells <= 0 or overlap_cells % 2 != 0:
                raise ValueError("The overlap direction holds both adjacent patches.")
            return "::exahype2::mergeOverlappingVolumes<{}, {}, {}>(marker, this->data(), neighbour.data());".format(
                int(self.data.dim[1]), overlap_cells // 2, int(self.data.no_of_unknowns)
            )
        raise ValueError("Unknown halo layout " + str(self.halo_layout))

    def get_stack_container(self):
        return "::peano4::grid::Stack<" + self.data.get_full_qualified_type() + ">"

    def _get_dictionary_for_output(self):
        assert self.data.association in (
            DoFAssociation.Face,
            DoFAssociation.Cell,
        ), "Patch data is face- or cell-associated."
        return {
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
            "INCLUDES": self.includes,
            "DATA_ASSOCIATION": int(self.data.association),
            "MERGE_METHOD_DEFINITION": self._create_merge_method_definition(),
            "FLOAT_TYPE": self.float_type,
        }

    def construct_output(self, output):
        output.makefile.add_cpp_file(
            self.data.namespace[-1] + "/" + self.data.name + ".cpp",
            generated=True,
        )
        template_directory = os.path.dirname(os.path.realpath(__file__))
        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "Data.h.jinja2"),
                os.path.join(template_directory, "Data.cpp.jinja2"),
                self.data.name,
                self.data.namespace,
                self.data.namespace[-1],
                self._get_dictionary_for_output(),
            )
        )


class Patch(DoF):
    """A block of ``dim`` values per axis with ``no_of_unknowns`` entries each on a cell or
    face; ``dim`` is a three-tuple whose last entry the 2D build drops."""

    def __init__(self, dim, no_of_unknowns, name, float_type="double"):
        super().__init__(name)
        self.dim = dim
        self.no_of_unknowns = no_of_unknowns
        self.generator = PatchGenerator(self, float_type)

    def __str__(self):
        return (
            "\nDim:       "
            + str(self.dim)
            + "\nUnknowns:  "
            + str(self.no_of_unknowns)
            + "\nGenerator: "
            + str(self.generator)
            + "\n"
        )
