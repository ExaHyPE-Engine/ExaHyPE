#pragma once


#include <string>


namespace convert {
  namespace filter {
    class Filter;
  }
}


namespace convert {
  namespace data {
    class DataSet;
    class Variable;
  }
}


class convert::filter::Filter {
  public:
	virtual void apply( convert::data::DataSet& dataSet, convert::data::Variable& inputVariable, std::string targetSelectorName ) = 0;
};

