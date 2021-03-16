#include "TopologyParser.h"
#include <iostream>
#include <fstream>
#include <sstream>


tarch::logging::Log   examples::exahype2::swe::TopologyParser::_log( "examples::exahype2::swe::TopologyParser" );

examples::exahype2::swe::TopologyParser::TopologyParser(std::string filename){

  this->filepath=filename;
  this->parsetopofile();

}

examples::exahype2::swe::TopologyParser::~TopologyParser(){
  std::cout << "Destroy topology parser."; 
}

std::vector <std::string> examples::exahype2::swe::TopologyParser::splitline(std::string line, char delimiter){

    // Vector of string to save tokens 
    std::vector <std::string> tokens; 
       
    std::stringstream strstream(line);  
    std::string intermediate; 
      
    //Split
    while(getline(strstream, intermediate, delimiter)) 
    { 
        tokens.push_back(intermediate); 
    } 
    
    return tokens;
}

void examples::exahype2::swe::TopologyParser::parsetopofile(){

  std::ifstream topofile (this->filepath);
  std::string line;
  std::stringstream ss;
  std::vector <std::string> thisline;

  
  if (topofile.is_open()){
  
    while(getline(topofile,line)){
    
      thisline = splitline(line, ' ');
      ss << thisline[0];
      float dataentry;
    
      if (thisline.size() == 2 && thisline.back() == "ncols"){
        ss >> this->ncols;
      }
      else if (thisline.size() == 2 && thisline.back() == "nrows"){
        ss >> this->nrows;
      }
      else if (thisline.size() == 2 && thisline.back() == "xll"){
        ss >> this->xlowerleft;
      }            
      else if (thisline.size() == 2 && thisline.back() == "yll"){
        ss >> this->ylowerleft;
      }   
      else if (thisline.size() == 2 && thisline.back() == "cellsize"){
        ss >> this->cellsize;
      }         
      else if (thisline.size() == 2 && thisline.back() == "nodata_value"){
        ss >> this->nodatavalue;
      } 
      else if (thisline.size() == 1){
        //get topology data
        ss >> dataentry;
        this->zvalues.push_back(dataentry);
      }           
      else{
        std::cout << "Warning: Undefined entry in topology file!"; 
      } 
    }
    
    topofile.close();
  }

  else{
    std::cout << "Error opening file " << this->filepath; 
  }

}

float examples::exahype2::swe::TopologyParser::sampletopology(float x, float y){
  
  int index = x*this->ncols + y;
	
  float z = this->zvalues.at(index);
	
  return z;

}









