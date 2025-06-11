// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/index.h"

#include <algorithm>
#include <vector>

#include "edge.h"
#include "toolbox/curvi/coordinate.h"

/*     Represents the boundaries of a 2D face    */
/*   ^                                           */
/*   |                                           */
/*   |  offset(xi)+     top_edge                 */
/*   |  size(xi)    --------------               */
/*   |             |              |              */
/*   |             |              |              */
/*xi |   left_edge |              | right_edge   */
/*   |             |              |              */
/*   |  offset(xi) |              |              */
/*   |              --------------               */
/*   |   offset(eta)  bottom_edge  offset(eta)+  */
/*   |                             size(eta)     */
/*   0------------------- eta------------------->*/

class Face;

class FaceBoundary{
  
 public:
  //enum Edge{ LEFT, RIGHT, BOTTOM, TOP };
  
  //  FaceBoundary();
  ~FaceBoundary(){};

  FaceBoundary(Face const& face);

  FaceBoundary(const FaceBoundary& other);
  FaceBoundary& operator=(FaceBoundary other);
  
  FaceBoundary(double a_offset[3], double a_size[3],
	       int a_range[3], Coordinate a_coords[2]);

  FaceBoundary(Coordinate a_coords[2],
	       double a_position,
	       Edge& a_left,
	       Edge& a_right,
	       Edge& a_bottom,
	       Edge& a_top);
  
 
  Coordinate getCoord     (int i) const;
  void       getCoord     (Coordinate a_coord[2]) const;
  
  int  getIndexSize (int i) const;
  void getIndexSize (int a_index_size[3]) const;
  
  double getPosition() const;
  
  Edge& getEdge(Coordinate coord, bool upper);  

  void getIndexRange(Coordinate coord, std::vector<int>& a_range) const;

  void setEdge(Coordinate coord, bool upper, Edge const& other);

  void split(Coordinate c, int intersection_index,
	     Edge intersection_path,
	     FaceBoundary& left, FaceBoundary& right);

  void checkConformity();
  
  friend std::ostream& operator<< (std::ostream &ss, const FaceBoundary& f);
  std::string printNodes();

  
private:
  Edge left_edge;
  Edge right_edge;
  Edge bottom_edge;
  Edge top_edge;

  Coordinate coords[2];
  int        index_size[3];
  double     position;
};
