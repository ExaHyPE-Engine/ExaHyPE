// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "globaldata/HydroPart.h"


namespace benchmarks{
  namespace swift2 {
    namespace hydro {
      namespace kernel_throughput {
        /*
         * Initial Conditions for the Noh problem
         *
         * We give particles an initial velocity towards the centre.
         * Particles will then develop a shock that propagates outwards.
         * This routine assumes that the particle of interest has a valid position
         * already, and that the particles are arranged within the unit
         * square/cube.
         *
         * @param h Mesh size of adjacent cells of the vertex to which this
         *   particle is associated.
         */
        void initNohProblemIC(
          globaldata::HydroPart&  particle,
          const tarch::la::Vector<Dimensions,double>& h,
          double HYDRO_PART_NUMBER,
          double HYDRO_DIMENSIONS
        );
      }
    }
  }
}
