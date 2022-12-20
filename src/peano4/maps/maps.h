// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <utility>


namespace peano4 {
  /**
   * @namespace peano4::maps
   *
   * The map namespaces hosts various map implementations. I use the maps to
   * look up stacks for spacetree id+stack number combinations. The semantics
   * of the classes is only described for STDStackMap which is a plain wrapper
   * around the C++ maps adding some semaphores. Whereever other classes
   * contain documentation, this documentation describes implementation
   * details.
   */
  namespace maps {
    /**
     * To look up the right stack, we use a combination of tree number (id) and
     * stack number.
     */
    typedef std::pair<int,int>  StackKey;
  }
}


