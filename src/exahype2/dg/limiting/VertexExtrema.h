// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Limiter.h"
#include "peano4/Constants.h"
#include "peano4/grid/SendReceiveContext.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::limiting {
  template <int NumberOfObservables, typename VertexLabel>
  INLINE_METHOD void resetVertexObservableExtrema(VertexLabel& vertexLabel);

  template <int NumberOfObservables, typename Value, typename VertexLabels>
  INLINE_METHOD void reduceCellObservableExtremaIntoVertices(
    const Value*        minimum,
    const Value*        maximum,
    const VertexLabels& vertexLabels
  );

  template <int NumberOfObservables, typename Value, typename VertexLabels>
  INLINE_METHOD void widenRangeByVertexObservableExtrema(
    Value*              minimum,
    Value*              maximum,
    const VertexLabels& vertexLabels
  );

  template <int NumberOfObservables, typename VertexLabel, typename CoarseVertexLabels>
  INLINE_METHOD void interpolateHangingVertexObservableExtrema(
    const tarch::la::Vector<DIMENSIONS, int>& hangingVertexPosition,
    VertexLabel&                              vertexLabel,
    const CoarseVertexLabels&                 coarseVertexLabels
  );

  template <int NumberOfObservables, typename VertexLabel, typename CoarseVertexLabels>
  INLINE_METHOD void restrictHangingVertexObservableExtrema(
    const tarch::la::Vector<DIMENSIONS, int>& hangingVertexPosition,
    const VertexLabel&                        vertexLabel,
    const CoarseVertexLabels&                 coarseVertexLabels
  );

  template <int NumberOfObservables, typename VertexLabel>
  INLINE_METHOD void mergeVertexObservableExtrema(
    peano4::grid::SendReceiveContext context,
    VertexLabel&                     vertexLabel,
    const VertexLabel&               neighbour
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/VertexExtrema.cpph"
