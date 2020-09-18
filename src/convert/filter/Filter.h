#ifndef _CONVERT_FILTER_FILTER_H_
#define _CONVERT_FILTER_FILTER_H_


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

#endif
