#ifndef _VISUALISATION_DATA_DATA_SET_H_
#define _VISUALISATION_DATA_DATA_SET_H_

#include <string>
#include <vector>
#include <map>

#include "PatchData.h"

#include "tarch/logging/Log.h"


#include "Variable.h"


namespace visualisation {
  namespace input {
    class PeanoTextPatchFileReader;
    class PeanoTextMetaFileReader;
  }
}


namespace visualisation {
  namespace data {
    class DataSet;
  }
}


/**
 * A dataset is a wrapper around one big map. The map maps instances of Variable
 * onto vectors of DataSet.
 */
class visualisation::data::DataSet {
  public:
	DataSet();

	void free();

	/**
	 * This operation does not do a deep copy, i.e. the PatchData instances
	 * within other are not copied. So don't free them.
	 */
	void merge(const DataSet& other);

	/**
	 * @return Set of readers tied to a particular model. These readers haven't
	 *         parsed their files yet, i.e. you have to call parse() manually.
	 */
    std::vector<PatchData>  getData( const Variable& selector ) const;

	/**
	 * @return All the selectors (data representations) stored for a time step
	 */
	std::vector<Variable>  getVariables() const;
	bool                   hasVariable( const std::string& name ) const;
	Variable               getVariable( const std::string& name ) const;

	/**
	 * If you add a patch, don't free the patch, i.e. this operation does not
	 * do a deep copy.
	 */
	void add( Variable variable, const std::vector<PatchData>& patchData );
  private:
	friend class visualisation::input::PeanoTextPatchFileReader;
	friend class visualisation::input::PeanoTextMetaFileReader;

	static tarch::logging::Log  _log;

    std::map< Variable, std::vector<PatchData> >  data;
};

#endif
