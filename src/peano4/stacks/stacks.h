// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_STACKS_STACK_H_
#define _PEANO4_STACKS_STACK_H_


#include <string>


namespace peano4 {
  namespace stacks {
    enum class IOMode {
      None,
      MPISend,
      MPIReceive
    };

    #if !defined(Parallel)
    /**
     * My stacks need send and receive signature.
     */
    typedef int         MPI_Comm;
    #endif
  }
}


std::string toString( peano4::stacks::IOMode mode );


#endif
