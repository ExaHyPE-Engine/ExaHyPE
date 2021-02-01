// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _CONVERT_FILTER_SEPARATE_RESOLUTIONS_H_
#define _CONVERT_FILTER_SEPARATE_RESOLUTIONS_H_


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
	void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) override;
};

#endif
