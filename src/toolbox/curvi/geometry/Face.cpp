// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/curvi/geometry/Face.h"

#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/Index.h"
#include "toolbox/curvi/interpolation/TransfiniteInterpolation.h"

#include <algorithm>
#include <cmath>
#include <sstream>


/* Represents a 2D face    */
/*     0               nx  */
/*      --------------     */
/*  ny |              | ny */
/*     |              |    */
/*     |              |    */
/*     |              |    */
/*   0 |              | 0  */
/*      --------------     */
/*     0               nx  */

toolbox::curvi::Face::Face():
  coords{Coordinate::Z, Coordinate::Z},
  index_offset{0, 0, 0},
  index_size{0, 0, 0} {}


toolbox::curvi::Face::Face(
  const Coordinate                      a_coords[2],
  const int                             a_index_offset[3],
  const int                             a_index_size[3],
  const double                          a_position,
  std::function<double(double, double)> a_perturbation
):
  perturbation(a_perturbation),
  position(a_position) {
  std::copy_n(a_coords, 2, coords);
  std::copy_n(a_index_offset, 3, index_offset);
  std::copy_n(a_index_size, 3, index_size);

  nodes.resize(index_size[coords[0]] * index_size[coords[1]] * NCOORDS);

  for (int i = 0; i < 3; i++) {
    if (index_size[i] < 1) {
      std::stringstream ss;
      ss << "Error: Init face with less then one node in one dimension " << std::endl;
      ;
      ss << index_size[0] << " " << index_size[1] << " " << index_size[2] << std::endl;
      throw std::domain_error(ss.str());
    }
  }
}

toolbox::curvi::Face::Face(
  int                                   a_index_offset[3],
  FaceBoundary&                         boundary,
  std::function<double(double, double)> a_perturbation,
  bool                                  keepBoundary
):
  perturbation(a_perturbation) {

  std::copy_n(a_index_offset, 3, index_offset);
  boundary.getIndexSize(index_size);
  boundary.getCoord(coords);

  Coordinate mu = getNormal(coords[0], coords[1]);
  nodes.resize(index_size[coords[0]] * index_size[coords[1]] * NCOORDS);

  Interpolation::Transfinite::interpolate2D(boundary, nodes);
  std::vector<int> range_eta;
  getIndexRange(coords[1], range_eta);
  std::vector<int> range_xi;
  getIndexRange(coords[0], range_xi);

  if (keepBoundary) {
    range_eta.pop_back();
    range_eta.erase(range_eta.begin());
    range_xi.pop_back();
    range_xi.erase(range_xi.begin());
  }

  position = boundary.getPosition();

  for (int j : range_xi) {
    for (int i : range_eta) {
      // std::cout << j << " " << i << std::endl;
      // std::cout << (*this)(coords[0],j,i) << " " << (*this)(coords[1],j,i) << std::endl;
      (*this)(mu, j, i) = position + perturbation((*this)(coords[0], j, i), (*this)(coords[1], j, i));
    }
  }

  for (int i = 0; i < 3; i++) {
    if (index_size[i] < 1) {
      std::stringstream ss;
      ss << "Error: Init face from faceBoundary with less then one node in one dimension " << std::endl;
      ss << boundary << std::endl;
      throw std::domain_error(ss.str());
    }
  }
}

/*Copy constructor*/
toolbox::curvi::Face::Face(const Face& other) {
  std::copy_n(other.coords, 2, coords);
  std::copy_n(other.index_size, 3, index_size);
  std::copy_n(other.index_offset, 3, index_offset);
  nodes        = other.getNodes();
  perturbation = other.perturbation;
  position     = other.position;
}

/*Copy assignment*/
toolbox::curvi::Face& toolbox::curvi::Face::operator=(Face other) {
  std::copy_n(other.coords, 2, coords);
  std::copy_n(other.index_size, 3, index_size);
  std::copy_n(other.index_offset, 3, index_offset);
  nodes        = other.nodes;
  perturbation = other.perturbation;
  position     = other.position;

  return *this;
}

toolbox::curvi::Coordinate toolbox::curvi::Face::getCoord(int i) const { return coords[i]; }

void toolbox::curvi::Face::getCoord(Coordinate a_coord[2]) const { std::copy_n(coords, 2, a_coord); }


double toolbox::curvi::Face::getPosition() const { return position; }


void toolbox::curvi::Face::getIndexOffset(int a_index_offset[3]) const { std::copy_n(index_offset, 3, a_index_offset); }

int toolbox::curvi::Face::getIndexOffset(Coordinate coord) const { return index_offset[coord]; }


int toolbox::curvi::Face::getIndexSize(Coordinate i) const { return index_size[i]; }

void toolbox::curvi::Face::getIndexSize(int a_index_size[3]) const { std::copy_n(index_size, 3, a_index_size); }

void toolbox::curvi::Face::reverseDimension(Coordinate coord) {
  Coordinate       coord_other = coord == coords[0] ? coords[1] : coords[0];
  std::vector<int> range;
  getIndexRange(coord, range);
  std::vector<int> range_other;
  getIndexRange(coord_other, range);
  for (int c_indx = 0; c_indx < 3; c_indx++) {
    for (int j : range_other) {
      std::vector<double> temp;
      temp.resize(range.size());
      for (int i : range) {
        int index_temp   = range.size() - (i + 1 - range[0]);
        temp[index_temp] = (*this)(static_cast<Coordinate>(c_indx), coord_other, j, coord, i);
      }
      for (int i : range) {
        (*this)(static_cast<Coordinate>(c_indx), coord_other, j, coord, i) = temp[i - range[0]];
      }
    }
  }
}

void toolbox::curvi::Face::getIndexRange(Coordinate coord, std::vector<int>& a_range) const {
  for (int i = index_offset[coord]; i < index_size[coord] + index_offset[coord]; i++) {
    a_range.push_back(i);
  }
}

const std::vector<double>& toolbox::curvi::Face::getNodes() const { return nodes; }

double& toolbox::curvi::Face::operator()(int coord, int j, int i) {

  idx3 idx(NCOORDS, index_size[coords[0]], index_size[coords[1]]);
  int  _j = j - index_offset[coords[0]];
  int  _i = i - index_offset[coords[1]];
  return nodes[idx(coord, _j, _i)];
}

const double& toolbox::curvi::Face::operator()(int coord, int j, int i) const {

  idx3 idx(NCOORDS, index_size[coords[0]], index_size[coords[1]]);
  int  _j = j - index_offset[coords[0]];
  int  _i = i - index_offset[coords[1]];
  return nodes[idx(coord, _j, _i)];
}

const double& toolbox::curvi::Face::operator()(int coord, Coordinate coord_j, int j, Coordinate coord_i, int i) const {
  // figure out orientation
  bool orientation_a = coord_j == coords[0] && coord_i == coords[1];
  bool orientation_b = coord_i == coords[0] && coord_j == coords[1];
  // check if both coordiantes exists
  if (!(orientation_a || orientation_b)) {
    std::cout
      << "accessing non existent coordiantes " << coord_i << " " << coord_j << " for face with coordinates" << coords[0]
      << " " << coords[1] << std::endl;
    std::terminate();
  }

  if (orientation_a) {
    return (*this)(coord, j, i);
  } else {
    return (*this)(coord, i, j);
  }
}

double& toolbox::curvi::Face::operator()(int coord, Coordinate coord_j, int j, Coordinate coord_i, int i) {
  // figure out orientation
  bool orientation_a = coord_j == coords[0] && coord_i == coords[1];
  bool orientation_b = coord_i == coords[0] && coord_j == coords[1];
  // check if both coordiantes exists
  if (!(orientation_a || orientation_b)) {
    std::cout
      << "accessing non existent coordiantes " << coord_i << " " << coord_j << " for face with coordinates" << coords[0]
      << " " << coords[1] << std::endl;
    std::terminate();
  }

  if (orientation_a) {
    return (*this)(coord, j, i);
  } else {
    return (*this)(coord, i, j);
  }
}

void toolbox::curvi::Face::subface(int subface_index_offset[3], int subface_index_size[3], Face& subface) const {

  for (int c = 0; c < 3; c++) {
    if (subface_index_offset[c] < index_offset[c]
        || subface_index_offset[c] + subface_index_size[c] - 1 > index_offset[c] + index_size[c] - 1) {
      std::stringstream ss;
      ss << "Subset index range out of scope in " << coordToString(static_cast<Coordinate>(c)) << std::endl
         << "   " << "Fl " << "Sl " << "Sr " << "Fr" << std::endl;

      for (int a = 0; a < 3; a++) {
        ss << coordToString(static_cast<Coordinate>(a)) << ": " << index_offset[a] << " " << subface_index_offset[a]
           << " " << subface_index_offset[a] + subface_index_size[a] - 1 << " " << index_offset[a] + index_size[a] - 1
           << std::endl;
      }
      throw std::domain_error(ss.str());
    }
  }

  subface = Face(coords, subface_index_offset, subface_index_size, position, perturbation);
  std::vector<int> range_eta;
  subface.getIndexRange(subface.coords[1], range_eta);
  std::vector<int> range_xi;
  subface.getIndexRange(subface.coords[0], range_xi);
  for (int j : range_xi) {
    for (int i : range_eta) {
      subface(Coordinate::X, j, i) = (*this)(Coordinate::X, j, i);
      subface(Coordinate::Y, j, i) = (*this)(Coordinate::Y, j, i);
      subface(Coordinate::Z, j, i) = (*this)(Coordinate::Z, j, i);
    }
  }
}


std::function<double(double, double)> toolbox::curvi::Face::getPerturbation() const { return perturbation; }

toolbox::curvi::Edge toolbox::curvi::Face::split(Face& other, Face& left, Face& right) const {
  /* We assume this face lies in the X Y plane, the other face in the Y Z plane*/
  /* The index cut happens in the Y coordinate */

  // Find shared coordiante:
  Coordinate x, y, z;
  if (coords[0] == other.coords[0] || coords[0] == other.coords[1]) {
    y = coords[0];
    x = coords[1];
  } else if (coords[1] == other.coords[0] || coords[1] == other.coords[1]) {
    y = coords[1];
    x = coords[0];
  } else {
    std::cout << "Couldn't find shared coordinate" << std::endl;
  }

  Edge intersection_path(y);

  z = getNormal(x, y);

  /*** Cut Indeces ***/
  int index_intersection_x = other.index_offset[x];
  int index_intersection_z = index_offset[z];

  if (index_intersection_x < index_offset[x] || index_intersection_x >= index_offset[x] + index_size[x]) {
    std::stringstream ss;
    ss << "Intersection index in coord " << x << " at " << index_intersection_x << " lies outside of face "
       << index_offset[x] << " " << index_offset[x] + index_size[x] << std::endl;
    throw std::domain_error(ss.str());
  }

  if (index_intersection_z < other.index_offset[z]
      || index_intersection_z >= other.index_offset[z] + other.index_size[z]) {
    std::stringstream ss;
    ss << "Intersection index in coord " << z << " at " << index_intersection_z << " lies outside of face "
       << index_offset[z] << " " << index_offset[z] + index_size[z] << std::endl;
    throw std::domain_error(ss.str());
  }
  /******************/


  /** Cut faces **/
  // We have to find intersecting planes on each column in y
  // The results is a intersection path
  std::vector<int> range_x;
  getIndexRange(x, range_x);
  range_x.pop_back();
  std::vector<int> range_y;
  getIndexRange(y, range_y);
  std::vector<int> range_z;
  other.getIndexRange(z, range_z);
  range_z.pop_back();

  intersection_path.resize(NCOORDS * range_y.size());
  std::vector<int> intersection_index_path;
  intersection_index_path.resize(NCOORDS * range_y.size());

  idx2 idx_path(NCOORDS, range_y.size());
  for (int ind_y : range_y) {
    bool   found = false;
    double x_int, z_int;
    int    x_int_index, z_int_index;
    for (int ind_x : range_x) {
      for (int ind_z : range_z) {
        double f0 = other(x, y, ind_y, z, ind_z);
        double f1 = other(x, y, ind_y, z, ind_z + 1);
        double y0 = other(z, y, ind_y, z, ind_z);
        double y1 = other(z, y, ind_y, z, ind_z + 1);

        double t0 = (*this)(z, y, ind_y, x, ind_x);
        double t1 = (*this)(z, y, ind_y, x, ind_x + 1);
        double x0 = (*this)(x, y, ind_y, x, ind_x);
        double x1 = (*this)(x, y, ind_y, x, ind_x + 1);

        if (intersect(x0, t0, f0, y0, x1 - x0, t1 - t0, f1 - f0, y1 - y0, x_int, z_int)) {
          found = true;
        }

        if (found) {
          x_int_index = ind_x;
          z_int_index = ind_z;
          break;
        }
      }
      if (found)
        break;
    }
    if (found) {
      intersection_path(x, ind_y - range_y[0]) = x_int;
      intersection_path(y, ind_y - range_y[0]) = (*this)(y, y, ind_y, x, x_int_index);
      intersection_path(z, ind_y - range_y[0]) = z_int;

      intersection_index_path[idx_path(x, ind_y - range_y[0])] = x_int_index;
      intersection_index_path[idx_path(y, ind_y - range_y[0])] = ind_y;
      intersection_index_path[idx_path(z, ind_y - range_y[0])] = z_int_index;
    } else {
      std::stringstream ss;
      ss << "Couldn't find intersection at " << coordToString(y) << "  = " << ind_y << std::endl;
      ss << coordToString(z) << " " << coordToString(y) << " " << coordToString(x) << std::endl;
      ss << "This face: " << std::endl;
      ss << (*this) << std::endl;
      ss << "Other face: " << std::endl;
      ss << other << std::endl;
      throw std::out_of_range(ss.str());
    }
  }

  FaceBoundary thisBoundary((*this));
  FaceBoundary otherBoundary(other);

  // copy the boundary of this face
  FaceBoundary left_boundary  = thisBoundary;
  FaceBoundary right_boundary = thisBoundary;

  thisBoundary.split(x, index_intersection_x - index_offset[x], intersection_path, left_boundary, right_boundary);

  int index_offset_left[3];
  std::copy_n(index_offset, 3, index_offset_left);
  int index_offset_right[3];
  std::copy_n(index_offset, 3, index_offset_right);
  index_offset_right[x] = index_intersection_x;

  left  = Face(index_offset_left, left_boundary, perturbation);
  right = Face(index_offset_right, right_boundary, perturbation);

  return intersection_path;
}

/*belongs somewhere else*/
bool toolbox::curvi::Face::intersect(
  double  a_0,
  double  a_1,
  double  b_0,
  double  b_1,
  double  c_0,
  double  c_1,
  double  d_0,
  double  d_1,
  double& x,
  double& y
) const {
  double mu     = (a_1 - b_1 + c_1 / c_0 * (b_0 - a_0)) / (d_1 - d_0 * c_1 / c_0);
  double lambda = (b_0 - a_0 + mu * d_0) / c_0;

  if (!std::isfinite(mu) || mu < 0 || mu > 1 || lambda < 0 || lambda > 1 || !std::isfinite(lambda)) {
    return false;
  } else {

    x = a_0 + lambda * c_0;
    y = a_1 + lambda * c_1;

    if (std::isnan(x) || std::isnan(y)) {
      std::stringstream ss;
      ss << "Intersection resulted in nan";
      ss << lambda << " " << mu << std::endl;
      ss << x << " " << y << std::endl;
      ss << "for" << std::endl;
      ss << a_0 << " " << a_1 << std::endl;
      ss << b_0 << " " << b_1 << std::endl;
      ss << c_0 << " " << c_1 << std::endl;
      ss << d_0 << " " << d_1 << std::endl;
      throw std::range_error(ss.str());
    }

    return true;
  }
}

std::ostream& toolbox::curvi::operator<<(std::ostream& ss, const Face& f) {
  ss << "Face" << std::endl;
  ss << "Coordinates: " << coordToString(f.coords[0]) << " " << coordToString(f.coords[1]) << " " << std::endl;

  ss << "Position: " << f.position << std::endl;

  ss << "Index Offset: " << f.index_offset[0] << " " << f.index_offset[1] << " " << f.index_offset[2] << " "
     << std::endl;


  ss << "Index Size: " << f.index_size[0] << " " << f.index_size[1] << " " << f.index_size[2] << " " << std::endl;

  return ss;
}

void toolbox::curvi::Face::toNetCDF(std::string filename) const {
  size_t start[2] = {0, 0};
  size_t count[2] = {static_cast<size_t>(index_size[coords[0]]), static_cast<size_t>(index_size[coords[1]])};

  NetCDFWriter
    writer(filename, 1, static_cast<size_t>(index_size[coords[0]]), static_cast<size_t>(index_size[coords[1]]));
  writer.init(1, static_cast<size_t>(index_offset[coords[0]]), static_cast<size_t>(index_offset[coords[1]]));
  writer.write(start, count, nodes);
  writer.close();
}

std::string toolbox::curvi::Face::printNodes() const {
  std::stringstream ss;
  for (int d = 0; d < 3; d++) {
    ss << d << std::endl;
    std::vector<int> range_eta;
    getIndexRange(coords[1], range_eta);
    std::vector<int> range_xi;
    getIndexRange(coords[0], range_xi);
    for (auto i : range_eta) {
      for (auto j : range_xi) {
        ss << (*this)(d, i, j) << " ";
      }
      ss << std::endl;
    }
    ss << std::endl;
  }
  return ss.str();
}
