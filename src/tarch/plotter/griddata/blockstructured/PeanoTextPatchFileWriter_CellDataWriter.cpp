#include "PeanoTextPatchFileWriter.h"
#include "tarch/la/ScalarOperations.h"


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::CellDataWriter(
  const std::string& identifier,
  int                unknownsPerAxis,
  int                numberOfUnknowns,
  const std::string& metaData,
  double*            mapping,
  tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter& writer
):
  _writer(writer),
  _identifier(identifier),
  _numberOfCellsPerAxis(unknownsPerAxis),
  _numberOfUnknowns(numberOfUnknowns),
  _entryCounter(0) {
  _writer._snapshotFileOut << "begin cell-metadata \"" << identifier << "\"" << std::endl
               << "  number-of-unknowns " << _numberOfUnknowns << std::endl
               << "  number-of-dofs-per-axis " << _numberOfCellsPerAxis << std::endl;

  _writer.writeMetaData(metaData);
  _writer.writeMapping(getCellsPerPatch(),mapping);

  _writer._snapshotFileOut << "end cell-metadata" << std::endl << std::endl;
}


tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::~CellDataWriter() {
}


int tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::getCellsPerPatch() const {
  return tarch::la::aPowI(_writer._dimensions,_numberOfCellsPerAxis);
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::plotCell( int index, double value ) {
  for (int i=0; i<_numberOfUnknowns; i++) {
    if (std::isnan(value)) {
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


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::plotCell( int index, double* values ) {
  for (int i=0; i<_numberOfUnknowns; i++) {
    if (std::isnan(values[i])) {
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


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::close() {
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::flushIfPatchIsComplete() {
  if (_entryCounter>=getCellsPerPatch()*_numberOfUnknowns) {
    _out << std::flush;
    _writer._snapshotFileOut << "  begin cell-values \"" << _identifier << "\"" << std::endl
                 << "    " << _out.rdbuf() << std::endl
                 << "  end cell-values" << std::endl;
    _out.clear();
    _entryCounter = 0;
  }
}


void tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::assignRemainingCellsDefaultValues() {
	assertionMsg( false, "not implemented" );
}


int tarch::plotter::griddata::blockstructured::PeanoTextPatchFileWriter::CellDataWriter::getFirstCellWithinPatch(int index) const {
  return index * getCellsPerPatch();
}


