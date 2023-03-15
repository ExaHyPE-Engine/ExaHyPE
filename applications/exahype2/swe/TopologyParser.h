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
    
    std::string dispfilepath;
    std::vector <double> displacement;
    int dispmx = -1;
    int dispmy = -1;
    int dispmt = -1;
    double dispxlowerleft = 0.0;
    double dispylowerleft = 0.0;
    double dispt0 = 0.0;
    double dispdx = 0.0;
    double dispdy = 0.0;
    double dispdt = 0.0;

  public:
  
    TopologyParser(std::string filename, std::string dispfilename);
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
    
    void parsedisplacementfile();
    double sampledisplacement(double x, double y, double t);
    
    std::string getdispfilepath() {return this->dispfilepath;}
    int getdispmx() {return this->dispmx;}
    int getdispmy() {return this->dispmy;}
    int getdispmt() {return this->dispmt;}
    double getdispxlowerleft() {return dispxlowerleft;}
    double getdispylowerleft() {return dispylowerleft;}
    double getdispt0() {return this->dispt0;}
    double getdispdx() {return this->dispdx;}
    double getdispdy() {return this->dispdy;}
    double getdispdt() {return this->dispdt;}
    std::vector <double> getdisplacement() {return this->displacement;}
    
            
};


#endif
