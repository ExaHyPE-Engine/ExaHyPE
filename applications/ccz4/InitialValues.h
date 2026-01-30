/**
 *
 * This code is taken from the original ExaHyPE project written by
 * colleagues from the University of Trento.
 *
 */

#pragma once

#include "tarch/la/Vector.h"

#include "peano4/Globals.h"

#ifdef IncludeTwoPunctures
#include "libtwopunctures/TP_bindding.h"
#endif


namespace applications {
  namespace exahype2 {
    namespace ccz4 {
      void gaugeWave(
        double * NOALIAS Q, // Q[64+0],
        const tarch::la::Vector<DIMENSIONS,double>&  x,
        double t
      );
      void diagonal_gaugeWave(
        double * NOALIAS Q, // Q[64+0],
        const tarch::la::Vector<DIMENSIONS,double>&  x,
        double t
      );
      void linearWave(
        double * NOALIAS Q, // Q[64+0],
        const tarch::la::Vector<DIMENSIONS,double>&  X,
        double t
      );
      void flat(
        double * NOALIAS Q, // Q[64+0],
        const tarch::la::Vector<DIMENSIONS,double>&  X,
        double t
      );

      #ifdef IncludeTwoPunctures
      void ApplyTwoPunctures(
        double * NOALIAS Q, // Q[64+0],
        const tarch::la::Vector<DIMENSIONS,double>&  X,
        double t,
	TP::TwoPunctures* tp,
        bool low_res
      );
      #endif
    }
  }
}

