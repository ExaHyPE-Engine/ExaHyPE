#ifndef _VISUALISATION_FILTER_FILTER_H_
#define _VISUALISATION_FILTER_FILTER_H_


#include <string>


namespace visualisation {
  namespace filter {
    class Filter;
  }
}


namespace visualisation {
  namespace data {
    class DataSet;
    class Variable;
  }
}


class visualisation::filter::Filter {
  public:
	virtual void apply( visualisation::data::DataSet& dataSet, visualisation::data::Variable& inputVariable, std::string targetSelectorName ) = 0;
};

#endif
