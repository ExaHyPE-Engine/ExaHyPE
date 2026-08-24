# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Cell and face label attributes for the subcell limiter.

``exahype2::dg::LimiterStatus`` owns the status ordering and merge. Each
adjacent cell writes a different face side after the preceding face event has
completed, so accessors rely on side ownership and traversal order.
"""
import peano4.datamodel

from peano4.datamodel.MetaDataAttribute import Integer, IntegerArray


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
    neighbour statuses whatever the traversal order or mesh decomposition, which is
    what makes the grown finite-volume domain reproducible. Only the live status is
    exchanged; the face merge for it is assembled by the solver, which alone knows
    the base face-label fields, and :func:`face_status_merge_snippet` supplies its
    status part.
    """
    _require_attributes_absent(label, ("limiterStatus", "limiterStatusSnapshot"))
    label.add_attribute(_FaceLimiterStatus("limiterStatus"))
    label.add_attribute(_FaceLimiterStatus("limiterStatusSnapshot"))
    label.includes += _STATUS_INCLUDE
    return label


def face_status_merge_snippet():
    """Returns the face-merge line that copies the neighbour-owned status side.

    Only the neighbour side is copied, chosen from the geometric outer normal, so
    the two sides stay independent and the merged status is the same for any mesh
    decomposition. The solver splices this next to its other per-side face fields.
    """
    return """
      setLimiterStatus(neighbourEntry, neighbour.getLimiterStatus(neighbourEntry));
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


def hanging_face_status_fold_snippet(solver_name):
    """Returns the C++ that folds a hanging face's fine-cell status upwards.

    The refined side of a face above hanging faces has no adjacent cell left to
    write it, so without this fold it would keep the status of the refined-away
    cell. By the time a hanging face is left, its adjacent fine cell has written
    its status onto its side, and the maximum over the subfaces lands on the
    coarse face's refined side. A maximum is the same for every subface order,
    so the fold does not depend on the traversal or the decomposition.
    """
    return f"""
    const auto refinedSide = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    coarseGridFaces{solver_name}FaceLabel(marker.getSelectedFaceNumber())
      .raiseLimiterStatus(refinedSide, fineGridFace{solver_name}FaceLabel.getLimiterStatus(refinedSide));
"""


def reset_refined_face_status_snippet(solver_name):
    """Returns the C++ that restarts the fold on a face's refined sides.

    The hanging subfaces raise the fine statuses onto a refined side while the
    sweep descends, and the fold has to restart from Regular each sweep or a
    raised status could never drop again. The face touch runs before the
    subtree below is entered, so the reset precedes every fold of the sweep.
    """
    return f"""
    for (int faceSide = 0; faceSide < 2; faceSide++) {{
      if (fineGridFace{solver_name}FaceLabel.getAboveHanging(faceSide)) {{
        fineGridFace{solver_name}FaceLabel.setLimiterStatus(faceSide, ::exahype2::dg::LimiterStatus::Regular);
      }}
    }}
"""


def _require_attributes_absent(label, names):
    existing_names = {attribute.name for attribute in label.attributes}
    duplicate_names = existing_names.intersection(names)
    if duplicate_names:
        duplicates = ", ".join(sorted(duplicate_names))
        raise ValueError(f"Limiter label attributes already exist: {duplicates}.")
