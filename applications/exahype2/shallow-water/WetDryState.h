// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::swe {
  enum class WetDryState {
    DryDry,               /**< Both cells are dry. */
    WetWet,               /**< Both cells are wet. */
    WetDryInundation,     /**< 1st cell: wet, 2nd cell: dry. 1st cell lies higher than the 2nd one. */
    WetDryWall,           /**< 1st cell: wet, 2nd cell: dry. 1st cell lies lower than the 2nd one. Momentum is not large enough to overcome the difference. */
    WetDryWallInundation, /**< 1st cell: wet, 2nd cell: dry. 1st cell lies lower than the 2nd one. Momentum is large enough to overcome the difference. */
    DryWetInundation,     /**< 1st cell: dry, 2nd cell: wet. 1st cell lies lower than the 2nd one. */
    DryWetWall,           /**< 1st cell: dry, 2nd cell: wet. 1st cell lies higher than the 2nd one. Momentum is not large enough to overcome the difference. */
    DryWetWallInundation  /**< 1st cell: dry, 2nd cell: wet. 1st cell lies higher than the 2nd one. Momentum is large enough to overcome the difference. */
  };
} // namespace applications::exahype2::swe
