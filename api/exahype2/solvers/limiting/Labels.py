# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Cell, face and vertex label attributes for the subcell limiter.

``exahype2::dg::LimiterStatus`` owns the status ordering and merge. Each
adjacent cell writes a different face side after the preceding face event has
completed, so accessors rely on side ownership and traversal order.
"""
import peano4.datamodel

from peano4.datamodel.MetaDataAttribute import DoubleArray, Integer, IntegerArray


_STATUS_INCLUDE = """
#include "exahype2/dg/LimiterStatus.h"
"""

_LIMITER_INCLUDE = """
#include "exahype2/dg/Limiter.h"
"""

_STATUS_ENUM = "::exahype2::dg::LimiterStatus"


class _CellLimiterStatus(Integer):
    """Stores a cell's limiter ownership as ``exahype2::dg::LimiterStatus``."""

    def get_public_declarations(self):
        return [
            f"""  INLINE_METHOD {_STATUS_ENUM} get{self.accessor_name}() const {{
    return static_cast<{_STATUS_ENUM}>({self.field_name});
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(const {_STATUS_ENUM} value) {{
    {self.field_name} = static_cast<int>(value);
  }}""",
        ]

    def get_to_string_expression(self):
        # The enum has no stream operator, so the label prints the numeric status.
        return f"static_cast<int>(get{self.accessor_name}())"


class _FaceLimiterStatus(IntegerArray):
    """Stores both face sides' independently owned limiter status.

    The preceding face event initialises both sides before either adjacent cell
    accesses them, and each cell writes only its own side.
    """

    def __init__(self, name):
        super().__init__(name, 2)

    def get_public_declarations(self):
        vector_type = "tarch::la::Vector<2, int>"
        return [
            f"""  INLINE_METHOD {vector_type} get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD {_STATUS_ENUM} get{self.accessor_name}(const int side) const {{
    return ::exahype2::dg::loadFaceStatus(&{self.field_name}(0), side);
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(const int side, const {_STATUS_ENUM} value) {{
    ::exahype2::dg::storeFaceStatus(&{self.field_name}(0), side, value);
  }}""",
            f"""  INLINE_METHOD void raise{self.accessor_name}(const int side, const {_STATUS_ENUM} value) {{
    ::exahype2::dg::raiseFaceStatus(&{self.field_name}(0), side, value);
  }}""",
        ]


_CELL_MERGE = """
  switch (context) {
  case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
    assertion(false, "Cell labels are never exchanged across a boundary.");
    break;
  case ::peano4::stacks::SendReceiveContext::ForkDomain:
    *this = neighbour;
    break;
  default:
    assertion(false, "The cell-label merge context is unsupported.");
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
        peano4.datamodel.Boolean("wasTroubledInPreviousStep", initval="false")
    )
    label.add_attribute(peano4.datamodel.Boolean("predictorDiverged", initval="false"))
    label.includes += _STATUS_INCLUDE
    label.merge_method_definition = _CELL_MERGE
    return label


def add_limiter_attributes_to_face_label(label):
    """Adds both face sides' live and snapshot limiter status to a face label.

    The snapshot is a frozen copy of the previous spreading ring's status. Within a
    ring a cell reads the snapshot and writes the live status, so it sees the same
    neighbour statuses whatever the traversal order or mesh decomposition. Only
    the live status is exchanged; the face merge for it is assembled by the
    solver, which alone knows the base face-label fields, and
    :func:`face_status_merge_snippet` supplies its status part.
    """
    _require_attributes_absent(label, ("limiterStatus", "limiterStatusSnapshot"))
    label.add_attribute(_FaceLimiterStatus("limiterStatus"))
    label.add_attribute(_FaceLimiterStatus("limiterStatusSnapshot"))
    label.includes += _STATUS_INCLUDE
    return label


def face_status_merge_snippet():
    """Returns the face-merge lines that copy the neighbour-owned status side.

    Only the neighbour side is copied, chosen from the geometric outer normal, so
    the two sides stay independent and the merged status is the same for any mesh
    decomposition. The solver splices this next to its other per-side face fields.
    """
    return """
      setLimiterStatus(neighbourEntry, neighbour.getLimiterStatus(neighbourEntry));
"""


def create_limiter_vertex_label(solver_name, number_of_dmp_observables):
    """Creates the vertex label that carries the previous-solution extrema.

    A vertex holds the minimum and maximum of every observable over the accepted
    solutions of its adjacent cells, so a cell reads the range of the cells that
    share a node with it off its vertices. The predictor sweep seeds each vertex
    with infinities and reduces the adjacent cells into it. The merge reduces the
    copies of a vertex on a tree boundary with the same minimum and maximum,
    which are the same for every order of the copies.
    """
    label = peano4.datamodel.MetaData(solver_name + "VertexLabel")
    label.add_attribute(DoubleArray("dmpMinimum", number_of_dmp_observables))
    label.add_attribute(DoubleArray("dmpMaximum", number_of_dmp_observables))
    label.includes += _LIMITER_INCLUDE
    label.merge_method_definition = _vertex_extrema_merge(number_of_dmp_observables)
    return label


def _vertex_extrema_merge(number_of_observables):
    return f"""
  switch (context) {{
  case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
  case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
    for (int observable = 0; observable < {number_of_observables}; observable++) {{
      setDmpMinimum(
        observable,
        ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(getDmpMinimum(observable), neighbour.getDmpMinimum(observable))
      );
      setDmpMaximum(
        observable,
        ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(getDmpMaximum(observable), neighbour.getDmpMaximum(observable))
      );
    }}
    break;
  case ::peano4::stacks::SendReceiveContext::ForkDomain:
    *this = neighbour;
    break;
  default:
    assertion(false, "The vertex-label merge context is unsupported.");
  }}
"""


def cell_status_to_face_snippet(solver_name):
    """Returns the C++ that projects a cell's limiter status onto its faces.

    A cell owns one side of each adjacent face, the lower faces on their right and
    the upper faces on their left. Writing the cell status there is the single place
    the per-side projection is formed, so the flagging and the spreading sweeps
    share this projection.
    """
    return f"""
    const auto limiterStatus = fineGridCell{solver_name}CellLabel.getLimiterStatus();
    for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {{
      const auto ownSide = faceNumber < DIMENSIONS ? 1 : 0;
      fineGridFaces{solver_name}FaceLabel(faceNumber).setLimiterStatus(ownSide, limiterStatus);
    }}
"""


def hanging_face_status_from_coarse_snippet(solver_name):
    """Returns the C++ that derives a hanging face's limiter status.

    A hanging face keeps whatever its storage slot last held, so both its live
    and its snapshot status are derived on first touch from the coarse face's
    snapshot. The spreading then reads the coarse neighbour's previous-ring
    status across the resolution transition, whatever the traversal order, and
    the adjacent fine cell overwrites its own side afterwards.
    """
    return f"""
    for (int faceSide = 0; faceSide < 2; faceSide++) {{
      const auto coarseStatus =
        coarseGridFaces{solver_name}FaceLabel(marker.getSelectedFaceNumber()).getLimiterStatusSnapshot(faceSide);
      fineGridFace{solver_name}FaceLabel.setLimiterStatus(faceSide, coarseStatus);
      fineGridFace{solver_name}FaceLabel.setLimiterStatusSnapshot(faceSide, coarseStatus);
    }}
"""


def hanging_face_status_reduce_snippet(solver_name):
    """Returns the C++ that reduces a hanging face's fine-cell status upwards.

    The refined side of a face above hanging faces has no adjacent cell left to
    write it. By the time a hanging face is left, its adjacent fine cell has
    written its status onto its side, and the maximum over the subfaces lands on
    the coarse face's refined side. A maximum is the same for every subface
    order, so the reduction does not depend on the traversal or the
    decomposition.
    """
    return f"""
    const auto refinedSide = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    coarseGridFaces{solver_name}FaceLabel(marker.getSelectedFaceNumber())
      .raiseLimiterStatus(refinedSide, fineGridFace{solver_name}FaceLabel.getLimiterStatus(refinedSide));
"""


def reset_refined_face_status_snippet(solver_name):
    """Returns the C++ that restarts the reduction on a face's refined sides.

    The hanging subfaces raise the fine statuses onto a refined side while the
    sweep descends, and the reduction restarts from Regular each sweep, so a
    raised status can drop again. The face touch runs before the subtree below
    is entered, so the reset precedes every reduction of the sweep.
    """
    return f"""
    for (int faceSide = 0; faceSide < 2; faceSide++) {{
      if (fineGridFace{solver_name}FaceLabel.getAboveHanging(faceSide)) {{
        fineGridFace{solver_name}FaceLabel.setLimiterStatus(faceSide, ::exahype2::dg::LimiterStatus::Regular);
      }}
    }}
"""


def reset_vertex_dmp_extrema_snippet(solver_name, number_of_observables):
    """Returns the C++ that seeds a vertex's extrema with infinities.

    The vertex touch runs before any adjacent cell reduces into the vertex, so a
    vertex no cell writes keeps the infinities, which leave a reference range
    unchanged.
    """
    return f"""
    for (int observable = 0; observable < {number_of_observables}; observable++) {{
      fineGridVertex{solver_name}VertexLabel.setDmpMinimum(observable, std::numeric_limits<double>::infinity());
      fineGridVertex{solver_name}VertexLabel.setDmpMaximum(observable, -std::numeric_limits<double>::infinity());
    }}
"""


def cell_dmp_extrema_to_vertices_snippet(solver_name, number_of_observables, minimum, maximum):
    """Returns the C++ that reduces a cell's observable extrema into its vertices.

    ``minimum`` and ``maximum`` name arrays of ``number_of_observables`` values.
    A minimum and a maximum are the same for every order of the adjacent cells,
    so the vertex does not depend on the traversal or the decomposition.
    """
    return f"""
    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {{
      auto& vertexLabel = fineGridVertices{solver_name}VertexLabel(vertexNumber);
      for (int observable = 0; observable < {number_of_observables}; observable++) {{
        vertexLabel.setDmpMinimum(
          observable,
          ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(
            vertexLabel.getDmpMinimum(observable), static_cast<double>({minimum}[observable])
          )
        );
        vertexLabel.setDmpMaximum(
          observable,
          ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(
            vertexLabel.getDmpMaximum(observable), static_cast<double>({maximum}[observable])
          )
        );
      }}
    }}
"""


def vertex_dmp_extrema_merge_snippet(
    solver_name, number_of_observables, minimum, maximum, precision
):
    """Returns the C++ that merges a cell's vertices' extrema into its range.

    ``minimum`` and ``maximum`` name arrays of ``number_of_observables`` values of
    type ``precision``. A vertex holds the range of every cell adjacent to it, so
    the merged range covers every cell that shares a node with the cell. A vertex
    no cell wrote holds infinities, which leave the range unchanged.
    """
    return f"""
    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {{
      const auto& vertexLabel = fineGridVertices{solver_name}VertexLabel(vertexNumber);
      for (int observable = 0; observable < {number_of_observables}; observable++) {{
        {minimum}[observable] = ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(
          {minimum}[observable],
          static_cast<{precision}>(vertexLabel.getDmpMinimum(observable))
        );
        {maximum}[observable] = ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(
          {maximum}[observable],
          static_cast<{precision}>(vertexLabel.getDmpMaximum(observable))
        );
      }}
    }}
"""


def hanging_vertex_dmp_extrema_from_coarse_snippet(solver_name, number_of_observables):
    """Returns the C++ that derives a hanging vertex's extrema from the coarse vertices.

    A hanging vertex keeps whatever its storage slot last held, so its extrema
    are seeded with infinities and reduced over the coarse vertices on the facet
    of the father cell that contains it. Those coarse vertices hold the range of
    the coarse cells adjacent to the facet and, from the predictor sweep's
    reduction, of the fine cells below it.
    """
    return reset_vertex_dmp_extrema_snippet(solver_name, number_of_observables) + f"""
    const auto hangingVertexPosition = marker.getRelativePositionWithinFatherCell();
    for (int coarseVertexNumber = 0; coarseVertexNumber < TWO_POWER_D; coarseVertexNumber++) {{
      if (::exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(hangingVertexPosition, coarseVertexNumber)) {{
        const auto& coarseVertexLabel = coarseGridVertices{solver_name}VertexLabel(coarseVertexNumber);
        for (int observable = 0; observable < {number_of_observables}; observable++) {{
          fineGridVertex{solver_name}VertexLabel.setDmpMinimum(
            observable,
            ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(
              fineGridVertex{solver_name}VertexLabel.getDmpMinimum(observable),
              coarseVertexLabel.getDmpMinimum(observable)
            )
          );
          fineGridVertex{solver_name}VertexLabel.setDmpMaximum(
            observable,
            ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(
              fineGridVertex{solver_name}VertexLabel.getDmpMaximum(observable),
              coarseVertexLabel.getDmpMaximum(observable)
            )
          );
        }}
      }}
    }}
"""


def hanging_vertex_dmp_extrema_reduce_snippet(solver_name, number_of_observables):
    """Returns the C++ that reduces a hanging vertex's extrema into the coarse vertices.

    By the time a hanging vertex is left, its adjacent fine cells have reduced
    their extrema into it. The minimum and maximum land on the coarse vertices on
    the facet of the father cell that contains the hanging vertex, which the
    vertex touch seeded with infinities. A minimum and a maximum are the same for
    every order, so the reduction does not depend on the traversal or the
    decomposition.
    """
    return f"""
    const auto hangingVertexPosition = marker.getRelativePositionWithinFatherCell();
    for (int coarseVertexNumber = 0; coarseVertexNumber < TWO_POWER_D; coarseVertexNumber++) {{
      if (::exahype2::dg::isCoarseVertexOnFacetOfHangingVertex(hangingVertexPosition, coarseVertexNumber)) {{
        auto& coarseVertexLabel = coarseGridVertices{solver_name}VertexLabel(coarseVertexNumber);
        for (int observable = 0; observable < {number_of_observables}; observable++) {{
          coarseVertexLabel.setDmpMinimum(
            observable,
            ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(
              coarseVertexLabel.getDmpMinimum(observable),
              fineGridVertex{solver_name}VertexLabel.getDmpMinimum(observable)
            )
          );
          coarseVertexLabel.setDmpMaximum(
            observable,
            ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(
              coarseVertexLabel.getDmpMaximum(observable),
              fineGridVertex{solver_name}VertexLabel.getDmpMaximum(observable)
            )
          );
        }}
      }}
    }}
"""


def _require_attributes_absent(label, names):
    existing_names = {attribute.name for attribute in label.attributes}
    duplicate_names = existing_names.intersection(names)
    if duplicate_names:
        duplicates = ", ".join(sorted(duplicate_names))
        raise ValueError(f"Limiter label attributes already exist: {duplicates}.")
