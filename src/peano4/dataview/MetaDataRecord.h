// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/SendReceiveContext.h"
#include "tarch/CompilerSpecificSettings.h"

namespace peano4::dataview {
  template <class Record, class Marker>
  class MetaDataRecord;
} // namespace peano4::dataview

/**
 * Provides exchange predicates for a self-contained metadata record.
 *
 * The derived record supplies sendCondition() and receiveAndMergeCondition().
 */
template <class Record, class Marker>
class peano4::dataview::MetaDataRecord {
public:
  using DoFType = std::uint8_t;

  static constexpr int Cardinality = 1;

  INLINE_METHOD bool send(const Marker& marker, peano4::grid::SendReceiveContext context) const;
  INLINE_METHOD bool receiveAndMerge(const Marker& marker, peano4::grid::SendReceiveContext context) const;

  INLINE_METHOD static bool load(const Marker& marker);
};

#include "peano4/dataview/MetaDataRecord.cpph"
