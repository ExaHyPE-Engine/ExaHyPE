// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _VISUALISATION_FILTER_INTERSECTION_H_
#define _VISUALISATION_FILTER_INTERSECTION_H_


#include "Filter.h"
#include "tarch/logging/Log.h"


namespace visualisation {
  namespace filter {
    class Intersection;
  }
}


class visualisation::filter::Intersection: public visualisation::filter::Filter {
  public:
	enum class Strategy {
	  KeepFinerGrid
	};
  private:
	static tarch::logging::Log _log;

	const Strategy _strategy;
  public:
	Intersection( Strategy strategy );
	void apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) override;
};


#endif
