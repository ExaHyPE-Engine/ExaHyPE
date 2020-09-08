#include <string>
#include <unordered_map>
#include <vector>
#include <list>

#include "../config.h"
#include "visualisation/input/PeanoTextPatchFileReader.h"

#include "tarch/Assertions.h"

#include "PeanoWriter.h"


tarch::logging::Log  visualisation::output::PeanoWriter::_log( "visualisation::output::PeanoWriter" );
const std::string    visualisation::output::PeanoWriter::_FileExtension( ".peano-patch-file" );
const std::string    visualisation::output::PeanoWriter::_Header( "# \n# Peano patch file\n# Version 0.2\n# Written by Peano's visualisation/conversion tool\n#\nformat ASCII\n" ) ;


visualisation::output::PeanoWriter::PeanoWriter(const std::string&  directory, const std::string& outputFileWithoutExtension):
  _outputFileWithoutExtension(outputFileWithoutExtension),
  _directory(directory),
  _metaFile(directory + "/" + outputFileWithoutExtension+_FileExtension) {
  _metaFile << _Header << std::endl;
}


visualisation::output::PeanoWriter::~PeanoWriter() {
  _metaFile.close();
}


/*
void visualisation::output::PeanoWriter::writeFile(const visualisation::data::DataSet& data, const std::string& selector) {
  for( int timeStep = 0; timeStep<metaFile.getNumberOfDataSets(); timeStep++ ) {
    _metaFile << "begin dataset" << std::endl;

	for ( auto selector: metaFile.getStoredDataRepresentations(timeStep) ) {
	  std::vector<PeanoPatch*>  p = metaFile.getData( timeStep, selector );
      logInfo( "convertFile(...)", "process " << p.size() << " patches with selector " << selector );

      if (!p.empty()) {
    	  // @todo in the future, we might wanna split up the patches into subdirectories
        //std::string filename = _outputFileWithoutExtension + "-" + std::to_string(timeStep) + "-" + std::to_string(i);
    	// @todo Perhaps raw ito filename?
        std::string filename = _outputFileWithoutExtension + "-" + std::to_string(timeStep);

        #pragma omp critical
        {
          if (selector==PeanoMetaFile::RawData) {
            _metaFile << "  include \"" << filename << _FileExtension << "\"" << std::endl;
          }
          else {
           _metaFile << "  include \"" << selector << "\" \"" << filename << _FileExtension << "\"" << std::endl;
          }
        }
        writeFile(_directory + "/" + filename, p);
      }
	}

    _metaFile << "end dataset" << std::endl << std::endl;
  }
}


void visualisation::output::PeanoWriter::writeFile(const PeanoMetaFile&  metaFile, const std::string& selector) {
  for( int timeStep = 0; timeStep<metaFile.getNumberOfDataSets(); timeStep++ ) {
	std::vector<PeanoPatch*> p = metaFile.getData( timeStep, selector );

//    logInfo( "convertFile(...)", "time step consists of " << readers.size() << " data set(s)" );

    _metaFile << "begin dataset" << std::endl;

//    #pragma omp parallel for
    if (!p.empty()) {
//      std::string filename = _outputFileWithoutExtension + "-" + std::to_string(timeStep) + "-" + std::to_string(i);
      std::string filename = _outputFileWithoutExtension + "-" + std::to_string(timeStep);

//        #pragma omp critical
      {
        _metaFile << "  include \"" << filename << _FileExtension << "\"" << std::endl;
      }

      writeFile(_directory + "/" + filename, p);
    }

    _metaFile << "end dataset" << std::endl << std::endl;
  }
}
*/


void visualisation::output::PeanoWriter::writeFile(const std::vector<visualisation::data::DataSet> dataSet) {
  const int numberOfDataSets = dataSet.size();
  #pragma omp parallel for
  for (int i=0; i<numberOfDataSets; i++) {
    const std::string outputFileName = numberOfDataSets==1 ? _outputFileWithoutExtension : _outputFileWithoutExtension + "-" + std::to_string(i);
    writeFile( dataSet[i], _directory + "/" + outputFileName + _FileExtension );
  }

  if (numberOfDataSets>1) {
    std::string outputFileName = _directory + "/" + _outputFileWithoutExtension + _FileExtension;
    logInfo( "writeFile(...)", "write meta file " << outputFileName );
    std::ofstream  file( outputFileName );
    file << _Header << std::endl;
    for (int i=0; i<numberOfDataSets; i++) {
	  const std::string outputFileName = _outputFileWithoutExtension + "-" + std::to_string(i) + _FileExtension;
      file << "begin dataset" << std::endl;
      file << "  include \"" << outputFileName << "\"" << std::endl;
      file << "end dataset" << std::endl << std::endl;
    }
  }
  else {
    logInfo( "writeFile(...)", _directory << "/" << _outputFileWithoutExtension << " is a single data set. Thus, no meta data file is written");
  }
}


void visualisation::output::PeanoWriter::writeFile(const visualisation::data::DataSet& dataSet) {
  std::string outputFileName = _directory + "/" + _outputFileWithoutExtension + _FileExtension;
  writeFile( dataSet, outputFileName );
}


void visualisation::output::PeanoWriter::writeFile(const visualisation::data::DataSet& dataSet, const std::string& filename) {
  std::ofstream  file( filename );
  logInfo( "writeFile(...)", "start to write to file " << filename );

  file << _Header << std::endl;

  if (dataSet.getVariables().size()>0) {
    file << "dimensions " << dataSet.getVariables()[0].dimensions << std::endl << std::endl;
  }

  std::list< visualisation::data::PatchData > allPatches;
  for (auto p: dataSet.getVariables()) {
    writeVariableDeclaration(p,file);
    auto newData = dataSet.getData(p);
    std::copy(newData.begin(), newData.end(), std::back_inserter(allPatches));
  }

  while (not allPatches.empty()) {
    visualisation::data::PatchData currentPatch = allPatches.back();

    // remove patch from all patches
    for (
      std::list< visualisation::data::PatchData >::iterator p = allPatches.begin();
      p != allPatches.end();
    ) {
      if (p->samePatch(currentPatch)) {
    	p = allPatches.erase( p );
      }
      else {
    	p++;
      }
    }

    file << "begin patch " << std::endl;
    file << "  offset";
    for (int d=0; d<currentPatch.dimensions; d++)
      file << " " << currentPatch.offset[d];
    file << std::endl;
    file << "  size";
    for (int d=0; d<currentPatch.dimensions; d++)
      file << " " << currentPatch.size[d];
    file << std::endl;

    for (auto p: dataSet.getVariables()) {
      for (auto pp: dataSet.getData(p)) {
        if (pp.samePatch(currentPatch)) {
          writePatchData( p, pp, file );
        }
      }
    }


    file << "end patch" << std::endl << std::endl;
  }

  file.close();

  logInfo( "writeFile(...)", "written to file " << filename );
}


void visualisation::output::PeanoWriter::writePatchData(const visualisation::data::Variable& variable, const visualisation::data::PatchData& p, std::ofstream& file ) {
    file << "  begin ";
    if ( variable.type==visualisation::data::PeanoDataType::Cell_Values) {
      file << "cell-values ";
    }
    else {
      file << "vertex-values ";
    }
    file << "\"" << variable.name << "\"" << std::endl;
    file << "    ";
    for (int i=0; i<variable.getTotalNumberOfQuantitiesPerPatch(); i++) {
      file << " " << std::to_string( p.data[i] );
    }
    file << std::endl;
    file << "  end ";
    if ( variable.type==visualisation::data::PeanoDataType::Cell_Values) {
      file << "cell-values ";
    }
    else {
      file << "vertex-values ";
    }
    file << std::endl;
}


void visualisation::output::PeanoWriter::writeFile(const visualisation::data::Variable& variable, const std::vector<visualisation::data::PatchData>& patches) {
  std::string outputFileName = _directory + "/" + _outputFileWithoutExtension + _FileExtension;
  std::ofstream  file( outputFileName );
  logInfo( "writeFile(...)", "write to file " << outputFileName );

  file << _Header << std::endl;

  file << "dimensions " << variable.dimensions << std::endl << std::endl;

  writeVariableDeclaration( variable, file );

  for (auto p: patches) {
    file << "begin patch " << std::endl;
    file << "  offset";
    for (int d=0; d<variable.dimensions; d++)
      file << " " << p.offset[d];
    file << std::endl;
    file << "  size";
    for (int d=0; d<variable.dimensions; d++)
      file << " " << p.size[d];
    file << std::endl;

    writePatchData( variable, p, file );

    file << "end patch" << std::endl << std::endl;
  }

  file.close();
}


void visualisation::output::PeanoWriter::writeVariableDeclaration(const visualisation::data::Variable& variable, std::ofstream& file) {
  file << "begin ";
  if ( variable.type==visualisation::data::PeanoDataType::Cell_Values) {
    file << "cell-values ";
  }
  else {
    file << "vertex-values ";
  }
  file << "\"" << variable.name << "\"" << std::endl;
  file << "  number-of-dofs-per-axis " << variable.dofsPerAxis << std::endl;
  file << "  number-of-unknowns      " << variable.unknowns << std::endl;
  file << "end ";
  if ( variable.type==visualisation::data::PeanoDataType::Cell_Values) {
    file << "cell-values ";
  }
  else {
    file << "vertex-values ";
  }
  file << std::endl << std::endl;
}
