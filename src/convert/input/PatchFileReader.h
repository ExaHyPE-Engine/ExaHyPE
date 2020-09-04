#ifndef _VISUALISATION_INPUT_PEANO_PATCH_FILE_READER_H_
#define _VISUALISATION_INPUT_PEANO_PATCH_FILE_READER_H_


namespace visualisation {
  namespace input {
    class PatchFileReader;
  }
}


#include <vector>


namespace visualisation {
  namespace data {
    class DataSet;
  }
}


class visualisation::input::PatchFileReader {
  public:
	virtual void parse() = 0;
	/**
	 * @return A time series of data sets
	 */
	virtual std::vector< visualisation::data::DataSet >  getData() const = 0;
};

#endif
