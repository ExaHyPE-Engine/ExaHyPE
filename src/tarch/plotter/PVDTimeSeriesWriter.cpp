#include <fstream>

#include "PVDTimeSeriesWriter.h"
#include "tarch/mpi/Rank.h"


namespace {
  tarch::logging::Log _log( "tarch::plotter::PVDTimeSeriesWriter" );

  const std::string Tail = "</Collection>\n</VTKFile>\n\n";
}


void tarch::plotter::PVDTimeSeriesWriter::createEmptyNewFile( const std::string& snapshotFileName ) {
  const std::string filename = snapshotFileName + ".pvd";

  std::vector<std::string> lines;

  lines.push_back( "<?xml version=\"1.0\"?>" );
  lines.push_back( "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">" );
  lines.push_back( "<Collection>" );

  addFileTail(lines);

  writeFile( filename, lines );
}


void tarch::plotter::PVDTimeSeriesWriter::addFileTail( std::vector<std::string>& lines ) {
  lines.push_back( "</Collection>" );
  lines.push_back( "</VTKFile>" );
}


void tarch::plotter::PVDTimeSeriesWriter::removeFileTail( std::vector<std::string>& lines ) {
  if (lines.back().find("</VTKFile>")!=std::string::npos) {
    lines.pop_back();
  }
  if (lines.back().find("</Collection>")!=std::string::npos) {
    lines.pop_back();
  }
}


std::tuple< int, int, std::vector<std::string> > tarch::plotter::PVDTimeSeriesWriter::parseFile( const std::string& filename) {
  std::ifstream in( filename );

  std::vector< std::string >  fileLines;
  int                         snapshot = 0;
  int                         part     = 1;

  std::string line;
  while (std::getline(in, line)) {
    fileLines.push_back(line);
    if (line.find("timestep=\"" + std::to_string(snapshot))!=std::string::npos) {
      snapshot++;
      part = 1;
    }
    if (line.find("timestep=\"" + std::to_string(snapshot-1))!=std::string::npos) {
      part++;
    }
  }
  snapshot--;
  part--;

  return std::tuple< int, int, std::vector<std::string> >(snapshot, part, fileLines);
}


void tarch::plotter::PVDTimeSeriesWriter::writeFile( const std::string& filename, const std::vector<std::string>& lines ) {
  std::ofstream out;
  out.open( filename.c_str() );
  //out.open( filename.c_str(), std::ios::binary );
  if ( (!out.fail()) and out.is_open() ) {
    for (auto& p: lines) {
      out << p << std::endl;
    }
  }
  else {
    logError( "createEmptyNewFile(std::string)", "failed to open " << filename );
  }
}


std::string tarch::plotter::PVDTimeSeriesWriter::createFileEntry( const std::string& snapshotFileName, int snapshotCounter, int partCounter ) {
  return "<DataSet timestep=\"" + std::to_string(snapshotCounter) + "\" group=\"\" part=\"" + std::to_string(partCounter) + "\" file=\"" + snapshotFileName + "\" />";
}


void tarch::plotter::PVDTimeSeriesWriter::validateFile( const std::string& filename ) {
  if (
    filename.find( ".vtu" )==std::string::npos
  ) {
    logWarning( "validateFile(std::string)", "expect file within time series to be an XML-based VTK file. Legacy VTK (ending with .vtk) is not supported by most Paraview installations" );
  }
}


void tarch::plotter::PVDTimeSeriesWriter::appendNewDataSet( const std::string& snapshotFileName, const std::string& dataFile ) {
  validateFile(dataFile);
  const std::string filename = snapshotFileName + ".pvd";

  auto file = parseFile( filename );

  int                        snapshotCounter = std::get<0>(file);
  int                        part            = std::get<1>(file);
  std::vector< std::string > lines           = std::get<2>(file);

  removeFileTail(lines);
  lines.push_back( createFileEntry(dataFile, snapshotCounter+1, 0) );
  addFileTail(lines);
  writeFile( filename, lines );
}


void tarch::plotter::PVDTimeSeriesWriter::appendNewData(const std::string& snapshotFileName, const std::string& dataFile) {
  validateFile(dataFile);
  const std::string filename = snapshotFileName + ".pvd";

  auto file = parseFile( filename );

  int                        snapshotCounter = std::get<0>(file);
  int                        part            = std::get<1>(file);
  std::vector< std::string > lines           = std::get<2>(file);

  removeFileTail(lines);
  lines.push_back( createFileEntry(dataFile, snapshotCounter, part+1) );
  addFileTail(lines);
  writeFile( filename, lines );
}
