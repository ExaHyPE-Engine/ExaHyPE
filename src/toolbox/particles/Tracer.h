// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLE_TRACER_
#define _TOOLBOX_PARTICLE_TRACER_


#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/CellMarker.h"


namespace toolbox {
  namespace particles {
    /**
     * Assume that we have a particle suspended in a cell. The cell hosts a
     * regular Cartesian mesh. The routine computes the correct voxel.
     */
    tarch::la::Vector<Dimensions,int> mapParticleOntoVoxel(
      const peano4::datamanagement::CellMarker& marker,
      int voxelsPerAxis,
      const tarch::la::Vector<Dimensions,double>& particleX
    );

    tarch::la::Vector<Dimensions,double> explicitEulerWithoutInterpolation(
      const peano4::datamanagement::CellMarker&   marker,
      int                                         voxelsPerAxis,
      int                                         unknownsPerVoxel,
      const tarch::la::Vector<Dimensions,int>&    velocityIndices,
      double                                      timeStepSize,
      double*                                     voxelField,
      const tarch::la::Vector<Dimensions,double>& particleX
    );
  }
}


#endif
