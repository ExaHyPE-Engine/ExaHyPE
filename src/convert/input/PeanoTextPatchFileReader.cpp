#include <string>
#include <iostream>
#include <vector>
#include <fstream>


#include "Parser.h"
#include "PeanoTextPatchFileReader.h"


#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"

#include "tarch/multicore/multicore.h"
#include "tarch/mpi/Rank.h"


tarch::logging::Log convert::input::PeanoTextPatchFileReader::_log( "convert::input::PeanoTextPatchFileReader" );


convert::input::PeanoTextPatchFileReader::PeanoTextPatchFileReader(const std::string &file):
  _dimensions(-1),
  _file(file) {
}


convert::input::PeanoTextPatchFileReader::~PeanoTextPatchFileReader() {
}


int convert::input::PeanoTextPatchFileReader::extractTreeNumberFromFileName() const {
  int result = -1;
  const std::string prefix    = "tree";
  const std::string separator = "-";
  std::size_t start = _file.find( "tree" + separator );
  if (start != std::string::npos) {
    start += std::string("tree" + separator).length();
    std::size_t end = _file.find( separator, start );
    std::string identifier = _file.substr( start, end-start );
    result = std::atoi( identifier.c_str() );
  }
  return result;
}


void convert::input::PeanoTextPatchFileReader::parse() {
  logInfo( "parse()", "reading file " << _file );

  bool isFirstDataSet = true;

  std::vector< PeanoTextPatchFileReader > subReaders;

  // Push in default guy
  _data.push_back( convert::data::DataSet() );

  std::ifstream ifs(_file);
  std::vector<std::string> lines;
  for (std::string line; std::getline(ifs, line); /**/ ) {
    lines.push_back(line);
  }
  ifs.close();

  if (lines.empty()) {
    logError( "parse()", "file " << _file << " is empty (or does not exist)" );
  }

  int treeNumber = extractTreeNumberFromFileName();
  logDebug( "parse()", "file " << _file << " will yield data with tree number " << treeNumber );

  {
  for(uint i = 0; i < lines.size(); i++) {
    std::string line = lines[i];

    std::vector< std::string > tokens = Parser::tokenise( line );

    if( tokens.empty()) { // or tokens[0]=="#" ) {
    }
    else if ( tokens[0]=="begin" and tokens[1]=="dataset" ) { //new snapshot
      if (_data.size()==1 and isFirstDataSet) {
        logInfo( "parse()", "data file is meta file including multiple data sets");
        isFirstDataSet = false;
	    }
	    else {
        _data.push_back( convert::data::DataSet() );
      }
    }
    else if ( tokens[0]=="end" and tokens[1]=="dataset" ) { //new snapshot
      const int NumberOfSubreaders = subReaders.size();
      #pragma omp parallel for
      for (int i=0; i<NumberOfSubreaders; i++) {
        subReaders[i].parse();
        std::vector< convert::data::DataSet >  subData = subReaders[i].getData();

        if ( subData.size()>1 ) {
          logError( "parse()", "included dataset seems to hold more than one dataset, i.e. seems to be series of datasets again. This is not supported" );
        }

        if ( not subData.empty() ) {
          #pragma omp critical
          {
            _data.back().merge( subData[0] );
          }
        }
	  }

      // will be reused for next dataset if this is a series of datasets
      subReaders.clear();
    }
    else if ( tokens[0]=="include") {
      std::string directory = Parser::getDirectory(_file);
      if ( directory.empty() ) directory = ".";
      const std::string filename = directory + "/" + Parser::removeHyphens(tokens[1]);

      logInfo ( "parse()", "create a new reader (with new task) for file " << filename << " resulting from token " << tokens[1] );

      subReaders.push_back( PeanoTextPatchFileReader(filename) );
    }
    else if ( tokens[0]=="begin" and tokens[1]=="cell-values" ) { //define a cell variable
      std::string variableName = Parser::removeHyphens(tokens[2]);
      std::vector<std::string> variableDeclarationLines;
      while ( i<lines.size() and lines[i].find( "end cell-values" )==std::string::npos ) {
        variableDeclarationLines.push_back( lines[i] );
        i++;
      }
      if (i==lines.size()) {
        logError( "parse()", "file " << _file << " is corrupt as cell-values for " << variableName << " is not terminated properly. Quit parsing" );
        tarch::mpi::Rank::abort(-1);
      }
      else {
        parseVariablesDeclaration( _data.size()-1, variableDeclarationLines, variableName, convert::data::PeanoDataType::Cell_Values );
      }
    }
    else if ( tokens[0]=="begin" and tokens[1]=="vertex-values" ) { //define a vertex variable
      std::string variableName = Parser::removeHyphens(tokens[2]);
      std::vector<std::string> variableDeclarationLines;
      while ( i<lines.size()  and lines[i].find( "end vertex-values" )==std::string::npos ) {
        variableDeclarationLines.push_back( lines[i] );
        i++;
      }
      if (i==lines.size()) {
        logError( "parse()", "file " << _file << " is corrupt as vertex-values for " << variableName << " is not terminated properly. Quit parsing" );
        tarch::mpi::Rank::abort(-1);
      }
      else {
        parseVariablesDeclaration( _data.size()-1, variableDeclarationLines, variableName, convert::data::PeanoDataType::Vertex_Values );
      }
    }
    else if ( tokens[0]=="dimensions" ) {
     try {
       _dimensions = std::stoi(tokens[1]);
     }
     catch (std::out_of_range& e) {
       logError( "parse()", "error reading data for data set for _dimensions" );
       logError( "parse()", "invalid token: " << tokens[1] );
       logError( "parse()", "have to quit" );
       tarch::mpi::Rank::abort(-1);
     }
     logDebug( "parse()", "dimensions=" << _dimensions );
    }
    else if( tokens[0]=="begin" and tokens[1]=="patch" ) {
      logDebug( "parse()", "start to parse patch" );
      std::vector<std::string> patchLines;
      while ( i<lines.size()  and lines[i].find( "end patch" )==std::string::npos ) {
        patchLines.push_back( lines[i] );
        i++;
      }
      if (i==lines.size()) {
        logError( "parse()", "file " << _file << " is corrupt as begin patch is not terminated properly. Quit parsing" );
      }
      else {
        parsePatch( _data.size()-1, treeNumber, patchLines );
      }
    }
    else {
	    logDebug( "parse()", "ignore line " << line );
    }
  }} // OpenMP and for loop scope

  if (_data.size()>1) {
    logDebug( "parse()", "file " << _file << " hosts " << _data.size() << " data sets (time steps or iterations, e.g.)");
  }
  else {
    logDebug( "parse()", "file " << _file << " hosts " << _data[0].data.size() << " variable(s)");
    for (auto p: _data[0].data) {
      logDebug( "parse()", "variable " << p.first.name << " is held by " << p.second.size() << " patch(es)");
    }
  }
}


void convert::input::PeanoTextPatchFileReader::parseVariablesDeclaration( int dataSetCounter, const std::vector<std::string>& description, const std::string&  variableName, convert::data::PeanoDataType type ) {
  //get mapping
  int numberOfUnknowns = -1;
  int numberOfDofs     = -1;
	for (auto p: description) {
    std::vector<std::string> tokens = Parser::tokenise( p );
	  if (tokens[0]=="number-of-unknowns") {
	    try {
        numberOfUnknowns = std::stoi(tokens[1]);
      }
      catch (std::out_of_range& e) {
        logError( "parse()", "error reading data for data set for number-of-unknowns" );
        logError( "parse()", "invalid token: " << tokens[1] );
        logError( "parse()", "have to quit" );
        tarch::mpi::Rank::abort(-1);
      }
	  }
	  if (tokens[0]=="number-of-dofs-per-axis") {
	    try {
        numberOfDofs = std::stoi(tokens[1]);
      }
      catch (std::out_of_range& e) {
        logError( "parse()", "error reading data for data set for number-of-dofs-per-axis" );
        logError( "parse()", "invalid token: " << tokens[1] );
        logError( "parse()", "have to quit" );
        tarch::mpi::Rank::abort(-1);
      }
	  }
	}

/*
    double* mapping = nullptr;
    int mappings = -1;
    std::string line2 = lines[i+2];
    if( line2.compare( "begin mapping")==0 ) {
      logDebug( "parse()", "start to parse mapping" );
      std::vector<std::string> patchLines;
      while ( lines[i].find( "end mapping" )==std::string::npos ) {
        patchLines.push_back( lines[i] );
        i++;
      }
      mapping = parseMapping( patchLines );
    }
    _data.push
*/

    // @todo This belongs into the variable definition
/*
    else if( tokens[0]=="patch-size" ) {
      for(int j = 1; j <= dimensions; j++) {
        patchSize[j-1] = std::stoi(tokens[j]);
      }

      //update number of cells and vertices
	  cells = 1;
      vertices = 1;
      for(int i = 0; i < dimensions; i++) {
        cells *= patchSize[i];
        vertices *= patchSize[i] +1;
      }
    }
*/

  logDebug( "parse()", "added new variables " << variableName << " with " << numberOfDofs << " dof per axis (Cartesian layout) and " << numberOfUnknowns << " unknowns per dof" );

  if (numberOfUnknowns<=0) {
    logError( "parseVariablesDeclaration(int,...)", "corrupted data file " << _file << " as number-of-unknowns for " << variableName << " is smaller or equal to 0. Terminate parsing" );
  }
  else if (numberOfDofs<=0) {
    logError( "parseVariablesDeclaration(int,...)", "corrupted data file " << _file << " as number-of-dofs-per-axis for " << variableName << " is smaller or equal to 0. Terminate parsing" );
    return;
  }
  else {
    _data[dataSetCounter].data.insert(
      std::pair<convert::data::Variable, std::vector<convert::data::PatchData>>(
        convert::data::Variable( variableName, numberOfDofs, numberOfUnknowns, type, _dimensions ),
        std::vector<convert::data::PatchData>()
	    )
	  );
  }
}


/*
double* convert::input::PeanoTextPatchFileReader::parseMapping( const std::vector<std::string>& patchDescription ) {
  std::vector<double> data;
  for (auto p: patchDescription) {
    std::vector<std::string> tokens = Parser::tokenise(p);
    for (auto pp: tokens) {
      data.push_back( std::stod(pp) );
    }
  }
  double* result = new double[ data.size() ];
  for (int i=0; i<data.size(); i++) {
	result[i] = data[i];
  }
  return result;
}
*/


void convert::input::PeanoTextPatchFileReader::parsePatch( int dataSetCounter, int treeNumber, const std::vector<std::string>& text ) {
  assertion( _dimensions==2 or _dimensions==3 );

  logDebug( "parsePatch(...)", "create patch described by " << text.size() << " lines" );

  std::vector<double> offset;
  std::vector<double> size;

  for (int i=0; i<text.size(); i++) {
    std::vector<std::string> tokens = Parser::tokenise( text[i] );

    if( tokens[0]=="offset" ) {
      logDebug( "parsePatch(...)", "set offset to " << text[i] );
      for(int j = 0; j < _dimensions; j++) {
        try {
          offset.push_back( std::stod(tokens[j+1]) );
        }
        catch (std::out_of_range& e) {
          logError( "parsePatch(...)", "error reading data for data set " << dataSetCounter );
          logError( "parsePatch(...)", "invalid token: " << tokens[j+1] );
          logError( "parsePatch(...)", "have to quit" );
          tarch::mpi::Rank::abort(-1);
        }
      }
    }
    else if ( tokens[0]=="size" ) {
      logDebug( "parsePatch(...)", "set sizes to " << text[i] );
      for(int j = 0; j < _dimensions; j++) {
        try {
          size.push_back( std::stod(tokens[j+1]) );
        }
        catch (std::out_of_range& e) {
          logError( "parsePatch(...)", "error reading data for data set " << dataSetCounter );
          logError( "parsePatch(...)", "invalid token: " << tokens[j+1] );
          logError( "parsePatch(...)", "have to quit" );
          tarch::mpi::Rank::abort(-1);
        }
      }
    }
    else if ( tokens[0]=="begin" and tokens[1]=="cell-values") {
      std::string variableName = Parser::removeHyphens(tokens[2]);

      i++;
      std::vector<std::string> data;
      while ( i<text.size() and text[i].find( "end cell-values" )==std::string::npos ) {
        std::vector<std::string> tokens = Parser::tokenise( text[i] );
        data.insert( data.end(), tokens.begin(), tokens.end() );
        i++;
      }

      if (i==text.size()) {
        logError( "parsePatch()", "file " << _file << " is corrupt as begin cell-values is not terminated properly. Quit parsing" );
      }
      else {
        logDebug( "parsePatch(...)", "found " << data.size() << " entries for " << variableName );
        addDataToPatch(dataSetCounter,variableName,offset.data(),size.data(),treeNumber,data);
      }
    }
    else if ( tokens[0]=="begin" and tokens[1]=="vertex-values" ) {
      std::string variableName = Parser::removeHyphens(tokens[2]);

      i++;
      std::vector<std::string> data;
      while ( i<text.size() and text[i].find( "end vertex-values" )==std::string::npos ) {
        std::vector<std::string> tokens = Parser::tokenise( text[i] );
        data.insert( data.end(), tokens.begin(), tokens.end() );
        i++;
      }
      if (i==text.size()) {
        logError( "parsePatch()", "file " << _file << " is corrupt as begin vertex-values is not terminated properly. Quit parsing" );
      }
      else {
        addDataToPatch(dataSetCounter,variableName,offset.data(),size.data(),treeNumber,data);
      }
    }
  }
}


void convert::input::PeanoTextPatchFileReader::addDataToPatch( int dataSetCounter, const std::string& variableName, double* offset, double* size, int treeNumber, const std::vector< std::string >& textData ) {
  logDebug( "parsePatch(...)", "set data of variable " << variableName << " (parse " << textData.size() << " entries)");

  if (!_data[dataSetCounter].hasVariable(variableName)) {
    logError( "parsePatch(...)", "no variables for " << variableName << " have been declared. Ignore data set");
    return;
  }

  const convert::data::Variable  key = _data[dataSetCounter].getVariable(variableName);

  const int expectedDataEntries = key.getTotalNumberOfQuantitiesPerPatch();
  if ( textData.size()!=expectedDataEntries ) {
    logError( "parsePatch(...)", "expected " << expectedDataEntries << " data entries for variable " << variableName << " but got " << textData.size() << ". Ignore data set. Key=" << key.toString() );
    return;
  }

  convert::data::PatchData newEntry(_dimensions, offset, size, key.dofsPerAxis, key.unknowns, treeNumber);
  for (int i=0; i<expectedDataEntries; i++) {
    try {
      newEntry.data[i] = std::stod(textData[i]);
    }
    catch (std::out_of_range& e) {
      logError( "parsePatch(...)", "error reading data for " << variableName );
      logError( "parsePatch(...)", "invalid value: " << textData[i] );
      logError( "parsePatch(...)", "have to quit" );
      tarch::mpi::Rank::abort(-1);
    }
  }

  _data[dataSetCounter].data[key].push_back( newEntry );
}



std::vector< convert::data::DataSet >  convert::input::PeanoTextPatchFileReader::getData() const {
  return _data;
}
