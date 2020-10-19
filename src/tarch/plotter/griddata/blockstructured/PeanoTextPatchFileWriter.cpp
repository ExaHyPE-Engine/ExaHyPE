#include <fstream>

#include "PeanoTextPatchFileWriter.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Lock.h"


tarch::logging::Log           tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::_log( "tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter" );
tarch::mpi::BooleanSemaphore  tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::_sempahore( "tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter" );


const std::string tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::HEADER =
"# \n" \
"# Peano patch file \n" \
"# Version 0.2 \n" \
"# \n";


namespace {
  const std::string Token_BeginDataSet = "begin dataset";
  const std::string Token_EndDataSet   = "end dataset";
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::addNewDatasetToIndexFile() {
  tarch::mpi::Lock  lock( _sempahore );
  std::ofstream     indexFileOut;
  indexFileOut.open( (_indexFile + ".peano-patch-file").c_str(), std::ios::app );
  indexFileOut << std::endl << Token_BeginDataSet << std::endl;
  indexFileOut << std::endl << Token_EndDataSet << std::endl;
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::addNewFileToCurrentDataSetInIndexFile( const std::string&  filename ) {
  tarch::mpi::Lock  lock( _sempahore );

  std::ifstream ifs(_indexFile + ".peano-patch-file");
  std::vector<std::string> lines;
  for (std::string line; std::getline(ifs, line); /**/ ) {
    lines.push_back(line);
  }
  ifs.close();

  if ( lines[lines.size()-1].find( Token_EndDataSet )==std::string::npos ) {
    logWarning(
      "addNewFileToCurrentDataSetInIndexFile(...)",
      "last line in index file " << _indexFile << ".peano-patch-file has not been end of dataset as expected. Expected \"" << Token_EndDataSet << "\" but got \"" << lines[lines.size()-1] << "\"" );
  }
  else {
    const std::string newEntry = "  include \"" + filename + "\"";
    lines[lines.size()-1] = newEntry;
    lines.push_back(Token_EndDataSet);
    std::ofstream     indexFileOut;
    indexFileOut.open( (_indexFile + ".peano-patch-file").c_str(), std::ios::out );
    for (auto& p: lines) {
      indexFileOut << p << std::endl;
    }
  }
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createEmptyIndexFile() {
  if (_indexFile.rfind(".peano-patch-file")!=std::string::npos) {
    logWarning( "PeanoTextPatchFileWriter()", "filename should not end with .peano-patch-file as routine adds extension automatically. Chosen filename=" << _indexFile );
  }

  tarch::mpi::Lock  lock( _sempahore );
  std::ofstream     indexFileOut;
  indexFileOut.open( (_indexFile + ".peano-patch-file").c_str(), std::ios::out );

  if ( (!indexFileOut.fail()) && indexFileOut.is_open() ) {
    indexFileOut << HEADER
                 << "format ASCII" << std::endl;

    indexFileOut << std::endl << Token_BeginDataSet << std::endl;
    indexFileOut << std::endl << Token_EndDataSet << std::endl;
  }

  if ( !indexFileOut.is_open() ) {
    logError("PeanoTextPatchFileWriter()", "have not been able to open file " << _indexFile << ".peano-patch-file");
  }
  indexFileOut.close();
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::PeanoTextPatchFileWriter(
  int dimension, const std::string&  fileName, const std::string&  indexFileName, IndexFileMode appendToIndexFile
):
  _writtenToFile(false),
  _dimensions(dimension),
  _fileName(fileName),
  _indexFile(indexFileName) {
  assertion( dimension>=2 );
  assertion( dimension<=3 );

  if (fileName.rfind(".peano-patch-file")!=std::string::npos) {
    logWarning( "writeToFile()", "filename should not end with .peano-patch-file as routine adds extension automatically. Chosen filename prefix=" << fileName );
  }
  if (indexFileName.rfind(".peano-patch-file")!=std::string::npos) {
    logWarning( "writeToFile()", "index filename should not end with .peano-patch-file as routine adds extension automatically. Chosen filename prefix=" << indexFileName );
  }

  clear();

  switch (appendToIndexFile) {
    case IndexFileMode::CreateNew:
      createEmptyIndexFile();
      break;
    case IndexFileMode::AppendNewDataSet:
      addNewDatasetToIndexFile();
      break;
    case IndexFileMode::AppendNewData:
      break;
  }
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::~PeanoTextPatchFileWriter() {
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::writeMapping(int totalEntries, double* values) {
  if (values!=nullptr) {
    _snapshotFileOut << "  begin mapping" << std::endl;
    for (int i=0; i<totalEntries * _dimensions; i++) {
      _snapshotFileOut << " " << values[i];
    }
    _snapshotFileOut << std::endl << "  end mapping" << std::endl;
  }
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::writeMetaData(const std::string& metaData) {
  if (!metaData.empty()) {
    _snapshotFileOut << "  begin meta-data" << std::endl
                     << "    " << metaData << std::endl
                     << "  end meta-data" << std::endl;
  }
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*
tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& description ) {
  return new CellDataWriter(identifier, unknownsPerAxis, recordsPerCell, description, "", nullptr, *this);
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*
tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& description, const std::string& metaData ) {
  return new CellDataWriter(identifier, unknownsPerAxis, recordsPerCell, description, metaData, nullptr, *this);
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter*
tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createCellDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerCell, const std::string& description, const std::string& metaData, double* mapping ) {
  return new CellDataWriter(identifier, unknownsPerAxis, recordsPerCell, description,  metaData, mapping, *this);
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*
tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& description ) {
  return new VertexDataWriter(identifier, unknownsPerAxis, recordsPerVertex , description, "", nullptr, *this);
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*
tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& description, const std::string& metaData ) {
  return new VertexDataWriter(identifier, unknownsPerAxis, recordsPerVertex , description, metaData, nullptr, *this);
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter*
tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::createVertexDataWriter( const std::string& identifier, int unknownsPerAxis, int recordsPerVertex, const std::string& description, const std::string& metaData, double* mapping ) {
  return new VertexDataWriter(identifier, unknownsPerAxis, recordsPerVertex , description, metaData, mapping, *this);
}


int tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::plotPatch(
  const tarch::la::Vector<2,double>& offset,
  const tarch::la::Vector<2,double>& size
) {
  if (_haveWrittenAtLeastOnePatch) {
    _snapshotFileOut << "end patch" << std::endl << std::endl;
  }

  _snapshotFileOut << "begin patch" << std::endl
       << "  offset";

  for (int d=0; d<2; d++) {
    _snapshotFileOut << " " << offset(d);
  }
  if (_dimensions==3) {
    _snapshotFileOut << " 0";
  }
  _snapshotFileOut << std::endl;

  _snapshotFileOut << "  size";

  for (int d=0; d<2; d++) {
    _snapshotFileOut << " " << size(d);
  }
  if (_dimensions==3) {
    _snapshotFileOut << " 0";
  }
  _snapshotFileOut << std::endl;

  _haveWrittenAtLeastOnePatch = true;

  _patchCounter++;
  return _patchCounter-1;
}


int tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::plotPatch(
  const tarch::la::Vector<3,double>& offset,
  const tarch::la::Vector<3,double>& size
) {
  assertion( _dimensions==3 );

  if (_haveWrittenAtLeastOnePatch) {
    _snapshotFileOut << "end patch" << std::endl << std::endl;
  }

  _snapshotFileOut << "begin patch" << std::endl
       << "  offset";

  for (int d=0; d<3; d++) {
    _snapshotFileOut << " " << offset(d);
  }
  _snapshotFileOut << std::endl;

  _snapshotFileOut << "  size";

  for (int d=0; d<3; d++) {
    _snapshotFileOut << " " << size(d);
  }
  _snapshotFileOut << std::endl;

  _haveWrittenAtLeastOnePatch = true;

  _patchCounter++;
  return _patchCounter-1;
}


bool tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::writeToFile() {
  assertion( !_writtenToFile );

  std::ostringstream filenameStream;
  filenameStream << _fileName
    #ifdef Parallel
                 << "-rank-" << tarch::mpi::Rank::getInstance().getRank()
    #endif
                 << ".peano-patch-file";
  const std::string filename = filenameStream.str();

  std::ofstream out;
  out.open( filename.c_str(), std::ios::binary );
  if ( (!out.fail()) && out.is_open() ) {
    _log.debug( "writeToFile()", "opened data file " + filename );

    if (_haveWrittenAtLeastOnePatch) {
      _snapshotFileOut << "end patch" << std::endl << std::endl;
    }

    out << _snapshotFileOut.rdbuf();

    addNewFileToCurrentDataSetInIndexFile(filename);

    _writtenToFile = true;
    return true;
  }
  else {
    _log.error( "close()", "unable to write output file " + filename );
    return false;
  }
}


bool tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::isOpen() {
  return !_writtenToFile;
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::clear() {
  _writtenToFile       = false;

  _snapshotFileOut.clear();

  _patchCounter = 0;

  _haveWrittenAtLeastOnePatch = false;

  _snapshotFileOut << HEADER
      << "format ASCII" << std::endl
      << "dimensions " << _dimensions  << std::endl;

  _snapshotFileOut << std::endl << std::endl;
}
