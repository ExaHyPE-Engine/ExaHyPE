#pragma once


namespace convert {
  namespace input {
    class PatchFileReader;
  }
}


#include <vector>


namespace convert {
  namespace data {
    class DataSet;
  }
}


class convert::input::PatchFileReader {
  public:
    virtual void parse() = 0;

    /**
     * @return A time series of data sets
     */
    virtual std::vector< convert::data::DataSet >  getData() const = 0;
};

