#include "Statistics.h"


#include <fstream>


tarch::logging::Statistics  tarch::logging::Statistics::_singleton;
tarch::logging::Log         tarch::logging::Statistics::_log( "tarch::logging::Statistics" );


tarch::logging::Statistics& tarch::logging::Statistics::getInstance() {
  static tarch::logging::Statistics singleton;
  return singleton;
}



tarch::logging::Statistics::Statistics():
  _minCountInBetweenTwoMeasurements(100),
  _minTimeInBetweenTwoMeasurements(1.0) {
}



tarch::logging::Statistics::DataSet::DataSet():
  _watch("tarch::logging::Statistics", "Statistics", false),
  _counter(0) {
}


double tarch::logging::Statistics::acceptNewData(const std::string& identifier) {
  if ( _map.count( identifier )==0 ) {
    _map.insert( std::pair<std::string,DataSet>( identifier, DataSet() ));
  }

  _map[identifier]._counter++;
  bool guard = _map[identifier]._counter>_minCountInBetweenTwoMeasurements;

  if (guard) {
    _map[identifier]._watch.stop();
    guard  = _map[identifier]._watch.getCalendarTime() > _minTimeInBetweenTwoMeasurements;
  }

  if (guard) {
    double result = _map[identifier]._watch.getCalendarTime();
    _map[identifier]._counter = 0;
    _map[identifier]._watch.start();
    return result;
  }
  else return 0.0;
}


void tarch::logging::Statistics::log( const std::string& identifier, double value ) {
  double t = acceptNewData(identifier);
  if (t>0) {
    logDebug( "log(string,double)", identifier << "=" << value );
    _map[identifier]._data.push_back( std::pair<double,double>(t,value) );
  }
}


void tarch::logging::Statistics::writeToCSV( const std::string& filename ) {
  logDebug( "writeToCSV(string)", "start to dump statistics into file " << filename );

  std::ofstream file( filename );
  file << "t";
  for (auto& p: _map) {
    file << ", " << p.first;
  }
  file << std::endl;

  double t = 0.0;
  bool   dataRemaining = true;
  while (dataRemaining) {
    dataRemaining = false;
    file << t;
    for (auto& p: _map) {
      if (not p.second._data.empty()) {
        dataRemaining = true;
        if ( p.second._data[0].first < t + _minTimeInBetweenTwoMeasurements ) {
          file << ", " << p.second._data[0].second;
          p.second._data.erase(p.second._data.begin());
        }
        else {
          file << ", ";
        }
      }
      else {
        file << ", ";
      }
    }
    file << "\n";
    t += _minTimeInBetweenTwoMeasurements;
  }

  file.close();
  logInfo( "writeToCSV(string)", "wrote statistics to file " << filename );
}
