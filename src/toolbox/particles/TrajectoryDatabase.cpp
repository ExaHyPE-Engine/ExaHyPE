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


toolbox::particles::TrajectoryDatabase::TrajectoryDatabase():
  _fileName(""),
  _delta(0.0),
  _numberOfDataPointsPerParticle(0) {
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
}


void toolbox::particles::TrajectoryDatabase::dumpCSVFile() {
  std::string filename = _fileName;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()>0 ) {
    filename += "-rank-" + std::to_string( tarch::mpi::Rank::getInstance().getRank() );
  }

  filename += ".csv";

  if (not _data.empty()) {

    std::ofstream file( filename );
    #if Dimensions==2
    file << "t, number, x(0), x(1), data " << std::endl;
    #else
    file << "t, number, x(0), x(1), x(2), data " << std::endl;
    #endif

    for (auto& particle: _data) {
      for (auto& snapshot: particle.second) {
        file << snapshot.timestamp
             << ", "
             << particle.first;
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
    logInfo( "dumpCSVFile()", "particle trajectory database is empty. Do not dump " + filename );
  }
}


void toolbox::particles::TrajectoryDatabase::setOutputFileName( const std::string& filename ) {
  _fileName = filename;
}


void toolbox::particles::TrajectoryDatabase::setDeltaBetweenTwoSnapsots( double value ) {
  _delta = value;
}


bool toolbox::particles::TrajectoryDatabase::addSnapshot(
  int number,
  const tarch::la::Vector<Dimensions,double>& x
) {
  tarch::multicore::Lock lock(_semaphore);

  if (_data.count(number)==0) {
    _data.insert( std::pair<int, std::forward_list<Entry> >(number,std::forward_list<Entry>() ) );
    return true;
  }
  else {
    tarch::la::Vector<Dimensions,double> oldX = _data.at(number).front().x;
    return tarch::la::norm2(oldX-x)>=_delta;
  }
}


void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int number,
  double timestamp,
  const tarch::la::Vector<Dimensions,double>& x
) {
  if (addSnapshot(number,x)) {
    _data.at(number).push_front( Entry(*this,x,timestamp) );
    for (int i=0; i<_numberOfDataPointsPerParticle; i++) {
      _data.at(number).front().data[i] = 0.0;
    }
  }
}



void toolbox::particles::TrajectoryDatabase::addParticleSnapshot(
  int number,
  double timestamp,
  const tarch::la::Vector<Dimensions,double>& x,
  int     numberOfDataEntries,
  double* data
) {
  assertion( _numberOfDataPointsPerParticle==numberOfDataEntries or _data.empty());
  _numberOfDataPointsPerParticle = std::max(_numberOfDataPointsPerParticle,numberOfDataEntries);

  if (addSnapshot(number,x)) {
    _data.at(number).push_front( Entry(*this,x,timestamp) );
    for (int i=0; i<numberOfDataEntries; i++) {
      _data.at(number).front().data[i] = data[i];
    }
    for (int i=numberOfDataEntries; i<_numberOfDataPointsPerParticle; i++) {
      _data.at(number).front().data[i] = 0.0;
    }
  }
}
