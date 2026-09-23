# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.datamodel

from exahype2.datamodel.metadata_attribute import DoubleArray
from exahype2.datamodel.metadata_attribute import Integer
from exahype2.datamodel.metadata_attribute import IntegerArray

_STATUS_ENUM = "::exahype2::dg::LimiterStatus"


class _CellLimiterStatus(Integer):
    def get_method_declarations(self):
        return [
            f"  INLINE_METHOD {_STATUS_ENUM} get{self.accessor_name}() const;",
            f"  INLINE_METHOD void set{self.accessor_name}({_STATUS_ENUM} value);",
        ]

    def get_method_definitions(self, qualified_class_name):
        return f"""INLINE_METHOD {_STATUS_ENUM} {qualified_class_name}::get{self.accessor_name}() const {{
  return static_cast<{_STATUS_ENUM}>({self.field_name});
}}

INLINE_METHOD void {qualified_class_name}::set{self.accessor_name}(const {_STATUS_ENUM} value) {{
  {self.field_name} = static_cast<int>(value);
}}"""

    def get_to_string_expression(self):
        return f"static_cast<int>(get{self.accessor_name}())"


class _FaceLimiterStatus(IntegerArray):
    """The preceding face event initialises both sides before either adjacent
    cell accesses them, and each cell writes only its own side.
    """

    def __init__(self, name):
        super().__init__(name, 2)

    def get_method_declarations(self):
        vector_type = "tarch::la::Vector<2, int>"
        return [
            f"  INLINE_METHOD {vector_type} get{self.accessor_name}() const;",
            f"  INLINE_METHOD {_STATUS_ENUM} get{self.accessor_name}(int side) const;",
            f"  INLINE_METHOD void set{self.accessor_name}(int side, {_STATUS_ENUM} value);",
            f"  INLINE_METHOD void raise{self.accessor_name}(int side, {_STATUS_ENUM} value);",
        ]

    def get_method_definitions(self, qualified_class_name):
        vector_type = "tarch::la::Vector<2, int>"
        return f"""INLINE_METHOD {vector_type} {qualified_class_name}::get{self.accessor_name}() const {{
  return {self.field_name};
}}

INLINE_METHOD {_STATUS_ENUM} {qualified_class_name}::get{self.accessor_name}(const int side) const {{
  return ::exahype2::dg::loadFaceStatus(&{self.field_name}(0), side);
}}

INLINE_METHOD void {qualified_class_name}::set{self.accessor_name}(const int side, const {_STATUS_ENUM} value) {{
  ::exahype2::dg::storeFaceStatus(&{self.field_name}(0), side, value);
}}

INLINE_METHOD void {qualified_class_name}::raise{self.accessor_name}(const int side, const {_STATUS_ENUM} value) {{
  ::exahype2::dg::raiseFaceStatus(&{self.field_name}(0), side, value);
}}"""


_CELL_MERGE = """
  switch (context) {
  case ::peano4::grid::SendReceiveContext::InterRank:
  case ::peano4::grid::SendReceiveContext::IntraRank:
  case ::peano4::grid::SendReceiveContext::Consolidation:
    assertion(false, "Cell labels are never merged.");
    break;
  }
"""


def add_limiter_attributes_to_cell_label(label):
    """Adds a cell's limiter ownership and its troubled history to a cell label."""
    _require_attributes_absent(
        label,
        (
            "limiterStatus",
            "previousLimiterStatus",
            "wasTroubledInPreviousStep",
            "predictorDiverged",
        ),
    )
    label.add_attribute(_CellLimiterStatus("limiterStatus"))
    label.add_attribute(_CellLimiterStatus("previousLimiterStatus"))
    label.add_attribute(
        exahype2.datamodel.Boolean("wasTroubledInPreviousStep", initval="false")
    )
    label.add_attribute(
        exahype2.datamodel.Boolean("predictorDiverged", initval="false")
    )
    label.includes.append("exahype2/dg/LimiterStatus.h")
    label.merge_method_definition = _CELL_MERGE
    return label


def add_limiter_attributes_to_face_label(label):
    """Adds both face sides' live and snapshot limiter status to a face label.

    The snapshot holds the previous spreading ring's status, and a cell reads the
    snapshot and writes the live status.
    """
    _require_attributes_absent(label, ("limiterStatus", "limiterStatusSnapshot"))
    label.add_attribute(_FaceLimiterStatus("limiterStatus"))
    label.add_attribute(_FaceLimiterStatus("limiterStatusSnapshot"))
    label.includes.append("exahype2/dg/LimiterStatus.h")
    return label


def face_status_merge_snippet():
    """Returns the face-merge lines that copy the neighbour-owned status side."""
    return """
    setLimiterStatus(neighbourEntry, neighbour.getLimiterStatus(neighbourEntry));
"""


def create_limiter_vertex_label(solver_name, number_of_dmp_observables):
    """Creates the vertex label that carries the previous-solution extrema.

    The predictor sweep seeds each vertex with infinities and reduces the accepted
    solutions of the adjacent cells into it.
    """
    label = exahype2.datamodel.MetaData(solver_name + "VertexLabel")
    label.add_attribute(DoubleArray("dmpMinimum", number_of_dmp_observables))
    label.add_attribute(DoubleArray("dmpMaximum", number_of_dmp_observables))
    label.includes.append("exahype2/dg/limiting/VertexExtrema.h")
    label.merge_method_definition = _vertex_extrema_merge(number_of_dmp_observables)
    return label


def _vertex_extrema_merge(number_of_observables):
    """Returns the merge call that reduces two copies of one vertex into its extrema."""
    return f"""
  ::exahype2::dg::limiting::mergeVertexObservableExtrema<{number_of_observables}>(context, *this, neighbour);
"""


def _require_attributes_absent(label, names):
    existing_names = {attribute.name for attribute in label.attributes}
    duplicate_names = existing_names.intersection(names)
    if duplicate_names:
        duplicates = ", ".join(sorted(duplicate_names))
        raise ValueError(f"Limiter label attributes already exist: {duplicates}.")
