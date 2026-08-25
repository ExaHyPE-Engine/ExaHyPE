# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Cell and face label attributes for the subcell limiter.

``exahype2::dg::LimiterStatus`` owns the status ordering and merge. Each
adjacent cell writes a different face side after the preceding face event has
completed, so accessors rely on side ownership and traversal order.
"""
import peano4.datamodel

from peano4.datamodel.MetaDataAttribute import DoubleArray, Integer, IntegerArray


_STATUS_INCLUDE = """
#include "exahype2/dg/LimiterStatus.h"
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


class _FaceDmpExtremum(DoubleArray):
    """Stores one previous-solution observable extremum per face side.

    Each cell writes its own side in the predictor sweep, and the flagging reads
    the neighbour side after the exchange.
    """

    def __init__(self, name, number_of_observables):
        super().__init__(name, 2 * number_of_observables)
        self._number_of_observables = number_of_observables

    def get_public_declarations(self):
        return super().get_public_declarations() + [
            f"""  INLINE_METHOD double get{self.accessor_name}(const int side, const int observable) const {{
    return {self.field_name}(side * {self._number_of_observables} + observable);
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(const int side, const int observable, const double value) {{
    {self.field_name}(side * {self._number_of_observables} + observable) = value;
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


def add_limiter_attributes_to_face_label(label, number_of_dmp_observables=0):
    """Adds both face sides' live and snapshot limiter status to a face label.

    The snapshot is a frozen copy of the previous spreading ring's status. Within a
    ring a cell reads the snapshot and writes the live status, so it sees the same
    neighbour statuses whatever the traversal order or mesh decomposition. Only
    the live status is exchanged; the face merge for it is assembled by the
    solver, which alone knows the base face-label fields, and
    :func:`face_status_merge_snippet` supplies its status part. A positive
    ``number_of_dmp_observables`` adds both sides' previous-solution observable
    extrema, which the flagging reads from its neighbours.
    """
    _require_attributes_absent(
        label,
        ("limiterStatus", "limiterStatusSnapshot", "dmpMinimum", "dmpMaximum"),
    )
    label.add_attribute(_FaceLimiterStatus("limiterStatus"))
    label.add_attribute(_FaceLimiterStatus("limiterStatusSnapshot"))
    if number_of_dmp_observables > 0:
        label.add_attribute(_FaceDmpExtremum("dmpMinimum", number_of_dmp_observables))
        label.add_attribute(_FaceDmpExtremum("dmpMaximum", number_of_dmp_observables))
    label.includes += _STATUS_INCLUDE
    return label


def face_status_merge_snippet(number_of_dmp_observables=0):
    """Returns the face-merge lines that copy the neighbour-owned status side.

    Only the neighbour side is copied, chosen from the geometric outer normal, so
    the two sides stay independent and the merged status is the same for any mesh
    decomposition. The solver splices this next to its other per-side face fields.
    A positive ``number_of_dmp_observables`` also copies the neighbour side's
    observable extrema.
    """
    result = """
      setLimiterStatus(neighbourEntry, neighbour.getLimiterStatus(neighbourEntry));
"""
    if number_of_dmp_observables > 0:
        result += f"""      for (int observable = 0; observable < {number_of_dmp_observables}; observable++) {{
        setDmpMinimum(neighbourEntry, observable, neighbour.getDmpMinimum(neighbourEntry, observable));
        setDmpMaximum(neighbourEntry, observable, neighbour.getDmpMaximum(neighbourEntry, observable));
      }}
"""
    return result


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


def reset_face_dmp_extrema_snippet(solver_name, number_of_observables):
    """Returns the C++ that seeds both sides of a face's extrema with infinities.

    The face touch runs before either adjacent cell writes its side, so a side no
    cell writes keeps the infinities: a domain boundary, or a rank boundary until
    the exchange copies the neighbour's side. A merge with an infinity leaves a
    reference range unchanged.
    """
    return f"""
    for (int faceSide = 0; faceSide < 2; faceSide++) {{
      for (int observable = 0; observable < {number_of_observables}; observable++) {{
        fineGridFace{solver_name}FaceLabel.setDmpMinimum(faceSide, observable, std::numeric_limits<double>::infinity());
        fineGridFace{solver_name}FaceLabel.setDmpMaximum(faceSide, observable, -std::numeric_limits<double>::infinity());
      }}
    }}
"""


def cell_dmp_extrema_to_face_snippet(solver_name, number_of_observables, minimum, maximum):
    """Returns the C++ that writes a cell's observable extrema onto its face sides.

    ``minimum`` and ``maximum`` name arrays of ``number_of_observables`` values.
    The cell owns the right side of its lower faces and the left side of its
    upper faces, as :func:`cell_status_to_face_snippet` does.
    """
    return f"""
    for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {{
      const auto ownSide = faceNumber < DIMENSIONS ? 1 : 0;
      for (int observable = 0; observable < {number_of_observables}; observable++) {{
        fineGridFaces{solver_name}FaceLabel(faceNumber).setDmpMinimum(ownSide, observable, static_cast<double>({minimum}[observable]));
        fineGridFaces{solver_name}FaceLabel(faceNumber).setDmpMaximum(ownSide, observable, static_cast<double>({maximum}[observable]));
      }}
    }}
"""


def face_neighbour_dmp_extrema_merge_snippet(
    solver_name, number_of_observables, minimum, maximum, precision
):
    """Returns the C++ that merges the face neighbours' extrema into a cell's range.

    ``minimum`` and ``maximum`` name arrays of ``number_of_observables`` values of
    type ``precision``. The neighbour side of a face without a neighbour holds
    infinities, which leave the range unchanged.
    """
    return f"""
    for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {{
      const auto  neighbourSide = faceNumber < DIMENSIONS ? 0 : 1;
      const auto& faceLabel     = fineGridFaces{solver_name}FaceLabel(faceNumber);
      for (int observable = 0; observable < {number_of_observables}; observable++) {{
        {minimum}[observable] = ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(
          {minimum}[observable],
          static_cast<{precision}>(faceLabel.getDmpMinimum(neighbourSide, observable))
        );
        {maximum}[observable] = ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(
          {maximum}[observable],
          static_cast<{precision}>(faceLabel.getDmpMaximum(neighbourSide, observable))
        );
      }}
    }}
"""


def hanging_face_dmp_extrema_from_coarse_snippet(solver_name, number_of_observables):
    """Returns the C++ that copies the coarse face's extrema onto a hanging face.

    A hanging face keeps whatever its storage slot last held, so the fine cell
    reads its coarse neighbour's extrema from the coarse face's copy.
    """
    return f"""
    const auto& coarseFaceLabel = coarseGridFaces{solver_name}FaceLabel(marker.getSelectedFaceNumber());
    for (int faceSide = 0; faceSide < 2; faceSide++) {{
      for (int observable = 0; observable < {number_of_observables}; observable++) {{
        fineGridFace{solver_name}FaceLabel.setDmpMinimum(faceSide, observable, coarseFaceLabel.getDmpMinimum(faceSide, observable));
        fineGridFace{solver_name}FaceLabel.setDmpMaximum(faceSide, observable, coarseFaceLabel.getDmpMaximum(faceSide, observable));
      }}
    }}
"""


def hanging_face_dmp_extrema_reduce_snippet(solver_name, number_of_observables):
    """Returns the C++ that reduces a hanging face's fine-cell extrema upwards.

    By the time a hanging face is left, its adjacent fine cell has written its
    extrema onto its side, and the minimum and maximum over the subfaces land on
    the coarse face's refined side, which the face touch seeded with infinities.
    A minimum and a maximum are the same for every subface order, so the
    reduction does not depend on the traversal or the decomposition.
    """
    return f"""
    const auto reducedSide     = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    auto&      coarseFaceLabel = coarseGridFaces{solver_name}FaceLabel(marker.getSelectedFaceNumber());
    for (int observable = 0; observable < {number_of_observables}; observable++) {{
      coarseFaceLabel.setDmpMinimum(
        reducedSide,
        observable,
        ::exahype2::dg::mergeDiscreteMaximumPrincipleMinimum(
          coarseFaceLabel.getDmpMinimum(reducedSide, observable),
          fineGridFace{solver_name}FaceLabel.getDmpMinimum(reducedSide, observable)
        )
      );
      coarseFaceLabel.setDmpMaximum(
        reducedSide,
        observable,
        ::exahype2::dg::mergeDiscreteMaximumPrincipleMaximum(
          coarseFaceLabel.getDmpMaximum(reducedSide, observable),
          fineGridFace{solver_name}FaceLabel.getDmpMaximum(reducedSide, observable)
        )
      );
    }}
"""


def _require_attributes_absent(label, names):
    existing_names = {attribute.name for attribute in label.attributes}
    duplicate_names = existing_names.intersection(names)
    if duplicate_names:
        duplicates = ", ".join(sorted(duplicate_names))
        raise ValueError(f"Limiter label attributes already exist: {duplicates}.")
