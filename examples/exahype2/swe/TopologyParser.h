//GeoClaw topology file paser
//
#ifndef _examples_exahype2_swe_TopologyParser_H_
#define _examples_exahype2_swe_TopologyParser_H_


#include "tarch/logging/Log.h"
#include <string>
#include <vector>

namespace examples {namespace exahype2 {namespace swe {
  class TopologyParser;

}}}



class examples::exahype2::swe::TopologyParser {
  private:
    static tarch::logging::Log _log;
    
    std::vector <std::string> splitline(std::string line, char delimiter);
    
    std::string filepath;
    int ncols = -1; 
    int nrows = -1;
    double xlowerleft = 0.0;
    double ylowerleft = 0.0;
    double cellsize = 0.0;
    double nodatavalue = 0.0;   
    std::vector <double> zvalues;

  public:
  
    TopologyParser(std::string filename);
    ~TopologyParser();
  
    void parsetopofile();
    double sampletopology(double x, double y);
    
    std::string getfilepath() {return this->filepath;}
    int getncols() {return this->ncols;} 
    int getnrows() {return this->nrows;}
    double getxlowerleft() {return this->xlowerleft;}
    double getylowerleft() {return this->ylowerleft;}
    double getcellsize() {return this->cellsize;}
    double getnodatavalue() {return this->nodatavalue;}   
    std::vector <double> getzvalues() {return this->zvalues;}
    
            
};


#endif
