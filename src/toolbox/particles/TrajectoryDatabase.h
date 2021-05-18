// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLE_TRAJECTORY_DATABASE_
#define _TOOLBOX_PARTICLE_TRAJECTORY_DATABASE_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"

#include <string>


namespace toolbox {
  namespace particles {
    class TrajectoryDatabase;
  }
}


class toolbox::particles::TrajectoryDatabase {
  private:
  public:
    TrajectoryDatabase();
    ~TrajectoryDatabase();

    void setOutputFileName( const std::string& filename );
    void setDeltaBetweenTwoSnapsots( double value );

    void addParticleSnapshot(
      int number,
      double timestamp,
      const tarch::la::Vector<Dimensions,double>& x
    );

    void addParticleSnapshot(
      int number,
      double timestamp,
      const tarch::la::Vector<Dimensions,double>& x,
      double data
    );

    void addParticleSnapshot(
      int number,
      double timestamp,
      const tarch::la::Vector<Dimensions,double>& x,
      const tarch::la::Vector<2,double>& data
    );
};


#endif
