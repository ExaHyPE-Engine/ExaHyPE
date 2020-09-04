// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _VISUALISATION_FILTER_SELECT_VALUE_H_
#define _VISUALISATION_FILTER_SELECT_VALUE_H_


#include "Filter.h"
#include "tarch/logging/Log.h"


namespace visualisation {
  namespace filter {
    class SelectValue;
  }
}


class visualisation::filter::SelectValue: public visualisation::filter::Filter {
  private:
	static tarch::logging::Log _log;

	const double _from;
	const double _to;
  public:
	SelectValue( double from, double to );
	void apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) override;
};

#endif
