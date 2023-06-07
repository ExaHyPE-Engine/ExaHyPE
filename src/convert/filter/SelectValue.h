// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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
	virtual void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) override;
};

