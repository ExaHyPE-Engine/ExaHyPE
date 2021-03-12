/**
 *
 * This code is taken from the original ExaHyPE project written by
 * colleagues from the University of Trento.
 *
 */

#ifndef __EXAHYPE_CCZ4_INITIAL_VALUE__
#define __EXAHYPE_CCZ4_INITIAL_VALUE__






#include "tarch/la/Vector.h"

#include "peano4/utils/Globals.h"



namespace examples {
  namespace exahype2 {
    namespace ccz4 {
      void gaugeWave(
        double * __restrict__ Q, // Q[64+0],
        const tarch::la::Vector<Dimensions,double>&  x,
        double t
      );
    }
  }
}



#endif /* __EXAHYPE_USER_PDE__ */
