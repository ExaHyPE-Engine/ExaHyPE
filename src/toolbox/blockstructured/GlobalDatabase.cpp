#include "GlobalDatabase.h"

#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"

#include <fstream>


tarch::logging::Log toolbox::blockstructured::GlobalDatabase::_log( "toolbox::blockstructured::GlobalDatabase" );


toolbox::blockstructured::GlobalDatabase::Entry::Entry( const GlobalDatabase& database, double  timestamp_ ):
  timestamp(timestamp_),
  numberOfDataEntries(database._numberOfGlobalDataPoints) {
  if (numberOfDataEntries>0) {
    data = new double[numberOfDataEntries];
    std::fill_n( data, numberOfDataEntries, 0.0 );
  }
  else {
    data =nullptr;
  }
}


toolbox::blockstructured::GlobalDatabase::Entry::Entry( const Entry& copy ):
  timestamp(copy.timestamp),
  numberOfDataEntries(copy.numberOfDataEntries) {
  if (numberOfDataEntries>0) {
    data = new double[numberOfDataEntries];
    std::copy_n( copy.data, numberOfDataEntries, data );
  }
  else {
    data =nullptr;
  }
}


toolbox::blockstructured::GlobalDatabase::Entry::~Entry() {
  delete[] data;
}


toolbox::blockstructured::GlobalDatabase::GlobalDatabase( int growthBetweenTwoDatabaseFlushes, double dataDelta, double timeDelta, bool clearDatabaseAfterFlush, bool deltasAreRelative ):
  _fileName(""),
  _dataName(" data "),
  _timeDelta(0.0),
  _dataDelta(dataDelta),
  _precision(0),
  _maxDataDelta(0.0),
  _numberOfGlobalDataPoints(0),
  _deltaBetweenTwoDatabaseFlushes(growthBetweenTwoDatabaseFlushes),
  _thresholdForNextDatabaseFlush(growthBetweenTwoDatabaseFlushes==0 ? std::numeric_limits<int>::max() : 0),
  _clearDatabaseAfterFlush(clearDatabaseAfterFlush),
  _deltasAreRelative(deltasAreRelative),
  _rank(-1),
  _maxTimestamp( -std::numeric_limits<double>::max() )  {
}


toolbox::blockstructured::GlobalDatabase::~GlobalDatabase() {
  if (_fileName!="") {
    dumpCSVFile();
  }
  clear();
}


void toolbox::blockstructured::GlobalDatabase::clear(bool lockSemaphore) {
  tarch::multicore::Lock lock(_semaphore, false);

  if (lockSemaphore) {
    lock.lock();
  }

  _data.clear();
}


void toolbox::blockstructured::GlobalDatabase::dumpCSVFile() {
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

  tarch::multicore::Lock lock(_semaphore);
  if (not _data.empty()) {
    logInfo( "dumpCSVFile()", "dump particle trajectory database " << snapshotFileName.str() );
    std::ofstream file( snapshotFileName.str() );
    file << std::scientific;
    file << "t," << _dataName << std::endl;

    for (auto& snapshot: _data) {
      file << snapshot.timestamp;

      if (snapshot.data!=nullptr) {
        if(_precision>0){ file.precision(_precision); }
        for (int i=0; i<_numberOfGlobalDataPoints; i++) {
          file << ", "
                << snapshot.data[i];
        }
      }
      file << std::endl;
    }

  }
  else {
    #if PeanoDebug>=1
    logInfo( "dumpCSVFile()", "particle trajectory database is empty. Do not dump " << snapshotFileName.str() );
    #endif
  }

  if (_clearDatabaseAfterFlush) {
    clear(false);
  }
}

void toolbox::blockstructured::GlobalDatabase::setDeltaBetweenTwoDatabaseFlushes(const int deltaBetweenTwoDatabaseFlushes) {
  _deltaBetweenTwoDatabaseFlushes = deltaBetweenTwoDatabaseFlushes;
}

void toolbox::blockstructured::GlobalDatabase::setOutputFileName( const std::string& filename ) {
  _fileName = filename;
}

void toolbox::blockstructured::GlobalDatabase::setDataName( const std::string& dataname ) {
  _dataName = dataname;
}

void toolbox::blockstructured::GlobalDatabase::setOutputPrecision( int precision ) {
  _precision = precision;
}

void toolbox::blockstructured::GlobalDatabase::clearDatabaseAfterFlush(bool value) {
  _clearDatabaseAfterFlush = value;
}

void toolbox::blockstructured::GlobalDatabase::setDataDeltaBetweenTwoSnapshots( double value, bool deltasAreRelative ) {
  assertion(value>=0.0);
  _dataDelta = value;
  _maxDataDelta = 1e-20;
  _deltasAreRelative = deltasAreRelative;
}

void toolbox::blockstructured::GlobalDatabase::setTimeDeltaBetweenTwoSnapshots( double value ) {
  assertion(value>=0.0);
  _timeDelta = value;
}

toolbox::blockstructured::GlobalDatabase::AddSnapshotAction toolbox::blockstructured::GlobalDatabase::getAction(
  double                                        timestamp
) {

  if (_data.empty()) {
      return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Append;
  }
  else if(tarch::la::equals(_data.front().timestamp,timestamp)){
      logWarning( "getAction(...)", "There were two values for same time stamp " << timestamp << ". This is inconsistent");
      return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Replace;
  }
  else if(timestamp - _data.front().timestamp >=_timeDelta and _timeDelta>=0.0 ){
     return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Append;
  }
  else{
      return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Ignore;
  }

}


toolbox::blockstructured::GlobalDatabase::AddSnapshotAction toolbox::blockstructured::GlobalDatabase::getAction(
  double                                       timestamp,
  int                                          numberOfDataEntries,
  double*                                      data
) {

  toolbox::blockstructured::GlobalDatabase::AddSnapshotAction result = getAction(timestamp);

  if (result == toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Replace) {
      double maxDataDeltaThisAction = 0;
      for (int i=0; i<numberOfDataEntries; i++) {
          maxDataDeltaThisAction = std::max( maxDataDeltaThisAction,
            std::abs( _data.front().data[i] - data[i] ));
      }
      _maxDataDelta = std::max( _maxDataDelta, maxDataDeltaThisAction );

      // if dataDelta this action is lower than the required dataDelta and things aren't relative, just keep
      // if it's greater and it isn't relative, replace
      // if it is relative, then use that to compare
      if( maxDataDeltaThisAction > _dataDelta ){
          return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Replace;
      }
      else if( _deltasAreRelative 
        and maxDataDeltaThisAction>=_deltaCutOffThreshold 
        and maxDataDeltaThisAction/_maxDataDelta >= _dataDelta ){
          return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Append;
      }
      else if ( not _deltasAreRelative and maxDataDeltaThisAction >= _dataDelta ) {
          return toolbox::blockstructured::GlobalDatabase::AddSnapshotAction::Append;
      }
  }

  return result;

}


bool toolbox::blockstructured::GlobalDatabase::dumpDatabaseSnapshot() {
  tarch::multicore::Lock lock(_semaphore);
  const int totalSize = _data.size();
  return tarch::la::greater(_maxTimestamp,0.0) and totalSize >= _thresholdForNextDatabaseFlush;
}


void toolbox::blockstructured::GlobalDatabase::addGlobalSnapshot(
  double                                       timestamp
) {

  if ( _rank<0 ) {
    _rank = tarch::mpi::Rank::getInstance().getRank();
  }

  tarch::multicore::Lock lock(_semaphore);
  _maxTimestamp = std::max(_maxTimestamp,timestamp);
  switch ( getAction(timestamp) ) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.push_front( Entry(*this,timestamp) );
      for (int i=0; i<_numberOfGlobalDataPoints; i++) {
        _data.front().data[i] = 0.0;
      }
      break;
    case AddSnapshotAction::Replace:
      for (int i=0; i<_numberOfGlobalDataPoints; i++) {
        _data.front().data[i] = 0.0;
      }
      break;
  }
  lock.free();

  if (dumpDatabaseSnapshot()) {
    dumpCSVFile();
    logInfo( "addSnapshot(...)", "flushed database file (temporary flush - simulation has not terminated yet)" );
    _thresholdForNextDatabaseFlush += _deltaBetweenTwoDatabaseFlushes;
  }
}


void toolbox::blockstructured::GlobalDatabase::addGlobalSnapshot(
  double                                       timestamp,
  int                                          numberOfDataEntries,
  double*                                      data
) {
  assertion( _numberOfGlobalDataPoints==numberOfDataEntries or _data.empty());
  _numberOfGlobalDataPoints = std::max(_numberOfGlobalDataPoints,numberOfDataEntries);

  if ( _rank<0 ) {
    _rank = tarch::mpi::Rank::getInstance().getRank();
  }

  tarch::multicore::Lock lock(_semaphore);
  _maxTimestamp = std::max(_maxTimestamp,timestamp);
  switch ( getAction(timestamp,numberOfDataEntries,data) ) {
    case AddSnapshotAction::Ignore:
      break;
    case AddSnapshotAction::Append:
      _data.push_front( Entry(*this,timestamp) );
      for (int i=0; i<_numberOfGlobalDataPoints; i++) {
        _data.front().data[i] = data[i];
      }
      break;
    case AddSnapshotAction::Replace:
      for (int i=0; i<_numberOfGlobalDataPoints; i++) {
        _data.front().data[i] = data[i];
      }
      break;
  }
  lock.free();

  if (dumpDatabaseSnapshot()) {
    dumpCSVFile();
    logInfo( "addSnapshot(...)", "flush database file " << _fileName << " (temporary flush - simulation has not terminated yet)" );
    _thresholdForNextDatabaseFlush = _deltaBetweenTwoDatabaseFlushes;
  }
}
