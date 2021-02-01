#include "tarch/logging/LogFilterFileReader.h"
#include "tarch/logging/LogFilter.h"


#include <fstream>
#include <stdlib.h>
#include <string>


#include "tarch/logging/CommandLineLogger.h"


tarch::logging::Log  tarch::logging::LogFilterFileReader::_log( "tarch::logging::LogFilterFileReader" );


bool tarch::logging::LogFilterFileReader::interpretTokens( const std::string& levelToken, const std::string& classNameToken, const std::string& rankToken, const std::string& onOffToken ) {
  bool result = true;

  if (
    levelToken.compare( tarch::logging::LogFilter::FilterListEntry::TargetInfo )!=0
    and
    levelToken.compare( tarch::logging::LogFilter::FilterListEntry::TargetDebug )!=0
    and
    levelToken.compare( tarch::logging::LogFilter::FilterListEntry::TargetTrace )!=0
    and
    levelToken.compare( tarch::logging::LogFilter::FilterListEntry::TargetAll   )!=0
  ) {
    logError( "interpretTokens(...)", "expected \"debug\", \"info\", \"all\" or \"trace\" but got " << levelToken );
    result = false;
  }

  int rank = rankToken=="-1" ? -1 : atoi( rankToken.c_str() );
  if ( rank<-1 ) {
    logError( "interpretTokens(...)", "expected positive rank or -1 for all ranks but got " << rankToken );
    result = false;
  }

  bool isFilter = onOffToken=="black";
  if (onOffToken.compare("black")!=0 && onOffToken.compare("white")!=0) {
    logError( "interpretTokens(...)", "expected \"black\" or \"white\" but got \"" << onOffToken << "\"");
    result = false;
  }

  if (result) {
    LogFilter::getInstance().addFilterListEntry(
      ::tarch::logging::LogFilter::FilterListEntry(
        levelToken, rank, classNameToken, isFilter )
    );
  }

  return result;
}

bool tarch::logging::LogFilterFileReader::parseLine(std::ifstream& file, const std::string& filename, const std::string& line, int linenumber) {
  std::string lineWithNewline = line + '\n';

  //Loop through line
  int characterPosition = 0;
  const int NumberOfTokensPerLine = 4;
  std::string tokens[NumberOfTokensPerLine];
  int currentToken = 0;
  while (currentToken<NumberOfTokensPerLine && !file.eof() && characterPosition < static_cast<int>(lineWithNewline.length())) {
    char buffer = lineWithNewline[characterPosition];

    const bool bufferIsWhiteSpace =
      buffer == ' '  ||
      buffer == '\t' ||
      buffer == '\r' ||
      buffer == '\n';
    if (bufferIsWhiteSpace && !tokens[currentToken].empty()) {
      currentToken++;
    }
    if (!bufferIsWhiteSpace) {
      tokens[currentToken] += buffer;
    }

    characterPosition++;
  }

  if (currentToken != NumberOfTokensPerLine) {
    logError( "parsePlainTextFile(string)", "syntax error in input file " << filename << ", line " << linenumber << ":" << line );
    return false;
  }
  else {
    return interpretTokens(tokens[0],tokens[1],tokens[2],tokens[3]);
  }
}


std::string tarch::logging::LogFilterFileReader::trimLine( const std::string& line ) {
  std::size_t first = line.find_first_not_of(' ');
  if (first == std::string::npos)
      return "";
  std::size_t last = line.find_last_not_of(' ');
  return line.substr(first, (last-first+1));
}


bool tarch::logging::LogFilterFileReader::parsePlainTextFile( const std::string& filename ) {
  bool result = true;

  LogFilter::getInstance().clearFilterList();

  tarch::logging::LogFilter::getInstance().addFilterListEntry( tarch::logging::LogFilter::FilterListEntry(
    tarch::logging::LogFilter::FilterListEntry::TargetAll,
    tarch::logging::LogFilter::FilterListEntry::AnyRank,
    "",
    tarch::logging::LogFilter::FilterListEntry::WhiteListEntry
  ));

  std::ifstream file;
  file.open(filename.c_str()); // open a file
  if (!file.good()) {
    logError( "parsePlainTextFile(string)", "was not able to open input file " << filename );
    result = false;
  }

  int linenumber = 1;
  std::string line;
  while (!file.eof() && result) {
    std::getline(file, line);
    line = trimLine(line);
    if(line.length() > 0 && line[0]!='#') {
      result = parseLine(file, filename, line, linenumber);
    }

    linenumber++;
  }

  if (!result) {
    logWarning( "parsePlainTextFile(string)", "filter file " << filename << " was invalid. Switch on all log statements" );
    LogFilter::getInstance().addFilterListEntry( ::tarch::logging::LogFilter::FilterListEntry( "info", false ) );
    LogFilter::getInstance().addFilterListEntry( ::tarch::logging::LogFilter::FilterListEntry( "debug", false ) );
  }

  return result;
}

