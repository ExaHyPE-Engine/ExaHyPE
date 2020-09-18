#ifndef _CONVERT_OUTPUT_WRITER_H_
#define _CONVERT_OUTPUT_WRITER_H_


#include "convert/data/PatchData.h"
#include "convert/data/Variable.h"

#include <vector>


namespace convert {
  namespace output {
    class Writer;
  }
}


class convert::output::Writer {
  public:
	/**
	 * Our writers write only one variable at a time even though the Peano file
	 * format can hold multiple variables/sets
	 */
	virtual void writeFile(const convert::data::Variable& variable, const std::vector<convert::data::PatchData>& data) = 0;
};


#endif
