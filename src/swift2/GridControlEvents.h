// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/grid/GridControlEvent.h"
#include <vector>
#include <list>


namespace swift2 {
  /**
   * @todo write some docu
   */
  extern std::vector< peano4::grid::GridControlEvent >   committedGridControlEvents;

  /**
   * Commmit a new set of events
   *
   * - Clear the old set.
   * - Copy the new data over. For this, we have to convert the list into a vector.
   * - Merge the elements within the vector.
   *
   * You still have to clear the passed data if you wanna reuse the argument container.
   */
  void commitGridControlEvents( const std::list< peano4::grid::GridControlEvent >& events );
}
