#include "PeanoTextPatchFileWriter.h"
#include "tarch/la/ScalarOperations.h"


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::VertexDataWriter(
  const std::string& identifier,
  int                unknownsPerAxis,
  int                numberOfUnknowns, const std::string& description,
  const std::string& metaData,
  double*            mapping,
  tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& writer
):
  _writer(writer),
  _identifier(identifier),
  _numberOfVerticesPerAxis(unknownsPerAxis),
  _numberOfUnknowns(numberOfUnknowns),
  _entryCounter(0) {
  _writer._snapshotFileOut << "begin vertex-metadata \"" << identifier << "\"" << std::endl
                           << "  number-of-unknowns " << _numberOfUnknowns << std::endl
                           << "  number-of-dofs-per-axis " << _numberOfVerticesPerAxis << std::endl
                           << "  description \"" << description << "\"" << std::endl;

  _writer.writeMetaData(metaData);
  _writer.writeMapping(getVerticesPerPatch(),mapping);

  _writer._snapshotFileOut << "end vertex-metadata" << std::endl << std::endl;
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::~VertexDataWriter() {
}


int tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::getVerticesPerPatch() const {
  return tarch::la::aPowI(_writer._dimensions,_numberOfVerticesPerAxis);
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::plotVertex( int index, double value ) {
  for (int i=0; i<_numberOfUnknowns; i++) {
    if ( std::isnan(value) ) {
      _out << " nan";
    }
    else {
      if ( std::abs(value) < std::numeric_limits<double>::min() ) value = 0.0;
      if (tarch::la::equals(value,0.0)) {
        _out << " 0";
      }
      else {
        _out << " " << value;
      }
    }
    _entryCounter++;
  }
  flushIfPatchIsComplete();
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::plotVertex( int index, double* values ) {
  for (int i=0; i<_numberOfUnknowns; i++) {
    if ( std::isnan(values[i]) ) {
      _out << " nan";
    }
    else {
      if ( std::abs(values[i]) < std::numeric_limits<double>::min() ) values[i] = 0.0;
      if (tarch::la::equals(values[i],0.0)) {
        _out << " 0";
      }
      else {
        _out << " " << values[i];
      }
    }
    _entryCounter++;
  }
  flushIfPatchIsComplete();
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::close() {
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::assignRemainingVerticesDefaultValues() {
/*
  while (_entryCounter<getVerticesPerPatch()) {
    _out << std::flush;
    _writer._snapshotFileOut << "  begin vertex-values \"" << _identifier << "\"" << std::endl
                 << "    " << _out.rdbuf() << std::endl
                 << "  end vertex-values" << std::endl;
    _out.clear();
    _entryCounter = 0;
  }
*/
	assertionMsg(false, "not yet implemented" );
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::flushIfPatchIsComplete() {
  if (_entryCounter>=getVerticesPerPatch()*_numberOfUnknowns) {
    _out << std::flush;
    _writer._snapshotFileOut << "  begin vertex-values \"" << _identifier << "\"" << std::endl
                 << "    " << _out.rdbuf() << std::endl
                 << "  end vertex-values" << std::endl;
    _out.clear();
    _entryCounter = 0;
  }
}



int tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::VertexDataWriter::getFirstVertexWithinPatch(int index) const {
  return index * getVerticesPerPatch();
}


