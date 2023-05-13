// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "Filter.h"
#include "tarch/logging/Log.h"


namespace convert {
  namespace filter {
    class SeparateResolutions;
  }
}


class convert::filter::SeparateResolutions: public convert::filter::Filter {
  private:
	static tarch::logging::Log _log;
  public:
	virtual void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) override;
};

