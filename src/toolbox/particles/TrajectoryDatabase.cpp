#include "TrajectoryDatabase.h"


toolbox::particles::TrajectoryDatabase::TrajectoryDatabase() {

}


toolbox::particles::TrajectoryDatabase::~TrajectoryDatabase() {

}


void toolbox::particles::TrajectoryDatabase::setOutputFileName( const std::string& filename ) {

}


void toolbox::particles::TrajectoryDatabase::setDeltaBetweenTwoSnapsots( double value ) {

}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int number,
  double timestamp,
  const tarch::la::Vector<Dimensions,double>& x
) {

}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
    int number,
    double timestamp,
    const tarch::la::Vector<Dimensions,double>& x,
    double data
  ) {

}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
    int number,
    double timestamp,
    const tarch::la::Vector<Dimensions,double>& x,
    const tarch::la::Vector<2,double>& data
  ) {

}
