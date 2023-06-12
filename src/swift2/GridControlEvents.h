// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>
#include <vector>

#include "peano4/grid/GridControlEvent.h"

namespace swift2 {
  /**
   * @todo write some docu
   */
  extern std::vector<peano4::grid::GridControlEvent> committedGridControlEvents;

  /**
   * Commmit a new set of events
   *
   * - Clear the old set.
   * - Copy the new data over. For this, we have to convert the list into a
   *   vector.
   * - Merge the elements within the vector. This last step is important, as we
   *   run the risk that we have thousands of refine events. Handling such
   *   large quantities is expensive, so we should merge them. At the same
   *   time, the underlying action set yields a lot of tiny, disjoint refine
   *   events, i.e. we haev to be rather generous when we merge them her.
   *
   * You still have to clear the passed data if you wanna reuse the argument container.
   */
  void commitGridControlEvents(const std::list<peano4::grid::GridControlEvent>& events);
} // namespace swift2
