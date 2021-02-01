#include "Parser.h"


#include <sstream>


std::vector<std::string> convert::input::Parser::tokenise( const std::string& line ) {
  std::vector<std::string> result;

  std::istringstream ss(line);
  std::string token;
  while (ss >> token) {
    result.push_back( token );
  }

  return result;
}


std::string convert::input::Parser::getDirectory(const std::string &fileName) {
  return fileName.substr(0, fileName.find_last_of("/\\") +1);
}


std::string convert::input::Parser::removeHyphens( const std::string& value ) {
  std::string result = value;
  if (result.at(result.size()-1)=='"') {
    result = result.substr(0,result.size()-1);
  }
  if (result.at(0)=='"') {
    result = result.substr(1);
  }
  return result;
}
