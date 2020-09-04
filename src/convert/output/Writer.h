#ifndef _VISUALISATION_OUTPUT_WRITER_H_
#define _VISUALISATION_OUTPUT_WRITER_H_


#include "visualisation/data/PatchData.h"
#include "visualisation/data/Variable.h"

#include <vector>


namespace visualisation {
  namespace output {
    class Writer;
  }
}


class visualisation::output::Writer {
  public:
	/**
	 * Our writers write only one variable at a time even though the Peano file
	 * format can hold multiple variables/sets
	 */
	virtual void writeFile(const visualisation::data::Variable& variable, const std::vector<visualisation::data::PatchData>& data) = 0;
};


#endif
