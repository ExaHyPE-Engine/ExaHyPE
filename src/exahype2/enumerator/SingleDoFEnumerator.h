// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace enumerator {
    /**
     * Default enumerator for volumes
     *
     * We assume that we have a lexicographic ordering of the patch data and
     * all data therein are ordered lexicographically and stored in as AoS.
     *
     * @param numberOfPatches_ This field is irrelevant for this enumerator,
     *   as the patches are enumerated one after another, i.e. there is no
     *   patch interleaving. To be formally correct, you should set it to 1
     *   if you construct and enumerator over the input or output data, as
     *   there is one patch per cell.
     */
    struct SingleDoFEnumerator {
      public:
        inline SingleDoFEnumerator(int unknowns, int numberOfAuxiliaryVariables):
          _unknowns(unknowns),
          _numberOfAuxiliaryVariables(numberOfAuxiliaryVariables) {}

        /**
         * Access an index
         *
         * The index always refers to the interior of the patches. So you can use negative
         * indices if you want.
         *
         * @todo modulo Operator muss ich noch dokumentieren
         */
        inline int operator()(
          [[maybe_unused]] int patchIndex,
          [[maybe_unused]] const tarch::la::Vector<Dimensions,int>& volumeIndex,
          [[maybe_unused]] int unknown
        ) const InlineMethod {
          return unknown;
        }

#if !defined(GPUOffloadingOMP)
        std::string toString() const;
#endif

        inline int size() const {
          return (_unknowns+_numberOfAuxiliaryVariables);
        }

        const int _unknowns;
        const int _numberOfAuxiliaryVariables;
    };
  }
}
