#ifndef _VISUALISATION_OUTPUT_PEANO_WRITER_H_
#define _VISUALISATION_OUTPUT_PEANO_WRITER_H_

#include "visualisation/data/Variable.h"


#include "Writer.h"

#include "../config.h"

#include "tarch/logging/Log.h"

#include <vector>
#include <fstream>
#include "../data/PatchData.h"
#include "../data/DataSet.h"

namespace visualisation {
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


class visualisation::output::PeanoWriter: public visualisation::output::Writer {
  public:
	PeanoWriter(const std::string&  directory, const std::string& outputFileWithoutExtention);
	~PeanoWriter();

	void writeFile(const visualisation::data::Variable& variable, const std::vector<visualisation::data::PatchData>& data) override;
	void writeFile(const visualisation::data::DataSet& dataSet);
	void writeFile(const std::vector< visualisation::data::DataSet > dataSet);
  private:
	void writeFile(const visualisation::data::DataSet& dataSet, const std::string& filename);

	static tarch::logging::Log  _log;

	static const std::string _FileExtension;
	static const std::string _Header;

	const std::string& _outputFileWithoutExtension;
	const std::string& _directory;
	std::ofstream      _metaFile;

	void writeVariableDeclaration(const visualisation::data::Variable& variable, std::ofstream& );
	void writePatchData(const visualisation::data::Variable& variable, const visualisation::data::PatchData& data, std::ofstream&  );
};

#endif
