#include "DataSet.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "tarch/Assertions.h"


tarch::logging::Log  convert::data::DataSet::_log( "convert::data::DataSet" );


convert::data::DataSet::DataSet() {
}


void convert::data::DataSet::free() {
  for (auto& p: data) {
	logDebug( "free()", "free data set " << p.first.name );
	for (auto& pp: p.second) {
      pp.free();
	}
  }
}


std::vector<convert::data::PatchData> convert::data::DataSet::getData( const Variable& selector ) const {
  if ( data.count(selector)==0) {
    logError( "createReaders(...)", "no data set with identifier " << selector.name );
	return std::vector<convert::data::PatchData>();
  }
  else {
    return data.at(selector);
  }
}


void convert::data::DataSet::merge(const DataSet& other) {
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


std::vector<convert::data::Variable>  convert::data::DataSet::getVariables() const {
  std::vector<convert::data::Variable> result;
  for (auto p: data) {
	result.push_back(p.first);
  }
  return result;
}


bool convert::data::DataSet::hasVariable( const std::string& name ) const {
  for (auto& p: data) {
 	if (p.first.name==name) {
  	  return true;
  	}
  }
  return false;
}


convert::data::Variable convert::data::DataSet::getVariable( const std::string& name ) const {
  const convert::data::Variable* result;
  for (auto& p: data) {
 	if (p.first.name==name) {
  	  result = &(p.first);
  	}
  }
  return *result;
}


void convert::data::DataSet::add( Variable variable, const std::vector<PatchData>& patchData ) {
  assertion( data.count(variable)==0 );

  data.insert( std::pair< convert::data::Variable, std::vector<convert::data::PatchData> >(
	variable, patchData
  ));
}
