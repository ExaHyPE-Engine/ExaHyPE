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
#include <list>


namespace toolbox {
  namespace particles {
    class TrajectoryDatabase;
  }
}


class toolbox::particles::TrajectoryDatabase {
  private:
    static tarch::logging::Log _log;

    std::string _fileName;
    double      _dataDelta;
    double      _positionDelta;
    int         _numberOfDataPointsPerParticle;

    const int   _deltaBetweenTwoDatabaseFlushes;
    int         _thresholdForNextDatabaseFlush;
    /**
     * @see TrajectoryDatabase()
     */
    const bool  _clearDatabaseAfterFlush;

    /**
     * This is a hack: Usually, I'd just ask the rank what its number is.
     * However, the database dump often is called in the very end, after
     * the MPI rank is already down. So it will return -1. So what I do is
     * that I store this piece of data whenever I insert a new entry.
     */
    int         _rank;

    struct Entry {
      tarch::la::Vector<Dimensions,double>  x;
      double                                timestamp;
      double*                               data;

      Entry( const TrajectoryDatabase& database, const tarch::la::Vector<Dimensions,double>&  x_, double  timestamp_ );
    };

    std::map< std::pair<int,int>, std::list<Entry> >  _data;

    tarch::multicore::BooleanSemaphore        _semaphore;

    enum class AddSnapshotAction {
      Ignore,
      Append,
      Replace
    };

    /**
     * Can't be const as it locks the semaphore.
     */
    AddSnapshotAction getAction(
      const std::pair<int, int>&                  number,
      const tarch::la::Vector<Dimensions,double>& x,
      double                                      timestamp
    );

    AddSnapshotAction getAction(
      const std::pair<int, int>&                   number,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       timestamp,
      int                                          numberOfDataEntries,
      double*                                      data
    );

    bool dumpDatabaseSnapshot() const;
  public:
    /**
     * Trajectory database
     *
     * The database dumps/stores data if and only if the delta of two particles is
     * bigger than a threshold. We always work with the max norm. There's two different
     * thresholds: one for the position, one for the data. So whenever a particle
     * moves by more than positionDelta in any component of its position, we write a
     * new snapshot of the particle. Whenever one of the particle's values changes
     * by more than  dataDelta in one of its components, we write a new snapshot of
     * this particle (even though it might not have moved).
     *
     * <h2> Flushing the database </h2>
     *
     * Please read the documentation of clearDatabaseAfterFlush and
     * growthBetweenTwoDatabaseFlushes below first. If you flush a database every
     * now and then and if you clear the database after that, then the following
     * situation can happen: One particle's data or position changes quite a lot.
     * Another particle's data doesn't change at all. We trigger a flush and, after
     * that, clear the database. Again, the one single particle is updated quite a
     * lot. We flush again. The particle that doesn't change et al will not be
     * contained in the second database snapshot.
     *
     *
     *
     * @param clearDatabaseAfterFlush If this flag is set, each flush of the database
     *   will go into a separte file, and the code will clear the database after the
     *   flush. As a consequence, the database will never exceed the memory. However,
     *   you'll get a lot of files that you have to merge afterwards.
     * @param growthBetweenTwoDatabaseFlushes Whenver a database has been added more
     *   than growthBetweenTwoDatabaseFlushes entries, we dump a snapshot. This can be
     *   growthBetweenTwoDatabaseFlushes different particles, or growthBetweenTwoDatabaseFlushes
     *   updates for only one single particle. In both cases, we flush.
     */
    TrajectoryDatabase( int growthBetweenTwoDatabaseFlushes, double positionDelta = 1e-8, double dataDelta = 1e-8, bool clearDatabaseAfterFlush=true );
    ~TrajectoryDatabase();

    void clear();
    void dumpCSVFile();

    void setOutputFileName( const std::string& filename );
    void setDataDeltaBetweenTwoSnapshots( double value );
    void setPositionDeltaBetweenTwoSnapshots( double value );

    /**
     * A particle is always uniquely identified by two integers (an
     * integer pair). This way, we can initialise (hand out) particle
     * numbers without any semaphore.
     */
    void addParticleSnapshot(
      const std::pair<int, int>&                   number,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x
    );

    void addParticleSnapshot(
      int                                          number0,
      int                                          number1,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x
    );

    void addParticleSnapshot(
      const std::pair<int, int>&                   number,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x,
      int                                          numberOfDataEntries,
      double*                                      data
    );

    void addParticleSnapshot(
      int                                          number0,
      int                                          number1,
      double                                       timestamp,
      const tarch::la::Vector<Dimensions,double>&  x,
      int                                          numberOfDataEntries,
      double*                                      data
    );
};


#endif
