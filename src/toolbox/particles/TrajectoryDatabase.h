// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_PARTICLE_TRAJECTORY_DATABASE_
#define _TOOLBOX_PARTICLE_TRAJECTORY_DATABASE_


#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/utils/Globals.h"

#include <string>
#include <map>
#include <forward_list>


namespace toolbox {
  namespace particles {
    class TrajectoryDatabase;
  }
}


class toolbox::particles::TrajectoryDatabase {
  private:
    static tarch::logging::Log _log;

    std::string _fileName;
    double      _delta;
    int         _numberOfDataPointsPerParticle;

    struct Entry {
      tarch::la::Vector<Dimensions,double>  x;
      double                                timestamp;
      double*                               data;

      Entry( const TrajectoryDatabase& database, const tarch::la::Vector<Dimensions,double>&  x_, double  timestamp_ );
    };

    std::map<int, std::forward_list<Entry> >  _data;

    tarch::multicore::BooleanSemaphore        _semaphore;

    bool addSnapshot(
      int number,
      const tarch::la::Vector<Dimensions,double>& x
    );


  public:
    TrajectoryDatabase();
    ~TrajectoryDatabase();

    void clear();
    void dumpCSVFile();

    void setOutputFileName( const std::string& filename );
    void setDeltaBetweenTwoSnapsots( double value );

    void addParticleSnapshot(
      int number,
      double timestamp,
      const tarch::la::Vector<Dimensions,double>& x
    );

    void addParticleSnapshot(
      int     number,
      double  timestamp,
      const tarch::la::Vector<Dimensions,double>& x,
      int     numberOfDataEntries,
      double* data
    );
};


#endif
