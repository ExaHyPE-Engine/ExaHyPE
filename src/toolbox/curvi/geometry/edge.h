// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/index.h"
#include "toolbox/curvi/coordinate.h"

class Edge{
 public:
  Edge(Coordinate a_direction);
  Edge(std::vector<double> a_data, Coordinate a_direction);
  Edge(Coordinate a_direction, double start[3], double stop[3], int n);
  
  Edge(const Edge& other);
  Edge& operator=(Edge other);
  
  Edge subedge(int start, int stop);
  
  void split( double intersection_node[3],
	      Edge& left_edge,
	      Edge& right_edge);
  
  Edge remesh(int numNodesNew);  
  Edge interpolate(std::vector<double> const& range_in,
		   std::vector<double> const& range_out,
                   int window=11);


  void resize(int i);
  void push_back(double d);
  
  int size() const;
  int numNodes() const;

  friend std::ostream& operator<< (std::ostream& ss, const Edge& e);
  std::string printEdge() const;
  
  double& operator()(int coord, int i);
  double const& operator()(int coord, int i) const;
  
  std::vector<double>& getData();
  
  Coordinate getDirection(){
    return direction;
  };
  
 private:
  Coordinate direction; //monotonically increasing index
  std::vector<double> data;
};
