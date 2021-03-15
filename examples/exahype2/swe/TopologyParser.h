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
    float xlowerleft = 0.0;
    float ylowerleft = 0.0;
    float cellsize = 0.0;
    float nodatavalue = 0.0;   
    std::vector <float> zvalues;

  public:
  
    void parsetopofile(std::string filename);
    float sampletopology(float x, float y);
    
    std::string getfilepath() {return this->filepath;}
    int getncols() {return this->ncols;} 
    int getnrows() {return this->nrows;}
    float getxlowerleft() {return this->xlowerleft;}
    float getylowerleft() {return this->ylowerleft;}
    float getcellsize() {return this->cellsize;}
    float getnodatavalue() {return this->nodatavalue;}   
    std::vector <float> getzvalues() {return this->zvalues;}
    
            
};


#endif
