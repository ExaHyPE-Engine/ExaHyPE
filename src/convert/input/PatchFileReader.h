#ifndef _CONVERT_INPUT_PEANO_PATCH_FILE_READER_H_
#define _CONVERT_INPUT_PEANO_PATCH_FILE_READER_H_


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

#endif
