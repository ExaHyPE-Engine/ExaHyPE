#include "DataSet.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "tarch/Assertions.h"


tarch::logging::Log  visualisation::data::DataSet::_log( "visualisation::data::DataSet" );


visualisation::data::DataSet::DataSet() {
}


void visualisation::data::DataSet::free() {
  for (auto& p: data) {
	logDebug( "free()", "free data set " << p.first.name );
	for (auto& pp: p.second) {
      pp.free();
	}
  }
}


std::vector<visualisation::data::PatchData> visualisation::data::DataSet::getData( const Variable& selector ) const {
  if ( data.count(selector)==0) {
    logError( "createReaders(...)", "no data set with identifier " << selector.name );
	return std::vector<visualisation::data::PatchData>();
  }
  else {
    return data.at(selector);
  }
}


void visualisation::data::DataSet::merge(const DataSet& other) {
  for (const auto& p: other.data) {
	if ( data.count(p.first)==0 ) {
      data.insert( std::pair< Variable, std::vector<PatchData> >(
        p.first, p.second
      ));
	}
	else {
	  for (const auto& pp: other.getData(p.first)) {
        data[p.first].push_back(pp);
	  }
	}
  }
}


std::vector<visualisation::data::Variable>  visualisation::data::DataSet::getVariables() const {
  std::vector<visualisation::data::Variable> result;
  for (auto p: data) {
	result.push_back(p.first);
  }
  return result;
}


bool visualisation::data::DataSet::hasVariable( const std::string& name ) const {
  for (auto& p: data) {
 	if (p.first.name==name) {
  	  return true;
  	}
  }
  return false;
}


visualisation::data::Variable visualisation::data::DataSet::getVariable( const std::string& name ) const {
  const visualisation::data::Variable* result;
  for (auto& p: data) {
 	if (p.first.name==name) {
  	  result = &(p.first);
  	}
  }
  return *result;
}


void visualisation::data::DataSet::add( Variable variable, const std::vector<PatchData>& patchData ) {
  assertion( data.count(variable)==0 );

  data.insert( std::pair< visualisation::data::Variable, std::vector<visualisation::data::PatchData> >(
	variable, patchData
  ));
}
