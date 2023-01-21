#pragma once


namespace convert {
  namespace input {
    class Parser;
  }
}


#include <vector>
#include <string>


class convert::input::Parser {
  public:
	static std::vector<std::string> tokenise( const std::string& line );
	static std::string getDirectory(const std::string &fileName);
	static std::string removeHyphens( const std::string& value );
};

