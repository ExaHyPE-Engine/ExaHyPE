// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _CONVERT_FILTER_SELECT_VALUE_H_
#define _CONVERT_FILTER_SELECT_VALUE_H_


#include "Filter.h"
#include "tarch/logging/Log.h"


namespace convert {
  namespace filter {
    class SelectValue;
  }
}


class convert::filter::SelectValue: public convert::filter::Filter {
  private:
	static tarch::logging::Log _log;

	const double _from;
	const double _to;
  public:
	SelectValue( double from, double to );
	void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) override;
};

#endif
