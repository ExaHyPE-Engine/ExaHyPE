// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/curvi/geometry/Block.h"

#include "toolbox/curvi/geometry/BlockBoundary.h"
#include "toolbox/curvi/interpolation/LagrangeInterpolation.h"
#include "toolbox/curvi/interpolation/TransfiniteInterpolation.h"
#include "toolbox/curvi/netcdf/NetCDFWriter.h"
#include "toolbox/curvi/transformation/Transformation.h"

#include <easi/YAMLParser.h>
#include <string>
#include <vector>

toolbox::curvi::Block::Block():
  coords{Coordinate::X, Coordinate::X, Coordinate::X},
  index_size{0, 0, 0},
  index_offset{0, 0, 0} {};


toolbox::curvi::Block::
  Block(Coordinate a_coords[3], int a_index_offset[3], int a_index_size[3], std::vector<double> a_nodes) {
  std::copy_n(a_coords, 3, coords);
  std::copy_n(a_index_size, 3, index_size);
  std::copy_n(a_index_offset, 3, index_offset);
  nodes = a_nodes;
};

toolbox::curvi::Block::Block(BlockBoundary boundary) {
  boundary.getCoord(coords);
  boundary.getIndexOffset(index_offset);
  boundary.getIndexSize(index_size);

  nodes.resize(index_size[2] * index_size[1] * index_size[0] * NCOORDS);

  Interpolation::Transfinite::interpolate3D(boundary, (*this));
};

toolbox::curvi::Block::Block(BlockBoundary boundary, int a_index_offset[3], int a_index_size[3]) {
  boundary.getCoord(coords);
  std::copy_n(a_index_offset, 3, index_offset);
  std::copy_n(a_index_size, 3, index_size);

  nodes.resize(index_size[2] * index_size[1] * index_size[0] * NCOORDS);

  Interpolation::Transfinite::interpolate3D(
    boundary,
    index_offset[coords[0]],
    index_size[coords[0]],
    index_offset[coords[1]],
    index_size[coords[1]],
    index_offset[coords[2]],
    index_size[coords[2]],
    (*this)
  );
};

/*Copy constructor*/
toolbox::curvi::Block::Block(const Block& other) {
  std::copy_n(other.coords, 3, coords);
  std::copy_n(other.index_size, 3, index_size);
  std::copy_n(other.index_offset, 3, index_offset);
  nodes = other.nodes;
}

/*Copy assignment*/
toolbox::curvi::Block& toolbox::curvi::Block::operator=(Block other) {
  std::copy_n(other.coords, 3, coords);
  std::copy_n(other.index_size, 3, index_size);
  std::copy_n(other.index_offset, 3, index_offset);
  nodes = other.nodes;

  return *this;
}

toolbox::curvi::Coordinate toolbox::curvi::Block::getCoord(int idx) const { return coords[idx]; }

void toolbox::curvi::Block::getCoord(Coordinate a_coord[3]) const { std::copy_n(coords, 3, a_coord); }

void toolbox::curvi::Block::getIndexSize(int a_index_size[3]) const { std::copy_n(index_size, 3, a_index_size); }

void toolbox::curvi::Block::getIndexOffset(int a_index_offset[3]) const {
  std::copy_n(index_offset, 3, a_index_offset);
}

toolbox::curvi::Block toolbox::curvi::Block::
  apply(std::function<void(const Block&, int, int, int, Block&)> func, int mode_z, int mode_y, int mode_x) const {

  return resample(func, mode_z, mode_y, mode_x, mode_z, mode_y, mode_x);
}


toolbox::curvi::Block toolbox::curvi::Block::resample(
  std::function<void(const Block&, int, int, int, Block&)> func,
  int                                                      in_z,
  int                                                      in_y,
  int                                                      in_x,
  int                                                      out_z,
  int                                                      out_y,
  int                                                      out_x
) const {

  if ((index_size[0] - 1) % (in_z - 1) != 0 || (index_size[1] - 1) % (in_y - 1) != 0
      || (index_size[2] - 1) % (in_x - 1) != 0) {
    std::stringstream ss;
    ss << "Resampling with wrong modes: " << std::endl;
    ss << "Modes: " << std::endl;
    ss << in_z << " " << in_y << " " << in_x << std::endl;
    ss << "IndexSizes: " << std::endl;
    ss << index_size[0] << " " << index_size[1] << " " << index_size[2] << std::endl;
    throw std::domain_error(ss.str());
  }

  /*the number of implicit elements*/
  int elements_z = (index_size[0] - 1) / (in_z - 1);
  int elements_y = (index_size[1] - 1) / (in_y - 1);
  int elements_x = (index_size[2] - 1) / (in_x - 1);

  /*the firest index of implicit elements*/
  int start_elements_z = (index_offset[0]) / (in_z - 1);
  int start_elements_y = (index_offset[1]) / (in_y - 1);
  int start_elements_x = (index_offset[2]) / (in_x - 1);

  Coordinate coords[3];
  getCoord(coords);

  int i_indexSize[3] = {elements_z * (out_z - 1) + 1, elements_y * (out_y - 1) + 1, elements_x * (out_x - 1) + 1};

  int i_indexOffset[3] = {
    start_elements_z * (out_z - 1),
    start_elements_y * (out_y - 1),
    start_elements_x * (out_x - 1)
  };

  std::vector<double> result_nodes;

  result_nodes.resize(i_indexSize[2] * i_indexSize[1] * i_indexSize[0] * NCOORDS);

  double position[3];
  idx4   id(NCOORDS, nodes.size(), nodes.size(), nodes.size());
  Block  result(coords, i_indexOffset, i_indexSize, result_nodes);


  for (int e_z = 0; e_z < elements_z; e_z++) {
    for (int e_y = 0; e_y < elements_y; e_y++) {
      for (int e_x = 0; e_x < elements_x; e_x++) {
        // std::cout << "e_z: " << e_z<< "e_y :" << e_y<< "e_x :" << e_x << std::endl;
        func((*this), start_elements_z + e_z, start_elements_y + e_y, start_elements_x + e_x, result);
      }
    }
  }
  return result;
}


toolbox::curvi::Block toolbox::curvi::Block::
  getMetricDerivatives(double** dudx, int nodes_z, int nodes_y, int nodes_x, double dx[3]) const {


  std::function<void(const Block&, int, int, int, Block&)> compute_metric_derivatives =
    [&](const Block& in, int e_z, int e_y, int e_x, Block& out) {
      std::vector<int> range_z;
      in.getIndexRange(in.getCoord(0), e_z * (nodes_z - 1), (e_z + 1) * (nodes_z - 1) + 1, range_z);
      std::vector<int> range_y;
      in.getIndexRange(in.getCoord(1), e_y * (nodes_y - 1), (e_y + 1) * (nodes_y - 1) + 1, range_y);
      std::vector<int> range_x;
      in.getIndexRange(in.getCoord(2), e_x * (nodes_x - 1), (e_x + 1) * (nodes_x - 1) + 1, range_x);

      Transformation::metricDerivativesAndJacobian3D(dudx, in, dx, range_z, range_y, range_x, out);
    };

  return apply(compute_metric_derivatives, nodes_z, nodes_y, nodes_x);
}

void toolbox::curvi::Block::interpolate(const double node[3], double result[3], int strip) const {
  std::vector<int> ranges[3];
  if (strip % 2 == 1)
    strip++;
  for (Coordinate c : coords) {
    int range_min = std::min(
      std::max(index_offset[c], int(std::ceil(node[c]) - strip / 2)),
      index_offset[c] + index_size[c] - strip
    );
    int range_max = strip + range_min - 1;
    getIndexRange(c, range_min, range_max + 1, ranges[c]);
  }
  Interpolation::Lagrange::interpolate3D(node, (*this), ranges[0], ranges[1], ranges[2], result);
}

void toolbox::curvi::Block::getIndexRange(Coordinate coord, std::vector<int>& a_range) const {
  for (int i = index_offset[coord]; i < index_size[coord] + index_offset[coord]; i++) {
    a_range.push_back(i);
  }
}

void toolbox::curvi::Block::getIndexRange(Coordinate coord, int start, int end, std::vector<int>& a_range) const {
  if (start < index_offset[coord] || index_size[coord] + index_offset[coord] < end) {
    std::stringstream ss;
    ss << "Can't generate index: Out of scope in " << coord << std::endl;
    ss << "Start " << start << " Offset " << index_offset[coord] << std::endl;
    ss << "End " << end << " End " << index_size[coord] + index_offset[coord] << std::endl;
    throw std::domain_error(ss.str());
  }
  for (int i = start; i < end; i++) {
    a_range.push_back(i);
  }
}

toolbox::curvi::Block toolbox::curvi::operator+(Block lhs, const Block& rhs) {
  Coordinate coords_left[3], coords_right[3];
  lhs.getCoord(coords_left);
  rhs.getCoord(coords_right);

  Coordinate coords[3];
  std::copy_n(coords_left, 3, coords);

  int new_index_offset[3];
  int new_index_size[3];

  /*find fitting coordinates*/
  int r_index[3];
  r_index[0] = coords_right[0] == coords_left[0] ? 0 : coords_right[1] == coords_left[0] ? 1 : 2;

  r_index[1] = coords_right[0] == coords_left[1] ? 0 : coords_right[1] == coords_left[1] ? 1 : 2;

  r_index[2] = 3 - r_index[0] - r_index[1];
  /**************************/

  //*******Check if merge is possible **********//
  int index_offset_left[3], index_offset_right[3];
  lhs.getIndexOffset(index_offset_left);
  rhs.getIndexOffset(index_offset_right);

  //******* For the offset **********//
  int same_offset    = 0;
  int const_coord_1  = -1;
  int const_coord_2  = -1;
  int merge_at_coord = -1; // the coordiante we will merge along
  if (index_offset_left[0] == index_offset_right[r_index[0]]) {
    same_offset++;
    new_index_offset[0] = index_offset_left[0];
    const_coord_1       = 0;
  } else {
    merge_at_coord = 0;
  }

  if (index_offset_left[1] == index_offset_right[r_index[1]]) {
    same_offset++;
    new_index_offset[1] = index_offset_left[1];
    const_coord_1       = const_coord_1 == -1 ? 1 : const_coord_1;
    const_coord_2       = const_coord_2 == -1 ? 1 : const_coord_2;
  } else {
    merge_at_coord = 1;
  }

  if (index_offset_left[2] == index_offset_right[r_index[2]]) {
    same_offset++;
    new_index_offset[2] = index_offset_left[2];
    const_coord_2       = 2;
  } else {
    merge_at_coord = 2;
  }

  if (same_offset != 2) {
    std::stringstream ss;
    ss << "Trying to merge non fitting bocks (offset)" << std::endl;
    ss << "lhs" << std::endl;
    ss << lhs.toString() << std::endl;
    ss << "rhs" << std::endl;
    ss << rhs.toString() << std::endl;
    throw std::domain_error(ss.str());
  }

  //******* For the size **********//

  int index_size_left[3], index_size_right[3];
  lhs.getIndexSize(index_size_left);
  rhs.getIndexSize(index_size_right);

  int same_size = 0;

  if (merge_at_coord != 0 && index_size_left[0] == index_size_right[r_index[0]]) {
    same_size++;
    new_index_size[0] = index_size_left[0];
  }

  if (merge_at_coord != 1 && index_size_left[1] == index_size_right[r_index[1]]) {
    same_size++;
    new_index_size[1] = index_size_left[1];
  }

  if (merge_at_coord != 2 && index_size_left[2] == index_size_right[r_index[2]]) {
    same_size++;
    new_index_size[2] = index_size_left[2];
  }

  if (same_size != 2) {
    std::stringstream ss;
    ss << "Trying to merge non fitting bocks (size)" << std::endl;
    ss << "lhs" << std::endl;
    ss << lhs.toString() << std::endl;
    ss << "rhs" << std::endl;
    ss << rhs.toString() << std::endl;
    throw std::domain_error(ss.str());
  }

  new_index_size[merge_at_coord] = index_size_left[merge_at_coord] + index_size_right[r_index[merge_at_coord]] - 1;

  std::vector<double> new_nodes;
  new_nodes.resize(new_index_size[0] * new_index_size[1] * new_index_size[2] * NCOORDS);

  /*****************************/

  //******* Merge blocks **********//
  Coordinate c1 = static_cast<Coordinate>(const_coord_1);
  Coordinate c2 = static_cast<Coordinate>(const_coord_2);
  Coordinate m  = static_cast<Coordinate>(merge_at_coord);

  /*Decide which block comes first*/
  /*left comes first*/
  if (index_offset_left[m] < index_offset_right[r_index[m]]) {
    /*Interface Matches*/
    if (index_offset_left[m] + index_size_left[m] - 1 == index_offset_right[r_index[m]]) {
      new_index_offset[m] = index_offset_left[m];
    } else {
      throw std::domain_error("Interfaces don't fit for block merge");
    }
    /*right comes first*/
  } else {
    /*Interface Matches*/
    if (index_offset_left[m] == index_offset_right[r_index[m]] + index_size_right[r_index[m]] - 1) {
      new_index_offset[m] = index_offset_right[r_index[m]];
    } else {
      throw std::domain_error("Interfaces don't fit for block merge");
    }
  }

  Block            result(coords, new_index_offset, new_index_size, new_nodes);
  std::vector<int> range_c1;
  lhs.getIndexRange(c1, range_c1);
  std::vector<int> range_c2;
  lhs.getIndexRange(c2, range_c2);
  std::vector<int> range_ml;
  lhs.getIndexRange(m, range_ml);
  std::vector<int> range_mr;
  rhs.getIndexRange(m, range_mr);

  for (int i : range_c1) {
    for (int j : range_c2) {
      for (int k : range_ml) {
        for (int d = 0; d < NCOORDS; d++) {
          result(d, c1, i, c2, j, m, k) = lhs(d, c1, i, c2, j, m, k);
        }
      }
      for (int k : range_mr) {
        for (int d = 0; d < NCOORDS; d++) {
          result(d, c1, i, c2, j, m, k) = rhs(d, c1, i, c2, j, m, k);
        }
      }
    }
  }
  /******************************/
  return result;
}

double& toolbox::curvi::Block::operator()(int coord, int k, int j, int i) {
  idx4 idx(NCOORDS, index_size[coords[0]], index_size[coords[1]], index_size[coords[2]]);
  int  _k = k - index_offset[coords[0]];
  int  _j = j - index_offset[coords[1]];
  int  _i = i - index_offset[coords[2]];
  return nodes[idx(coord, _k, _j, _i)];
}

const double& toolbox::curvi::Block::operator()(int coord, int k, int j, int i) const {
  idx4 idx(NCOORDS, index_size[coords[0]], index_size[coords[1]], index_size[coords[2]]);
  int  _k = k - index_offset[coords[0]];
  int  _j = j - index_offset[coords[1]];
  int  _i = i - index_offset[coords[2]];
  return nodes[idx(coord, _k, _j, _i)];
}

double& toolbox::curvi::Block::
  operator()(int d, Coordinate coord_k, int k, Coordinate coord_j, int j, Coordinate coord_i, int i) {
  bool orientation_kji = coord_k == coords[0] && coord_j == coords[1] && coord_i == coords[2];
  bool orientation_kij = coord_k == coords[0] && coord_i == coords[1] && coord_j == coords[2];
  bool orientation_ikj = coord_i == coords[0] && coord_k == coords[1] && coord_j == coords[2];
  bool orientation_jki = coord_j == coords[0] && coord_k == coords[1] && coord_i == coords[2];
  bool orientation_ijk = coord_i == coords[0] && coord_j == coords[1] && coord_k == coords[2];
  bool orientation_jik = coord_j == coords[0] && coord_i == coords[1] && coord_k == coords[2];

  if (orientation_kji)
    return (*this)(d, k, j, i);
  if (orientation_kij)
    return (*this)(d, k, i, j);
  if (orientation_ikj)
    return (*this)(d, i, k, j);
  if (orientation_jki)
    return (*this)(d, j, k, i);
  if (orientation_ijk)
    return (*this)(d, i, j, k);
  if (orientation_jik)
    return (*this)(d, j, i, k);
}

const double& toolbox::curvi::Block::
  operator()(int d, Coordinate coord_k, int k, Coordinate coord_j, int j, Coordinate coord_i, int i) const {
  bool orientation_kji = coord_k == coords[0] && coord_j == coords[1] && coord_i == coords[2];
  bool orientation_kij = coord_k == coords[0] && coord_i == coords[1] && coord_j == coords[2];
  bool orientation_ikj = coord_i == coords[0] && coord_k == coords[1] && coord_j == coords[2];
  bool orientation_jki = coord_j == coords[0] && coord_k == coords[1] && coord_i == coords[2];
  bool orientation_ijk = coord_i == coords[0] && coord_j == coords[1] && coord_k == coords[2];
  bool orientation_jik = coord_j == coords[0] && coord_i == coords[1] && coord_k == coords[2];

  if (orientation_kji)
    return (*this)(d, k, j, i);
  if (orientation_kij)
    return (*this)(d, k, i, j);
  if (orientation_ikj)
    return (*this)(d, i, k, j);
  if (orientation_jki)
    return (*this)(d, j, k, i);
  if (orientation_ijk)
    return (*this)(d, i, j, k);
  if (orientation_jik)
    return (*this)(d, j, i, k);
}

std::string toolbox::curvi::Block::toString() const {
  std::stringstream ss;
  ss << "Coodinates: " << coordToString(coords[0]) << " " << coordToString(coords[1]) << " " << coordToString(coords[2])
     << " " << std::endl;

  ss << "Index Offset: " << index_offset[0] << " " << index_offset[1] << " " << index_offset[2] << " " << std::endl;


  ss << "Index Size: " << index_size[0] << " " << index_size[1] << " " << index_size[2] << " " << std::endl;

  return ss.str();
}

void toolbox::curvi::Block::appendToNetCDF(std::string filename, std::string prefix) {
  size_t start[3] = {0, 0, 0};
  size_t count[3] = {
    static_cast<size_t>(index_size[coords[0]]),
    static_cast<size_t>(index_size[coords[1]]),
    static_cast<size_t>(index_size[coords[2]])
  };

  NetCDFWriter writer(
    filename,
    static_cast<size_t>(index_size[coords[0]]),
    static_cast<size_t>(index_size[coords[1]]),
    static_cast<size_t>(index_size[coords[2]])
  );

  writer.open(
    static_cast<size_t>(index_offset[coords[0]]),
    static_cast<size_t>(index_offset[coords[1]]),
    static_cast<size_t>(index_offset[coords[2]]),
    true,
    prefix
  );
  writer.write(start, count, nodes);
  writer.close();
}


void toolbox::curvi::Block::toNetCDF(std::string filename) const {
  size_t start[3] = {0, 0, 0};
  size_t count[3] = {
    static_cast<size_t>(index_size[coords[0]]),
    static_cast<size_t>(index_size[coords[1]]),
    static_cast<size_t>(index_size[coords[2]])
  };

  NetCDFWriter writer(
    filename,
    static_cast<size_t>(index_size[coords[0]]),
    static_cast<size_t>(index_size[coords[1]]),
    static_cast<size_t>(index_size[coords[2]])
  );

  writer.init(
    static_cast<size_t>(index_offset[coords[0]]),
    static_cast<size_t>(index_offset[coords[1]]),
    static_cast<size_t>(index_offset[coords[2]])
  );
  writer.write(start, count, nodes);
  writer.close();
};
