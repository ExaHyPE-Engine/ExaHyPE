// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/curvi/geometry/FaceBoundary.h"

#include "toolbox/curvi/Index.h"
#include "toolbox/curvi/geometry/Edge.h"
#include "toolbox/curvi/geometry/Face.h"
#include "toolbox/curvi/interpolation/LagrangeInterpolation.h"

#include <algorithm>
#include <cmath>
#include <vector>


// FaceBoundary::FaceBoundary(){};

/*Copy constructor*/
toolbox::curvi::FaceBoundary::FaceBoundary(const FaceBoundary& other):
  left_edge(other.left_edge),
  right_edge(other.right_edge),
  top_edge(other.top_edge),
  bottom_edge(other.bottom_edge) {
  std::copy_n(other.coords, 2, coords);
  std::copy_n(other.index_size, 3, index_size);
  position = other.position;
  checkConformity();
}

/*Copy assignment*/
toolbox::curvi::FaceBoundary& toolbox::curvi::FaceBoundary::operator=(FaceBoundary other) {
  std::copy_n(other.coords, 2, coords);
  std::copy_n(other.index_size, 3, index_size);
  left_edge   = other.left_edge;
  right_edge  = other.right_edge;
  top_edge    = other.top_edge;
  bottom_edge = other.bottom_edge;
  position    = other.position;
  checkConformity();

  return *this;
}

toolbox::curvi::FaceBoundary::
  FaceBoundary(Coordinate a_coords[2], double a_position, Edge& a_left, Edge& a_right, Edge& a_bottom, Edge& a_top):
  position(a_position),
  left_edge(a_left),
  right_edge(a_right),
  bottom_edge(a_bottom),
  top_edge(a_top) {
  std::copy_n(a_coords, 2, coords);

  // if(a_left.size() != a_right.size()){
  //   std::domain_error("Initialising Face Boundary with no fittig edges");
  // }

  // if(a_bottom.size() != a_top.size()){
  //   std::domain_error("Initialising Face Boundary with no fittig edges");
  // }

  Coordinate mu = getNormal(coords[0], coords[1]);

  index_size[coords[0]] = left_edge.numNodes();
  index_size[coords[1]] = top_edge.numNodes();
  index_size[mu]        = 1;
  checkConformity();
};

/*Initializes the boundary to a given face*/
toolbox::curvi::FaceBoundary::FaceBoundary(const Face& face):
  left_edge(Edge(face.getCoord(0))),
  right_edge(Edge(face.getCoord(0))),
  bottom_edge(Edge(face.getCoord(1))),
  top_edge(Edge(face.getCoord(1))) {
  /*Copy position, indeces and coordinates from face*/
  face.getIndexSize(index_size);
  face.getCoord(coords);
  position = face.getPosition();

  /*Copy values from the boundary of the face*/
  std::vector<int> range_eta;
  face.getIndexRange(coords[1], range_eta);
  std::vector<int> range_xi;
  face.getIndexRange(coords[0], range_xi);

  for (int c_int = 0; c_int < NCOORDS; c_int++) {
    Coordinate c = static_cast<Coordinate>(c_int);

    for (int i : range_eta) {
      bottom_edge.push_back(face(c, range_xi.front(), i));
      top_edge.push_back(face(c, range_xi.back(), i));
    }
    for (int i : range_xi) {
      left_edge.push_back(face(c, i, range_eta.front()));
      right_edge.push_back(face(c, i, range_eta.back()));
    }
  }

  int minSize[3]     = {1, 1, 1};
  minSize[coords[0]] = 2;
  minSize[coords[1]] = 2;
  for (int i = 0; i < 3; i++) {
    if (index_size[i] < minSize[i]) {
      std::stringstream ss;
      ss << "Error: Init faceBoundary with not enough nodes one dimension " << std::endl;
      ss << coordToString(coords[0]) << " " << coordToString(coords[0]) << std::endl;
      ss << index_size[0] << " " << index_size[1] << " " << index_size[2] << std::endl;
      throw std::invalid_argument(ss.str());
    }
  }

  checkConformity();
}

/*Initializes the boundary to a given set of geometric parameters*/
toolbox::curvi::FaceBoundary::
  FaceBoundary(double offset[3], double size[3], int a_index_size[3], Coordinate a_coords[2]):
  left_edge(Edge(a_coords[0])),
  right_edge(Edge(a_coords[0])),
  bottom_edge(Edge(a_coords[1])),
  top_edge(Edge(a_coords[1])) {

  /*Set position, indeces*/
  std::copy_n(a_coords, 2, coords);
  std::copy_n(a_index_size, 3, index_size);
  /*Test input arguments*/
  int minSize[3]     = {1, 1, 1};
  minSize[coords[0]] = 2;
  minSize[coords[1]] = 2;
  for (int i = 0; i < 3; i++) {
    bool              error = false;
    std::stringstream ss;
    if (index_size[i] < minSize[i]) {
      error = true;
      ss << "Error: Init faceBoundary with not enough nodes in one dimension " << std::endl;
    }

    if (!std::isfinite(offset[i]) || !std::isfinite(size[i])) {
      error = true;
      ss << "Error: Init faceBoundary with not finite domain definition" << std::endl;
      ss << "Size: " << size[0] << " " << size[1] << " " << size[2] << std::endl;
      ss << "Offset: " << offset[0] << " " << offset[1] << " " << offset[2] << std::endl;
    }

    if (error) {
      ss << (*this) << std::endl;
      throw std::invalid_argument(ss.str());
    }
  }
  /*******************/
  Coordinate eta = coords[1];
  Coordinate xi  = coords[0];
  Coordinate mu  = getNormal(eta, xi);

  position = offset[mu];
  /***********************/

  int index_size_eta = index_size[coords[1]];
  int index_size_xi  = index_size[coords[0]];

  left_edge.resize(index_size_xi * NCOORDS);
  right_edge.resize(index_size_xi * NCOORDS);
  bottom_edge.resize(index_size_eta * NCOORDS);
  top_edge.resize(index_size_eta * NCOORDS);


  /*init top and bottom line*/
  {
    idx2   idx(NCOORDS, index_size_eta);
    double deta = 1.0 / double(index_size_eta - 1) * size[eta];
    for (int i = 0; i < index_size_eta; i++) {
      double coord_eta = i * deta + offset[eta];

      bottom_edge(eta, i) = coord_eta;
      bottom_edge(xi, i)  = offset[xi];
      bottom_edge(mu, i)  = position;

      top_edge(eta, i) = coord_eta;
      top_edge(xi, i)  = offset[xi] + size[xi];
      top_edge(mu, i)  = position;
    }
  }

  /*init left and right edge*/
  {
    idx2   idx(NCOORDS, index_size_xi);
    double dxi = 1.0 / double(index_size_xi - 1) * size[xi];
    for (int i = 0; i < index_size_xi; i++) {
      double coord_xi = i * dxi + offset[xi];

      left_edge(eta, i) = offset[eta];
      left_edge(xi, i)  = coord_xi;
      left_edge(mu, i)  = position;

      right_edge(eta, i) = offset[eta] + size[eta];
      right_edge(xi, i)  = coord_xi;
      right_edge(mu, i)  = position;
    }
  }
}

toolbox::curvi::Coordinate toolbox::curvi::FaceBoundary::getCoord(int i) const { return coords[i]; }

void toolbox::curvi::FaceBoundary::getCoord(Coordinate a_coord[2]) const { std::copy_n(coords, 2, a_coord); }


int toolbox::curvi::FaceBoundary::getIndexSize(int i) const { return index_size[i]; }

void toolbox::curvi::FaceBoundary::getIndexSize(int a_index_size[3]) const { std::copy_n(index_size, 3, a_index_size); }

double toolbox::curvi::FaceBoundary::getPosition() const { return position; }

toolbox::curvi::Edge& toolbox::curvi::FaceBoundary::getEdge(Coordinate coord, bool upper) {
  if (coord != coords[1] && coord != coords[0]) {
    throw std::domain_error("Trying to access edge in domain out of faces scope");
  }
  if (coord == coords[1] && upper) {
    return right_edge;
  }

  if (coord == coords[1] && !upper) {
    return left_edge;
  }

  if (coord == coords[0] && upper) {
    return top_edge;
  }

  if (coord == coords[0] && !upper) {
    return bottom_edge;
  }
}

void toolbox::curvi::FaceBoundary::setEdge(Coordinate coord, bool upper, const Edge& other) {
  if (coord == coords[1] && upper) {
    right_edge = other;
  }

  if (coord == coords[1] && !upper) {
    left_edge = other;
  }

  if (coord == coords[0] && upper) {
    top_edge = other;
  }

  if (coord == coords[0] && !upper) {
    bottom_edge = other;
  }
}

std::ostream& toolbox::curvi::operator<<(std::ostream& ss, const FaceBoundary& f) {
  ss << "Face Boundary" << std::endl;
  ss << "Coodinates: " << coordToString(f.coords[0]) << " " << coordToString(f.coords[1]) << " " << std::endl;

  ss << "Position: " << f.position << std::endl;
  ss << "Index Size " << f.index_size[0] << " " << f.index_size[1] << " " << f.index_size[2] << std::endl;
  return ss;
}


/*Splitts a Face Boundary along an edge defined by intersection_path*/
void toolbox::curvi::FaceBoundary::
  split(Coordinate c, int intersection_index, Edge cutting_edge, FaceBoundary& left, FaceBoundary& right) {

  // Check if coordinate c is part of Face Boundary
  if (c != coords[0] && c != coords[1]) {
    std::stringstream ss;
    ss << "Splitting Face Boundary in wrong dimension" << std::endl;
    ss << (*this) << std::endl;
    ss << coordToString(c) << std::endl;
    throw std::invalid_argument(ss.str());
  }

  // We assume we split from left to right, along eta:
  Coordinate eta = c;
  Coordinate xi  = c == coords[0] ? coords[1] : coords[0];

  // To keep the Face Boundary valid
  // the intersection path needs to have the same size as its paralell edges
  Edge o_edge = getEdge(eta, false);

  if (o_edge.size() != cutting_edge.size()) {
    std::stringstream ss;
    ss << "Trying to split Face Boundary with intersection path of wrong length" << std::endl;
    ss << (*this) << std::endl;
    ss << "Old edge" << std::endl;
    ss << o_edge << std::endl;
    ss << "Cutting edge" << std::endl;
    ss << cutting_edge << std::endl;
    throw std::invalid_argument(ss.str());
  }

  // We assume the first node of the intersection lies on the bottom edge of the face boundary
  // same with the last node and the top edge
  // Both edges will be split at these nodes

  Edge   top_edge(getEdge(xi, true));
  Edge   bottom_edge(getEdge(xi, false));
  double intersection_node_top[3];
  double intersection_node_bottom[3];

  for (int d = 0; d < NCOORDS; d++) {
    intersection_node_top[d]    = cutting_edge(d, cutting_edge.numNodes() - 1);
    intersection_node_bottom[d] = cutting_edge(d, 0);
  }

  Edge left_new_bottom(bottom_edge.getDirection());
  Edge left_new_top(top_edge.getDirection());
  Edge right_new_bottom(bottom_edge.getDirection());
  Edge right_new_top(top_edge.getDirection());

  // split top and bottome edge

  top_edge.split(intersection_node_top, left_new_top, right_new_top);
  bottom_edge.split(intersection_node_bottom, left_new_bottom, right_new_bottom);

  left_new_top    = left_new_top.remesh(intersection_index + 1);
  left_new_bottom = left_new_bottom.remesh(intersection_index + 1);

  right_new_top    = right_new_top.remesh(top_edge.numNodes() - intersection_index);
  right_new_bottom = right_new_bottom.remesh(top_edge.numNodes() - intersection_index);


  // set resulting face boundaries
  if (eta == coords[1]) {
    left = FaceBoundary(coords, position, getEdge(eta, false), cutting_edge, left_new_bottom, left_new_top);

    right = FaceBoundary(coords, position, cutting_edge, getEdge(eta, true), right_new_bottom, right_new_top);
  } else {
    left = FaceBoundary(coords, position, left_new_bottom, left_new_top, getEdge(eta, false), cutting_edge);

    right = FaceBoundary(coords, position, right_new_bottom, right_new_top, cutting_edge, getEdge(eta, true));
  }
};


void toolbox::curvi::FaceBoundary::checkConformity() {
  bool conform = true;
  conform &= left_edge.getDirection() == right_edge.getDirection();
  conform &= top_edge.getDirection() == bottom_edge.getDirection();

  conform &= left_edge.numNodes() == right_edge.numNodes();
  conform &= top_edge.numNodes() == bottom_edge.numNodes();

  conform &= left_edge.getDirection() != top_edge.getDirection();

  Coordinate eta = top_edge.getDirection() == coords[0] ? coords[0] : coords[1];
  Coordinate xi  = left_edge.getDirection() == coords[0] ? coords[0] : coords[1];

  conform &= index_size[eta] == top_edge.numNodes();
  conform &= index_size[xi] == left_edge.numNodes();

  if (!conform) {
    std::stringstream ss;
    ss << "faceBoundary is not conform" << std::endl;
    ss << (*this) << std::endl;
    ss << "left edge" << std::endl;
    ss << left_edge << std::endl;
    ss << "right edge" << std::endl;
    ss << right_edge << std::endl;
    ss << "top edge" << std::endl;
    ss << top_edge << std::endl;
    ss << "bottom edge" << std::endl;
    ss << bottom_edge << std::endl;
    throw std::range_error(ss.str());
  }
}

std::string toolbox::curvi::FaceBoundary::printNodes() {
  std::stringstream ss;

  ss << "left edge" << std::endl;
  ss << left_edge.printEdge() << std::endl;
  ss << "right edge" << std::endl;
  ss << right_edge.printEdge() << std::endl;
  ss << "top edge" << std::endl;
  ss << top_edge.printEdge() << std::endl;
  ss << "bottom edge" << std::endl;
  ss << bottom_edge.printEdge() << std::endl;
  return ss.str();
}
