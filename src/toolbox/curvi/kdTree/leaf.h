// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/kdTree/node.h"

class Leaf : public Node{
 public:
 Leaf():boundary(BlockBoundary(Coordinate::X,Coordinate::X,Coordinate::X)){
   isInit = false;
 };


  void init(BlockBoundary& a_boundary, uint modes[3]){
    boundary = a_boundary;
    isInit = true;
  }
 
  std::string toString(int depth = 0){
    checkInit();
    
    std::stringstream ss;
    for(int i = 0 ; i< depth ; i++){
      ss << "\t";
    }
    ss << "Leaf: " << std::endl;
    ss << boundary.toString(depth+1) << std::endl;
    return ss.str();
  }  

  Block traverse(){
    checkInit();
    
    Block block(boundary);
    return block;
  }

  BlockBoundary traverse(int index[3]){
    checkInit();
    return boundary;
  }
  
  BlockBoundary traverseProjection(double coordinate[3]){
    checkInit();
    return boundary;
  }

  void checkInit(){
    if(!isInit){
      throw std::logic_error("Leaf not initialized yet");
    }
  }
  
  bool isLeaf() const{
    return true;
  }

 private:
  bool isInit;
  BlockBoundary boundary;  
};
