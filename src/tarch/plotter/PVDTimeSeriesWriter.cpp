#include <fstream>

#include "PVDTimeSeriesWriter.h"
#include "tarch/mpi/Rank.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/Assertions.h"


namespace {
  tarch::logging::Log _log( "tarch::plotter::PVDTimeSeriesWriter" );

  const std::string Tail = "</Collection>\n</VTKFile>\n\n";

  double TimeStampPrecision = 1e-5;
}


double tarch::plotter::PVDTimeSeriesWriter::getLatestTimeStepInIndexFile( std::string  dataFile ) {
  if ( dataFile.find( ".pvd" )==std::string::npos ) {
    dataFile += ".pvd";
  }

  std::tuple< double, int, std::vector<std::string> > fileData = parseFile( dataFile );
  double result = std::get<0>(fileData);
  logDebug( "getLatestTimeStepInIndexFile(string)", "latest timestamp is " << result );
  return result;
}


void tarch::plotter::PVDTimeSeriesWriter::createEmptyIndexFile( const std::string& snapshotFileName ) {
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


std::tuple< double, int, std::vector<std::string> > tarch::plotter::PVDTimeSeriesWriter::parseFile( const std::string& filename) {
  std::ifstream in( filename );

  std::vector< std::string >  fileLines;
  double                      timestamp = 0;
  int                         part      = -1;

  std::string line;
  while (std::getline(in, line)) {
    fileLines.push_back(line);
    if ( line.find("<DataSet")!=std::string::npos ) {
      std::size_t startTimeStep = line.find( "timestep=\"" ) + std::string("timestep=\"").size();
      std::size_t endTimeStep   = line.find( "\"", startTimeStep+1 );
      std::string tokenTimeStep = line.substr(startTimeStep, endTimeStep-startTimeStep );

      std::size_t startPart = line.find( "part=\"" ) + std::string("part=\"").size();
      std::size_t endPart   = line.find( "\"", startPart+1 );
      std::string tokenPart = line.substr(startPart, endPart-startPart );

      double currentTimeStep = std::atof( tokenTimeStep.c_str() );
      int    currentPart     = std::atoi( tokenPart.c_str() );

      if (tarch::la::smaller(timestamp,currentTimeStep,TimeStampPrecision)) {
        timestamp = currentTimeStep;
        part      = currentPart;
      }
      else if (tarch::la::equals(timestamp,currentTimeStep,TimeStampPrecision)) {
        part = std::max(part, currentPart);
      }
    }
  }

  return std::tuple< double, int, std::vector<std::string> >(timestamp, part, fileLines);
}


void tarch::plotter::PVDTimeSeriesWriter::writeFile( const std::string& filename, const std::vector<std::string>& lines ) {
  std::ofstream out;
  out.open( filename.c_str() );
  if ( (!out.fail()) and out.is_open() ) {
    for (auto& p: lines) {
      out << p << std::endl;
    }
  }
  else {
    logError( "createEmptyNewFile(std::string)", "failed to open " << filename );
  }
}


std::string tarch::plotter::PVDTimeSeriesWriter::createFileEntry( const std::string& snapshotFileName, double timestamp, int partCounter ) {
  std::string newEntry = "<DataSet timestep=\"" + std::to_string(timestamp) + "\" group=\"\" part=\"" + std::to_string(partCounter) + "\" file=\"" + snapshotFileName + "\" />";
  logDebug( "createFileEntry(...)", "added " << newEntry );
  return newEntry;
}


void tarch::plotter::PVDTimeSeriesWriter::appendNewData(const std::string& snapshotFileName, const std::string& dataFile, double timeStamp) {
  const std::string filename = snapshotFileName + ".pvd";

  auto file = parseFile( filename );

  double                     latestTimeStep  = std::get<0>(file);
  int                        part            = std::get<1>(file);
  std::vector< std::string > lines           = std::get<2>(file);

  removeFileTail(lines);

  if ( tarch::la::equals(latestTimeStep,timeStamp,TimeStampPrecision) ) {
    lines.push_back( createFileEntry(dataFile, timeStamp, part+1) );
  }
  else {
    lines.push_back( createFileEntry(dataFile, timeStamp, 0) );
  }

  logDebug( "appendNewDataSet(...)", "no of lines=" << lines.size() );

  addFileTail(lines);
  writeFile( filename, lines );
}
