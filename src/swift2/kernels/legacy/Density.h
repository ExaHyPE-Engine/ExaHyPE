// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



namespace swift2 {
  namespace kernels {
    /**
     * @namespace swift2::kernels::legacy Legacy SPH implementation
     *
     * This implementation is very very close to the original Swift kernels.
     * This can immediately be seen from the signature of the routines: The
     * core routines all work solely with pointers.
     *
     * For Swift 2 however, we need signatures which work with references and
     * also evaluate predicates. So each routine actually comes along as pair:
     * The core science with a pointer and then a wrapper around it.
     */
    namespace legacy {

    }
  }
}
