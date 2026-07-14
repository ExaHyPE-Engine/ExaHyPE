// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::datamanagement {
  /**
   * Helper for generated data records which can swap compatible backing storage.
   */
  template <typename LeftData, typename RightData>
  struct SwapHelper {
    static void apply(LeftData& left, RightData& right) noexcept {
      static_assert(std::is_same_v<typename LeftData::DoFType, typename RightData::DoFType>);
      static_assert(LeftData::Cardinality == RightData::Cardinality);
      std::swap(left._data, right._data);
    }
  };
} // namespace peano4::datamanagement
