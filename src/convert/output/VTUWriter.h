#ifndef _VISUALISATION_OUTPUT_VTU_WRITER_H_
#define _VISUALISATION_OUTPUT_VTU_WRITER_H_

#include "Writer.h"

#include "../config.h"
#include "../data/PatchData.h"

#include "tarch/logging/Log.h"


#ifdef UseVTK
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#endif


#include <vector>


namespace visualisation {
  namespace output {
    class VTUWriter;
  }

  namespace input {
    /**
     * Forward declaration
     */
    class PeanoTextPatchFileReader;
  }
}


class visualisation::output::VTUWriter: public visualisation::output::Writer {
  public:
	VTUWriter(const std::string&  directory, const std::string& outputFileWithoutExtention);
	~VTUWriter();

	void writeFile(const visualisation::data::Variable& variable, const std::vector<visualisation::data::PatchData>& data) override;

	/**
	 * Meta file write
	 */
/*
	void writeFile(const PeanoMetaFile& metaFile, const std::string& selector) override;

	static void writeFile(const std::string& outputFileWithoutExtension, const std::vector<PeanoPatch*>& patches);
*/
  private:
    static tarch::logging::Log  _log;

    const std::string& _outputFileWithoutExtension;
    const std::string& _directory;

    #ifdef UseVTK
    /**
     * Maps a set of patches onto one unstructured grids.
     */
//    static vtkSmartPointer<vtkUnstructuredGrid> combine(const std::vector<PeanoPatch*>& patches);

	/**
	 * Convert one patch into a regular grid, i.e. an image.
	 */
	static vtkSmartPointer<vtkImageData> toImageData(const visualisation::data::Variable& variable, const visualisation::data::PatchData& data);

	/**
	 * Basic routine.
	 *
	 * Take an individual patch and convert it into an unstructured grid.
	 */
	static vtkSmartPointer<vtkUnstructuredGrid> toUnstructuredGrid(const visualisation::data::Variable& variable, const visualisation::data::PatchData& data);

    static vtkSmartPointer<vtkDoubleArray> getVTUDataForOnePatch(const visualisation::data::Variable& variable, const visualisation::data::PatchData& data);

    /**
     * Creates data for the patch but instead of the patch's data, it writes the given
     * value instead. I need this to visualise the tree association, e.g.
     */
    static vtkSmartPointer<vtkDoubleArray> getMetaDataForOnePatch(const visualisation::data::Variable& variable, const visualisation::data::PatchData& data);
    #endif

	/**
	 * Helper function that we use extensively when we map a patch within one
	 * cell onto an unstructured grid fragment.
	 */
	static int xyzToIndex(int x, int y, int z, int verticesPerAxis);
};

#endif
