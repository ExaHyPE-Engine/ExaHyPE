// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/curvi/geometry/Edge.h"

#include "toolbox/curvi/interpolation/LagrangeInterpolation.h"

toolbox::curvi::Edge::Edge(Coordinate a_direction) {
  std::vector<double> a_data;
  data      = a_data;
  direction = a_direction;
}

toolbox::curvi::Edge::Edge(std::vector<double> a_data, Coordinate a_direction) {
  data      = a_data;
  direction = a_direction;
}


toolbox::curvi::Edge::Edge(Coordinate a_direction, double start[3], double stop[3], int n) {
  direction = a_direction;
  data.resize(n * NCOORDS);
  idx2 idx(NCOORDS, n);
  for (int d = 0; d < 3; d++) {
    auto range = Index::getRange(0, n, (stop[d] - start[d]) / double(n - 1), start[d]);
    for (int i = 0; i < n; i++) {
      data[idx(d, i)] = range[i];
    }
  }
}


toolbox::curvi::Edge::Edge(const Edge& other) {
  data      = other.data;
  direction = other.direction;
}

toolbox::curvi::Edge& toolbox::curvi::Edge::operator=(Edge other) {
  data      = other.data;
  direction = other.direction;

  return *this;
}

toolbox::curvi::Edge toolbox::curvi::Edge::subedge(int start, int stop) {
  if ((stop > numNodes()) || (start < 0)) {
    std::stringstream ss;
    ss << "Subedge out of range " << start << " " << stop << std::endl;
    throw std::out_of_range(ss.str());
  }
  std::vector<double> sub_data;
  sub_data.resize(NCOORDS * (stop - start));
  idx2 idx_n(NCOORDS, (stop - start));
  idx2 idx_d(NCOORDS, numNodes());

  for (int i = start; i < stop; i++) {
    for (int d = 0; d < NCOORDS; d++) {
      sub_data[idx_n(d, i - start)] = data[idx_d(d, i)];
    }
  }

  return Edge(sub_data, direction);
}

/*splits an edge at an intersection into two edges */
/*the intersection needs to lie on the edge*/
void toolbox::curvi::Edge::split(double intersection_node[3], Edge& left_edge, Edge& right_edge) {

  idx2 idx(NCOORDS, numNodes());

  /*Find relative position of the intersection node in the direction of the edge*/
  double node_index = -1;
  for (int i = 0; i < numNodes(); i++) {
    if (intersection_node[direction] >= data[idx(direction, i)]
        && intersection_node[direction] < data[idx(direction, i + 1)]) {
      double dc  = (data[idx(direction, i + 1)] - data[idx(direction, i)]);
      node_index = i + (intersection_node[direction] - data[idx(direction, i)]) / dc;
      break;
    }
  }

  if (node_index == -1) {
    std::stringstream ss;
    ss << "Couldn't find intersection in " << coordToString(direction) << ":" << std::endl;
    ss << intersection_node[direction] << std::endl;
    for (int d = 0; d < NCOORDS; d++) {
      std::cout << intersection_node[d] << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < numNodes(); i++) {
      std::cout << data[idx(direction, i)] << std::endl;
    }
    throw std::out_of_range(ss.str());
  }

  /*For both resulting edges we first take the subedge including the intersection */
  /* between the last (first) two nodes */
  int left_edge_last_index   = std::ceil(node_index);
  int right_edge_first_index = std::floor(node_index);

  left_edge  = subedge(0, left_edge_last_index + 1);
  right_edge = subedge(right_edge_first_index, numNodes());

  idx2 idx_l(NCOORDS, left_edge.numNodes());
  idx2 idx_r(NCOORDS, right_edge.numNodes());

  /*We then shift the end of the left edge and beginning of right edge to the intersection*/
  for (int d = 0; d < NCOORDS; d++) {
    left_edge.data[idx_l(d, left_edge_last_index)] = intersection_node[d];
    right_edge.data[idx_r(d, 0)]                   = intersection_node[d];
  }

  /*Last step is the interpolation of both edges on uniform spaced nodes*/
  std::vector<double> left_range  = Index::getRange(0, left_edge.numNodes(), 1);
  std::vector<double> right_range = Index::getRange(right_edge_first_index, right_edge.numNodes(), 1);

  left_range.back()   = node_index;
  right_range.front() = node_index;

  double              dxl            = node_index / double(left_edge.numNodes() - 1);
  std::vector<double> new_left_range = Index::getRange(0, left_edge.numNodes(), dxl);
  left_edge                          = left_edge.interpolate(left_range, new_left_range);

  double              dxr             = (numNodes() - node_index - 1) / double(right_edge.numNodes() - 1);
  std::vector<double> new_right_range = Index::getRange(0, right_edge.numNodes(), dxr, node_index);
  right_edge                          = right_edge.interpolate(right_range, new_right_range);

  /*********************************************/
};

toolbox::curvi::Edge toolbox::curvi::Edge::remesh(int numNodesNew) {
  if (numNodesNew < 1) {
    std::stringstream ss;
    ss << "Can't remesh with new nodes less then ";
    ss << numNodesNew << std::endl;

    throw std::out_of_range(ss.str());
  }

  /*Generate new ranges according to the intersection_index*/
  std::vector<double> newRange = Index::getRange(0, numNodesNew, 1.0 / (numNodesNew - 1));
  std::vector<double> oldRange = Index::getRange(0, numNodes(), 1.0 / (numNodes() - 1));
  return interpolate(oldRange, newRange);
}


toolbox::curvi::Edge toolbox::curvi::Edge::
  interpolate(const std::vector<double>& range_in, const std::vector<double>& range_out, int window) {

  idx2                idx(NCOORDS, range_out.size());
  idx2                idx_in(NCOORDS, range_in.size());
  std::vector<double> out;
  out.resize(NCOORDS * range_out.size());

  // for now only odd windows sizes
  if (window % 2 == 0)
    window++;

  int i = 0;
  for (double position : range_out) {
    int    j        = 0;
    double min_dist = std::numeric_limits<double>::max();
    for (double node : range_in) {
      double dist = std::abs(node - position);
      if (min_dist > dist) {
        min_dist = dist;
        j++;
        continue;
      }
      break;
    }


    int min_index   = int(std::max(j - std::floor(window / 2), 0.0));
    int max_index   = int(std::min(j + std::floor(window / 2), range_in.size() - 1.0));
    int window_size = max_index - min_index + 1;

    std::vector<double> range_in_window;
    range_in_window.resize(window_size);
    std::vector<double> data_window;
    data_window.resize(window_size * NCOORDS);
    idx2 idx_win(NCOORDS, window_size);

    std::copy_n(range_in.data() + min_index, window_size, range_in_window.data());
    for (int d = 0; d < NCOORDS; d++) {
      std::copy_n(data.data() + idx_in(d, min_index), window_size, data_window.data() + idx_win(d, 0));
    }

    double result[NCOORDS];
    Interpolation::Lagrange::interpolate1D(position, range_in_window, data_window, result);
    for (int d = 0; d < NCOORDS; d++) {
      out[idx(d, i)] = result[d];
    }
    i++;
  }

  return Edge(out, direction);
}

void toolbox::curvi::Edge::push_back(double d) { data.push_back(d); }
void toolbox::curvi::Edge::resize(int i) { data.resize(i); }

int toolbox::curvi::Edge::size() const { return data.size(); }

int toolbox::curvi::Edge::numNodes() const { return size() / NCOORDS; }

std::vector<double>& toolbox::curvi::Edge::getData() { return data; }

double& toolbox::curvi::Edge::operator()(int coord, int i) {
  idx2 idx(NCOORDS, numNodes());
  return data[idx(coord, i)];
}

const double& toolbox::curvi::Edge::operator()(int coord, int i) const {
  idx2 idx(NCOORDS, numNodes());
  return data[idx(coord, i)];
}


std::string toolbox::curvi::Edge::printEdge() const {
  std::stringstream ss;
  for (int d = 0; d < NCOORDS; d++) {
    Coordinate c = static_cast<Coordinate>(d);
    ss << coordToString(c) << std::endl;
    ss << "Values" << std::endl;
    for (int p = 0; p < numNodes(); p++) {
      ss << (*this)(d, p) << " ";
    }
    ss << std::endl;
    ss << "delta" << std::endl;
    for (int p = 0; p < numNodes() - 1; p++) {
      ss << (*this)(d, p) - (*this)(d, p + 1) << " ";
    }
    ss << std::endl;
  }
  ss << std::endl;
  return ss.str();
}

std::ostream& toolbox::curvi::operator<<(std::ostream& ss, const Edge& e) {
  ss << "Edge" << std::endl;
  ss << "NumNodes: " << e.numNodes() << std::endl;
  ss << "Direction: " << coordToString(e.direction) << std::endl;
  return ss;
}
