#ifndef _CONVERT_OUTPUT_PEANO_WRITER_H_
#define _CONVERT_OUTPUT_PEANO_WRITER_H_

#include "convert/data/Variable.h"


#include "Writer.h"

#include "../config.h"

#include "tarch/logging/Log.h"

#include <vector>
#include <fstream>
#include "../data/PatchData.h"
#include "../data/DataSet.h"

namespace convert {
  namespace output {
    class PeanoWriter;
  }

  namespace input {
    /**
     * Forward declaration
     */
    class PeanoTextPatchFileReader;
  }
}


class convert::output::PeanoWriter: public convert::output::Writer {
  public:
	PeanoWriter(const std::string&  directory, const std::string& outputFileWithoutExtention);
	~PeanoWriter();

	void writeFile(const convert::data::Variable& variable, const std::vector<convert::data::PatchData>& data) override;
	void writeFile(const convert::data::DataSet& dataSet);
	void writeFile(const std::vector< convert::data::DataSet > dataSet);
  private:
    void writeFile(const convert::data::DataSet& dataSet, const std::string& filename);

	static tarch::logging::Log  _log;

	static const std::string _FileExtension;
	static const std::string _Header;

	const std::string& _outputFileWithoutExtension;
	const std::string& _directory;
	std::ofstream      _metaFile;

	void writeVariableDeclaration(const convert::data::Variable& variable, std::ofstream& );
	void writePatchData(const convert::data::Variable& variable, const convert::data::PatchData& data, std::ofstream&  );
};

#endif
