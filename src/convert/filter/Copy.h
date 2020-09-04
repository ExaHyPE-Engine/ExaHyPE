// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _VISUALISATION_FILTER_COPY_H_
#define _VISUALISATION_FILTER_COPY_H_


#include "Filter.h"
#include "tarch/logging/Log.h"


namespace visualisation {
  namespace filter {
    class Copy;
  }
}


class visualisation::filter::Copy: public visualisation::filter::Filter {
  private:
	static tarch::logging::Log _log;
  public:
	void apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) override;
};

#endif
