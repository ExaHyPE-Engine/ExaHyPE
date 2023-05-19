// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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
	  virtual void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) override;
};


