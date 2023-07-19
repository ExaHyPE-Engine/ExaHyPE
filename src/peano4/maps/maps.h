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
     * Unique key identifying a stack
     *
     * To look up the right stack, we use a combination of tree number (id) and
     * stack number. The tree number has to be positive, but it might refer to
     * a local or a remote tree. In the latter case, the identified stack is a
     * data exchange stack. The second entry is always positive and it identifies
     * which stack of the corresponding tree is meant. The class
     * peano4::grid::PeanoCurve contains information on pre-defined stack
     * numbers.
     */
    typedef std::pair<int,int>  StackKey;
  }
}


