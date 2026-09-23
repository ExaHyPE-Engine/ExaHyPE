// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * A solver family supplies four kernels, and the routines of this file call them in the
 * traversal events a level jump produces. The concepts of this file name the signature of each
 * kernel, where T is the type the data is stored in.
 */
namespace exahype2::amr {
  //! Fills one side of a hanging face from the coarse face above it.
  template <typename Kernel, typename T>
  concept FaceInterpolationKernel = requires(
    const Kernel&                       kernel,
    const peano4::dataview::FaceMarker& marker,
    const T*                            coarseFaceValues,
    T*                                  fineFaceValues,
    int                                 side
  ) { kernel(marker, coarseFaceValues, fineFaceValues, side); };

  //! Reduces every subface staged below one side of a face and discards the frame.
  template <typename Kernel, typename T>
  concept FaceRestrictionKernel = requires(
    const Kernel&                     kernel,
    exahype2::amr::ChildDataStage<T>& stage,
    const void*                       coarseFaceKey,
    T*                                coarseFaceValues
  ) { kernel(stage, coarseFaceKey, coarseFaceValues); };

  //! Fills one child of a refined cell from the cell.
  template <typename Kernel, typename T>
  concept CellProlongationKernel = requires(
    const Kernel&                       kernel,
    const peano4::dataview::CellMarker& marker,
    const T*                            coarseGridValues,
    T*                                  fineGridValues
  ) { kernel(marker, coarseGridValues, fineGridValues); };

  //! Reduces every child staged below a cell and discards the frame.
  template <typename Kernel, typename T>
  concept CellRestrictionKernel =
    requires(const Kernel& kernel, exahype2::amr::ChildDataStage<T>& stage, T* coarseGridValues) {
      kernel(stage, coarseGridValues);
    };

  template <typename T>
  class CellAveragingInterpolation;

  template <typename T>
  class CellAveragingRestriction;

  constexpr int SubfacesPerFace = peano4::power(peano4::sfc::ActiveCurve::SubdivisionFactor, DIMENSIONS - 1);

  INLINE_METHOD int subfaceIndexWithinFatherFace(const peano4::dataview::FaceMarker& marker);

  INLINE_METHOD int childIndexWithinFatherCell(const peano4::dataview::CellMarker& marker);

  INLINE_METHOD bool isOnDomainBoundary(
    const peano4::dataview::FaceMarker&          marker,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize
  );
} // namespace exahype2::amr

TARGET_DECLARE
namespace exahype2::amr {
  template <typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void clearTransferBuffer(T* NOALIAS values, int numberOfValues);
} // namespace exahype2::amr
TARGET_DECLARE_END

namespace exahype2::amr {
  template <typename Record>
  INLINE_METHOD void clearTransferRecord(Record& record);

  template <typename T, FaceInterpolationKernel<T> FaceInterpolation>
  INLINE_METHOD void interpolateFaceFromCoarseFace(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseFaceValues,
    T* NOALIAS                          fineFaceValues,
    int                                 side,
    const FaceInterpolation&            interpolateFace
  );

  template <typename T>
  INLINE_METHOD bool stageSubfaceForRestriction(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    const void*                         coarseFaceKey,
    const T* NOALIAS                    fineFaceValues
  );

  template <typename T, FaceRestrictionKernel<T> FaceRestriction>
  INLINE_METHOD bool restrictFaceFromStagedSubfaces(
    exahype2::amr::ChildDataStage<T>& stage,
    const void*                       coarseFaceKey,
    T*                                coarseFaceValues,
    const FaceRestriction&            restrictFace
  );

  template <typename T>
  INLINE_METHOD void collectStagedSubfaces(
    const exahype2::amr::ChildDataStage<T>& stage,
    const void*                             coarseFaceKey,
    const T**                               stagedSubfaces
  );

  template <typename T, CellProlongationKernel<T> CellProlongation>
  INLINE_METHOD void prolongCellIntoChild(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseGridValues,
    T* NOALIAS                          fineGridValues,
    const CellProlongation&             prolongCell
  );

  template <typename T>
  INLINE_METHOD bool stageChildForRestriction(
    const peano4::dataview::CellMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    T* NOALIAS                          coarseGridValues,
    const T* NOALIAS                    fineGridValues
  );

  template <typename T, CellRestrictionKernel<T> CellRestriction>
  INLINE_METHOD void restrictStagedChildrenIntoCell(
    exahype2::amr::ChildDataStage<T>& stage,
    T* NOALIAS                        coarseGridValues,
    const CellRestriction&            restrictCell
  );

  template <typename T, CellRestrictionKernel<T> CellRestriction>
  INLINE_METHOD bool restrictChildrenIntoCell(
    const peano4::dataview::CellMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    T* NOALIAS                          coarseGridValues,
    const T* NOALIAS                    fineGridValues,
    const CellRestriction&              restrictCell
  );

  template <typename T>
  INLINE_METHOD void collectStagedChildren(
    const exahype2::amr::ChildDataStage<T>& stage,
    const void*                             coarseCellKey,
    const T**                               stagedChildren
  );

  template <typename FaceLabel>
  INLINE_METHOD void copyFaceTimeStampsFromCoarseFace(
    const FaceLabel& coarseFaceLabel,
    FaceLabel&       fineFaceLabel,
    int              side
  );

  template <typename FaceLabel>
  INLINE_METHOD void raiseFaceTimeStampsFromCoarseFace(const FaceLabel& coarseFaceLabel, FaceLabel& fineFaceLabel);

  template <typename FaceLabel>
  INLINE_METHOD void raiseFaceTimeStampsOntoCoarseFace(FaceLabel& coarseFaceLabel, const FaceLabel& fineFaceLabel);

  template <typename FaceLabel>
  INLINE_METHOD void raiseUpdatedTimeStampOntoCoarseFace(
    FaceLabel&       coarseFaceLabel,
    const FaceLabel& fineFaceLabel,
    int              side
  );

  template <typename FaceLabel>
  INLINE_METHOD void copyUpdatedTimeStampOntoCoarseFace(
    FaceLabel&       coarseFaceLabel,
    const FaceLabel& fineFaceLabel,
    int              side
  );

  template <typename CellLabel, typename FaceLabel>
  INLINE_METHOD void setFaceTimeStampsFromCoarseCell(const CellLabel& coarseCellLabel, FaceLabel& fineFaceLabel);

  template <typename CellLabel>
  INLINE_METHOD void copyCellLabelIntoChild(const CellLabel& coarseCellLabel, CellLabel& fineCellLabel);

  template <typename CellLabel>
  INLINE_METHOD void copyCellTimeStampsFromChild(CellLabel& coarseCellLabel, const CellLabel& fineCellLabel);

  template <typename CellLabel>
  INLINE_METHOD void raiseCellTimeStampsFromChild(CellLabel& coarseCellLabel, const CellLabel& fineCellLabel);

  template <typename CellLabel>
  INLINE_METHOD void mergeCellLabelFromChild(CellLabel& coarseCellLabel, const CellLabel& fineCellLabel);

  template <typename FaceLabelEnumerator>
  INLINE_METHOD void requestDataTransferOnOwnSides(const FaceLabelEnumerator& faceLabels);

  template <typename FaceLabelEnumerator>
  INLINE_METHOD void releaseDataTransferOnOwnSides(const FaceLabelEnumerator& faceLabels);

  template <typename FaceLabelEnumerator>
  INLINE_METHOD void markOwnSidesUpdated(const FaceLabelEnumerator& faceLabels, double timeStamp);

  template <typename FaceLabelEnumerator>
  INLINE_METHOD bool awaitsDataTransfer(
    const peano4::dataview::CellMarker& marker,
    const FaceLabelEnumerator&          faceLabels
  );

  std::vector<peano4::grid::GridControlEvent> gridControlEvents();
} // namespace exahype2::amr

/**
 * Prolongs a cell into one child by copying the covering coarse degree of freedom.
 */
template <typename T>
class exahype2::amr::CellAveragingInterpolation final {
public:
  INLINE_METHOD CellAveragingInterpolation(int numberOfDofsPerAxis, int numberOfValuesPerDof);

  INLINE_METHOD void operator()(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseGridValues,
    T* NOALIAS                          fineGridValues
  ) const;

private:
  const int _numberOfDofsPerAxis{0};
  const int _numberOfValuesPerDof{0};
};

/**
 * Restricts a complete sibling set into a cell by averaging the covering children.
 */
template <typename T>
class exahype2::amr::CellAveragingRestriction final {
public:
  INLINE_METHOD CellAveragingRestriction(int numberOfDofsPerAxis, int numberOfValuesPerDof);

  INLINE_METHOD void operator()(exahype2::amr::ChildDataStage<T>& stage, T* NOALIAS coarseGridValues) const;

private:
  const int _numberOfDofsPerAxis{0};
  const int _numberOfValuesPerDof{0};
};

#include "exahype2/amr/LevelJumpEvents.cpph"
