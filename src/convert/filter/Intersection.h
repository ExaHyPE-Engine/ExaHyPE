// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _CONVERT_FILTER_INTERSECTION_H_
#define _CONVERT_FILTER_INTERSECTION_H_


#include "Filter.h"
#include "tarch/logging/Log.h"


namespace convert {
  namespace filter {
    class Intersection;
  }
}


class convert::filter::Intersection: public convert::filter::Filter {
  public:
	enum class Strategy {
	  KeepFinerGrid
	};
  private:
	static tarch::logging::Log _log;

	const Strategy _strategy;
  public:
	Intersection( Strategy strategy );
	void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) override;
};


#endif
