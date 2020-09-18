#ifndef _CONVERT_INPUT_PEANO_TEXT_PATCH_FILE_READER_H_
#define _CONVERT_INPUT_PEANO_TEXT_PATCH_FILE_READER_H_



#include <string>
#include <vector>

#include "../data/PatchData.h"
#include "PatchFileReader.h"


#include "convert/data/DataSet.h"
#include "convert/data/Variable.h"


#include "tarch/logging/Log.h"


namespace convert {
  namespace input {
    class PeanoTextPatchFileReader;
  }
}


/**
 * Represent a Peano reader
 *
 * A reader is an object which basically takes one file and gives the content
 * of this file.
 *
 * If you just want to read in one file, open it with this file and pass the field
 * patches into the PeanoConverter. So this class has two purposes:
 *
 * - It can read a single file. In this case, getData() gives you back only one
 *   entry.
 * - It can read a set of datasets (meta file). In this case, getData() gives you
 *   back a sequence of datasets. Internally, the reader creates new readers.
 *
 * At the moment, a file either includes other files and thus holds a sequence of
 * data sets, or it holds real data. Mixtures are not allowed. Therefore, there's
 * no recursion of reads.
 *
 * @author Tobias Weinzierl
 */
class convert::input::PeanoTextPatchFileReader: public convert::input::PatchFileReader {
  private:
    static tarch::logging::Log _log;

    const std::string  _file;

    void    parsePatch( int dataSetCounter, int treeNumber, const std::vector<std::string>& patchDescription );
    void    parseVariablesDeclaration( int dataSetCounter, const std::vector<std::string>& patchDescription, const std::string&  name, convert::data::PeanoDataType type );

    void    addDataToPatch( int dataSetCounter, const std::string& variableName, double* offset, double* size, int treeNumber, const std::vector< std::string >& textData );

    int     extractTreeNumberFromFileName() const;

    int                                          _dimensions;
    std::vector< convert::data::DataSet >  _data;
  public:
/**
 * Read in one file.
 *
 * \section Parse process
 *
 * - Load the whole files into a vector called lines, i.e. we load line by line.
 * -
 */
PeanoTextPatchFileReader(const std::string &file);
    virtual ~PeanoTextPatchFileReader();

    void parse() override;

	/**
	 * A file can hold a series of datasets
	 */
	std::vector< convert::data::DataSet >  getData() const override;
};

#endif
