#include "TrajectoryDatabase.h"

#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"

#include <fstream>


tarch::logging::Log toolbox::particles::TrajectoryDatabase::_log( "toolbox::particles::TrajectoryDatabase" );


toolbox::particles::TrajectoryDatabase::Entry::Entry( const TrajectoryDatabase& database, const tarch::la::Vector<Dimensions,double>&  x_, double  timestamp_ ):
  x(x_),
  timestamp(timestamp_) {
  if (database._numberOfDataPointsPerParticle>0) {
    data = new double[database._numberOfDataPointsPerParticle];
  }
  else {
    data =nullptr;
  }
}


toolbox::particles::TrajectoryDatabase::TrajectoryDatabase( int growthBetweenTwoDatabaseFlushes, double positionDelta, double dataDelta, bool clearDatabaseAfterFlush ):
  _fileName(""),
  _dataDelta(dataDelta),
  _positionDelta(positionDelta),
  _numberOfDataPointsPerParticle(0),
  _deltaBetweenTwoDatabaseFlushes(growthBetweenTwoDatabaseFlushes),
  _thresholdForNextDatabaseFlush(growthBetweenTwoDatabaseFlushes==0 ? std::numeric_limits<int>::max() : _deltaBetweenTwoDatabaseFlushes),
  _clearDatabaseAfterFlush(clearDatabaseAfterFlush),
  _rank(-1) {
}


toolbox::particles::TrajectoryDatabase::~TrajectoryDatabase() {
  if (_fileName!="") {
    dumpCSVFile();
  }
  clear();
}


void toolbox::particles::TrajectoryDatabase::clear() {
  for (auto& particle: _data) {
    for (auto& snapshot: particle.second) {
      if (snapshot.data!=nullptr) {
        delete[] snapshot.data;
      }
    }
  }
  _data.clear();
}


void toolbox::particles::TrajectoryDatabase::dumpCSVFile() {
  std::ostringstream snapshotFileName;
  snapshotFileName << _fileName;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>0 ) {
    if ( _rank<0 ) {
      _rank = tarch::mpi::Rank::getInstance().getRank();
    }
    snapshotFileName << "-rank-" << _rank;
  }

  if (_clearDatabaseAfterFlush) {
    static int snapshotCounter = -1;
    snapshotCounter++;
    snapshotFileName << "-snapshot-" << snapshotCounter;
  }

  snapshotFileName << ".csv";

  if (not _data.empty()) {
    logInfo( "dumpCSVFile()", "dump particle trajectory database " << snapshotFileName.str() );
    std::ofstream file( snapshotFileName.str() );
    #if Dimensions==2
    file << "number(0), number(1), t, x(0), x(1), data " << std::endl;
    #else
    file << "number(0), number(1), t, x(0), x(1), x(2), data " << std::endl;
    #endif

    for (auto& particle: _data) {
      for (auto& snapshot: particle.second) {
        file << particle.first.first
             << ", "
             << particle.first.second
			 << ", "
			 << snapshot.timestamp;
        #if Dimensions==2
        file << ", "
             << snapshot.x(0)
             << ", "
             << snapshot.x(1);
        #else
        file << ", "
             << snapshot.x(0)
             << ", "
             << snapshot.x(1)
             << ", "
             << snapshot.x(2);
        #endif

        if (snapshot.data!=nullptr) {
          for (int i=0; i<_numberOfDataPointsPerParticle; i++) {
            file << ", "
                 << snapshot.data[i];
          }
        }
        file << std::endl;
      }
    }
  }
  else {
    #if PeanoDebug>=1
    logInfo( "dumpCSVFile()", "particle trajectory database is empty. Do not dump " << snapshotFileName.str() );
    #endif
  }

  if (_clearDatabaseAfterFlush) {
    clear();
  }
}


void toolbox::particles::TrajectoryDatabase::setOutputFileName( const std::string& filename ) {
  _fileName = filename;
}


void toolbox::particles::TrajectoryDatabase::setDataDeltaBetweenTwoSnapshots( double value ) {
  assertion(value>=0.0);
  _dataDelta = value;
}


void toolbox::particles::TrajectoryDatabase::setPositionDeltaBetweenTwoSnapshots( double value ) {
  assertion(value>=0.0);
  _positionDelta = value;
}


toolbox::particles::TrajectoryDatabase::AddSnapshotAction toolbox::particles::TrajectoryDatabase::getAction(
  const std::pair<int, int>&                    number,
  const tarch::la::Vector<Dimensions,double>&   x,
  double                                        timestamp
) {
  tarch::multicore::Lock lock(_semaphore);

  if (_data.count(number)==0) {
    _data.insert( std::pair<std::pair<int,int>, std::list<Entry>>(number,std::list<Entry>()) );
    return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Append;
  }
  else if (_data.at(number).empty()) {
    return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Append;
  }
  else {
    tarch::la::Vector<Dimensions,double> oldX = _data.at(number).front().x;
    if (
      tarch::la::normMax(oldX-x)>=_positionDelta
      and
      tarch::la::equals( _data.at(number).front().timestamp, timestamp )
    ) {
      return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Replace;
    }
    else if (
      tarch::la::normMax(oldX-x)>=_positionDelta
    ) {
      return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Replace;
    }
    else {
      return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Ignore;
    }
  }
}


toolbox::particles::TrajectoryDatabase::AddSnapshotAction toolbox::particles::TrajectoryDatabase::getAction(
  const std::pair<int, int>&                   number,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       timestamp,
  int                                          numberOfDataEntries,
  double*                                      data
) {
  toolbox::particles::TrajectoryDatabase::AddSnapshotAction result = getAction(number,x,timestamp);

  if (result == toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Ignore) {
    tarch::multicore::Lock lock(_semaphore);
    for (int i=0; i<numberOfDataEntries; i++) {
      if (
        std::abs( _data.at(number).front().data[i] - data[i] ) > _dataDelta
        and
        tarch::la::equals( _data.at(number).front().timestamp, timestamp )
      ) {
        return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Replace;
      }
      else if (
        std::abs( _data.at(number).front().data[i] - data[i] ) > _dataDelta
      ) {
        return toolbox::particles::TrajectoryDatabase::AddSnapshotAction::Append;
      }
    }
  }

  return result;
}


bool toolbox::particles::TrajectoryDatabase::dumpDatabaseSnapshot() const {
  int totalSize = 0;
  for (auto p: _data) {
    totalSize += p.second.size();
  }
  return totalSize >= _thresholdForNextDatabaseFlush;
}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  const std::pair<int, int>&                   number,
  double                                       timestamp,
  const tarch::la::Vector<Dimensions,double>&  x
) {
  if ( _rank<0 ) {
    _rank = tarch::mpi::Rank::getInstance().getRank();
  }

  switch ( getAction(number,x,timestamp) ) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.at(number).push_front( Entry(*this,x,timestamp) );
      for (int i=0; i<_numberOfDataPointsPerParticle; i++) {
        _data.at(number).front().data[i] = 0.0;
      }
      break;
    case AddSnapshotAction::Replace:
      _data.at(number).front().x = x;
      for (int i=0; i<_numberOfDataPointsPerParticle; i++) {
        _data.at(number).front().data[i] = 0.0;
      }
      break;
  }

  if (dumpDatabaseSnapshot()) {
    dumpCSVFile();
    logInfo( "addSnapshot(...)", "flushed database file (temporary flush - simulation has not terminated yet)" );
    _thresholdForNextDatabaseFlush += _deltaBetweenTwoDatabaseFlushes;
  }
}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int                                          number0,
  int                                          number1,
  double                                       timestamp,
  const tarch::la::Vector<Dimensions,double>&  x
) {
  addParticleSnapshot(
    std::pair<int,int>(number0,number1),
    timestamp,
    x
  );
}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  const std::pair<int, int>&                   number,
  double                                       timestamp,
  const tarch::la::Vector<Dimensions,double>&  x,
  int                                          numberOfDataEntries,
  double*                                      data
) {
  assertion( _numberOfDataPointsPerParticle==numberOfDataEntries or _data.empty());
  _numberOfDataPointsPerParticle = std::max(_numberOfDataPointsPerParticle,numberOfDataEntries);

  if ( _rank<0 ) {
    _rank = tarch::mpi::Rank::getInstance().getRank();
  }

  switch ( getAction(number,x,timestamp,numberOfDataEntries,data) ) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.at(number).push_front( Entry(*this,x,timestamp) );
      for (int i=0; i<_numberOfDataPointsPerParticle; i++) {
        _data.at(number).front().data[i] = data[i];
      }
      break;
    case AddSnapshotAction::Replace:
      _data.at(number).front().x = x;
      for (int i=0; i<_numberOfDataPointsPerParticle; i++) {
        _data.at(number).front().data[i] = data[i];
      }
      break;
  }

  if (dumpDatabaseSnapshot()) {
    dumpCSVFile();
    logInfo( "addSnapshot(...)", "flush database file " << _fileName << " (temporary flush - simulation has not terminated yet)" );
    _thresholdForNextDatabaseFlush = _deltaBetweenTwoDatabaseFlushes;
  }
}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int                                          number0,
  int                                          number1,
  double                                       timestamp,
  const tarch::la::Vector<Dimensions,double>&  x,
  int                                          numberOfDataEntries,
  double*                                      data
) {
  addParticleSnapshot(
    std::pair<int,int>(number0,number1),
    timestamp,
    x,
    numberOfDataEntries,
    data
  );
}
