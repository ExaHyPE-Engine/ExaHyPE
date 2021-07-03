#include "FileReader.h"

#include <fstream>

#include <stdio.h>
#include <string.h>


tarch::logging::Log  toolbox::particles::FileReader::_log( "toolbox::particles::FileReader" );


void toolbox::particles::FileReader::readDatFile( const std::string& filename ) {
  std::fstream fin;

  fin.open( filename, std::ios::in );
  std::string line;

  if (not fin) {
    logError( "readDatFile(string)", "was not able to open file " << filename );
  }

  while ( std::getline(fin,line) ){
    std::stringstream currentLine(line);
    tarch::la::Vector<Dimensions,double> newEntry;

    int entry = 0;
    std::string token;
    while ( std::getline(currentLine, token, ' ') and entry<Dimensions ) {
      if (token!="") {
        newEntry(entry) = std::atof( token.c_str() );
        entry++;
      }
    }
    _data.push_back( newEntry );
  }

  fin.close();

  logInfo( "readDatFile(string)", "read " << _data.size() << " particle positions from file " << filename );
}


void toolbox::particles::FileReader::clear() {
  _data.clear();
}


std::list< tarch::la::Vector<Dimensions,double> > toolbox::particles::FileReader::getParticlesWithinVoxel(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  std::list< tarch::la::Vector<Dimensions,double> > result;

  std::list< tarch::la::Vector<Dimensions,double> >::iterator p = _data.begin();
  while ( p!=_data.end() ) {
    bool overlaps = true;

    for (int d=0; d<Dimensions; d++) {
      overlaps &= tarch::la::smallerEquals( x(d)-h(d)/2.0, (*p)(d) );
      overlaps &= tarch::la::greaterEquals( x(d)+h(d)/2.0, (*p)(d) );
    }

    if (overlaps) {
      result.push_back(*p);
      p = _data.erase(p);
    }
    else {
      p++;
    }
  }

  return result;
}
