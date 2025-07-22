// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/curvi/geometry/BlockBoundary.h"

toolbox::curvi::BlockBoundary::BlockBoundary(Coordinate a_coords[3]) { std::copy_n(a_coords, 3, coords); }

toolbox::curvi::BlockBoundary::BlockBoundary(Coordinate c0, Coordinate c1, Coordinate c2) {
  coords[0] = c0;
  coords[1] = c1;
  coords[2] = c2;
}

toolbox::curvi::BlockBoundary::BlockBoundary(Face topography, bool top, int n_depth, double depth) {
  /*We asume topography is in the eta xi plane at the bottom of the domain*/

  Coordinate coords_topo[2];
  topography.getCoord(coords_topo);
  Coordinate eta = topography.getCoord(1);
  Coordinate xi  = topography.getCoord(0);
  Coordinate mu  = getNormal(eta, xi);

  // set coordiantes for block
  coords[2] = eta;
  coords[1] = xi;
  coords[0] = mu;

  // set index_size of block
  topography.getIndexSize(index_size); // in eta and xi we copy from the topography
  index_size[mu] = n_depth;            // in mu the size is given

  // we copy the index offset from the topography
  topography.getIndexOffset(index_offset);

  // in case it is on top we have subtract the index depth
  if (top)
    index_offset[mu] = index_offset[mu] - n_depth + 1;

  //--------------------//

  //--Next we span the edges of the block--//
  std::vector<int> range_eta;
  topography.getIndexRange(eta, range_eta);
  std::vector<int> range_xi;
  topography.getIndexRange(xi, range_xi);


  double offset_block[3];
  offset_block[eta] = topography(eta, eta, range_eta.front(), xi, range_xi.front());
  offset_block[xi]  = topography(xi, eta, range_eta.front(), xi, range_xi.front());
  offset_block[mu]  = topography.getPosition();
  // in case the topography is on top we have to subtract the depth
  if (top)
    offset_block[mu] -= depth;

  double size_block[3];
  size_block[eta] = topography(eta, eta, range_eta.back(), xi, range_xi.back()) - offset_block[eta];
  size_block[xi]  = topography(xi, eta, range_eta.back(), xi, range_xi.back()) - offset_block[xi];
  size_block[mu]  = depth;
  //---------------------------------------//


  double offset_other[3];
  double size_other[3];
  int    index_offset_other[3];
  int    index_size_other[3];

  std::copy_n(index_offset, 3, index_offset_other);
  std::copy_n(index_size, 3, index_size_other);
  std::copy_n(offset_block, 3, offset_other);
  std::copy_n(size_block, 3, size_other);

  if (!top) {
    index_offset_other[mu] += index_size_other[mu] - 1;
    offset_other[mu] += size_other[mu];
  }
  index_size_other[mu] = 1;
  size_other[mu]       = 0.0;

  FaceBoundary face_boundary_mu_other(offset_other, size_other, index_size_other, coords_topo);
  Face         face_mu_other(index_offset_other, face_boundary_mu_other);
  //--set faces in mu --//
  setFace(mu, top, topography); // set topography as first face
  setFace(mu, !top, face_mu_other);
  /*---------------*/

  /*--Init edges in corners of xi eta---*/
  double start_eta_false_xi_false[3];
  std::copy_n(offset_block, 3, start_eta_false_xi_false);
  double start_eta_true_xi_false[3];
  std::copy_n(offset_block, 3, start_eta_true_xi_false);
  start_eta_true_xi_false[eta] += size_block[eta];
  double start_eta_false_xi_true[3];
  std::copy_n(offset_block, 3, start_eta_false_xi_true);
  start_eta_false_xi_true[xi] += size_block[xi];
  double start_eta_true_xi_true[3];
  std::copy_n(offset_block, 3, start_eta_true_xi_true);
  start_eta_true_xi_true[eta] += size_block[eta];
  start_eta_true_xi_true[xi] += size_block[xi];
  double stop_eta_false_xi_false[3];
  std::copy_n(start_eta_false_xi_false, 3, stop_eta_false_xi_false);
  double stop_eta_true_xi_false[3];
  std::copy_n(start_eta_true_xi_false, 3, stop_eta_true_xi_false);
  double stop_eta_false_xi_true[3];
  std::copy_n(start_eta_false_xi_true, 3, stop_eta_false_xi_true);
  double stop_eta_true_xi_true[3];
  std::copy_n(start_eta_true_xi_true, 3, stop_eta_true_xi_true);

  stop_eta_false_xi_false[mu] += size_block[mu];
  stop_eta_false_xi_true[mu] += size_block[mu];
  stop_eta_true_xi_false[mu] += size_block[mu];
  stop_eta_true_xi_true[mu] += size_block[mu];


  Edge edge_eta_false_xi_false(mu, start_eta_false_xi_false, stop_eta_false_xi_false, n_depth);
  Edge edge_eta_true_xi_false(mu, start_eta_true_xi_false, stop_eta_true_xi_false, n_depth);
  Edge edge_eta_false_xi_true(mu, start_eta_false_xi_true, stop_eta_false_xi_true, n_depth);
  Edge edge_eta_true_xi_true(mu, start_eta_true_xi_true, stop_eta_true_xi_true, n_depth);

  /* std::cout << edge_eta_false_xi_false.printEdge() << std::endl; */
  /* std::cout << edge_eta_true_xi_false.printEdge() << std::endl; */
  /* std::cout << edge_eta_false_xi_true.printEdge() << std::endl; */

  FaceBoundary face_boundary_mu_true(getFace(mu, true));
  FaceBoundary face_boundary_mu_false(getFace(mu, false));

  Edge edge_eta_false_mu_false = face_boundary_mu_false.getEdge(eta, false);
  Edge edge_eta_true_mu_false  = face_boundary_mu_false.getEdge(eta, true);
  Edge edge_xi_false_mu_false  = face_boundary_mu_false.getEdge(xi, false);
  Edge edge_xi_true_mu_false   = face_boundary_mu_false.getEdge(xi, true);

  Edge edge_eta_false_mu_true = face_boundary_mu_true.getEdge(eta, false);
  Edge edge_eta_true_mu_true  = face_boundary_mu_true.getEdge(eta, true);
  Edge edge_xi_false_mu_true  = face_boundary_mu_true.getEdge(xi, false);
  Edge edge_xi_true_mu_true   = face_boundary_mu_true.getEdge(xi, true);
  /*------------------------------------*/

  /*Init faces in eta */
  Coordinate coords_neta[2] = {mu, xi};
  int        index_offset_neta_true[3];
  int        index_offset_neta_false[3];

  getFace(mu, false).getIndexOffset(index_offset_neta_true);
  getFace(mu, false).getIndexOffset(index_offset_neta_false);
  index_offset_neta_true[eta] += index_size[eta] - 1;

  FaceBoundary face_boundary_neta_false(
    coords_neta,
    offset_block[eta],
    edge_eta_false_xi_false,
    edge_eta_false_xi_true,
    edge_eta_false_mu_false,
    edge_eta_false_mu_true
  );
  FaceBoundary face_boundary_neta_true(
    coords_neta,
    offset_block[eta] + size_block[eta],
    edge_eta_true_xi_false,
    edge_eta_true_xi_true,
    edge_eta_true_mu_false,
    edge_eta_true_mu_true
  );
  setFace(eta, false, Face(index_offset_neta_false, face_boundary_neta_false));
  setFace(eta, true, Face(index_offset_neta_true, face_boundary_neta_true));

  /*--------------------------*/

  /*Init faces in xi*/
  Coordinate coords_nxi[2] = {mu, eta};
  int        index_offset_nxi_true[3];
  int        index_offset_nxi_false[3];

  getFace(mu, false).getIndexOffset(index_offset_nxi_true);
  getFace(mu, false).getIndexOffset(index_offset_nxi_false);
  index_offset_nxi_true[xi] += index_size[xi] - 1;

  FaceBoundary face_boundary_nxi_false(
    coords_nxi,
    offset_block[xi],
    edge_eta_false_xi_false,
    edge_eta_true_xi_false,
    edge_xi_false_mu_false,
    edge_xi_false_mu_true
  );

  FaceBoundary face_boundary_nxi_true(
    coords_nxi,
    offset_block[xi] + size_block[xi],
    edge_eta_false_xi_true,
    edge_eta_true_xi_true,
    edge_xi_true_mu_false,
    edge_xi_true_mu_true
  );

  setFace(xi, false, Face(index_offset_nxi_false, face_boundary_nxi_false));
  setFace(xi, true, Face(index_offset_nxi_true, face_boundary_nxi_true));
  checkConformity();
}


double& toolbox::curvi::BlockBoundary::operator()(BlockFace face, Coordinate coord, int j, int i) {
  if (face == LEFT) {
    idx3 idx(NCOORDS, index_size[1], index_size[2]);
    return left_face(coord, j, i);
  }
  if (face == RIGHT) {
    idx3 idx(NCOORDS, index_size[1], index_size[2]);
    return right_face(coord, j, i);
  }
  if (face == BOTTOM) {
    idx3 idx(NCOORDS, index_size[2], index_size[0]);
    return bottom_face(coord, j, i);
  }
  if (face == TOP) {
    idx3 idx(NCOORDS, index_size[2], index_size[0]);
    return top_face(coord, j, i);
  }
  if (face == FRONT) {
    idx3 idx(NCOORDS, index_size[1], index_size[0]);
    return front_face(coord, j, i);
  }
  if (face == BACK) {
    idx3 idx(NCOORDS, index_size[0], index_size[0]);
    return back_face(coord, j, i);
  }
};

const toolbox::curvi::Face& toolbox::curvi::BlockBoundary::getFace(BlockFace face) {
  if (face == LEFT) {
    return left_face;
  }
  if (face == RIGHT) {
    return right_face;
  }
  if (face == BOTTOM) {
    return bottom_face;
  }
  if (face == TOP) {
    return top_face;
  }
  if (face == FRONT) {
    return front_face;
  }
  if (face == BACK) {
    return back_face;
  }
}

const toolbox::curvi::Face& toolbox::curvi::BlockBoundary::getFace(Coordinate coord, bool upper) {
  if (coord == coords[2]) {
    if (upper) {
      return right_face;
    } else {
      return left_face;
    }
  }

  if (coord == coords[1]) {
    if (upper) {
      return top_face;
    } else {
      return bottom_face;
    }
  }

  if (coord == coords[0]) {
    if (upper) {
      return back_face;
    } else {
      return front_face;
    }
  }
}

void toolbox::curvi::BlockBoundary::setFace(BlockFace face, const Face& other) {
  if (face == LEFT) {
    left_face = other;
  }
  if (face == RIGHT) {
    right_face = other;
  }
  if (face == BOTTOM) {
    bottom_face = other;
  }
  if (face == TOP) {
    top_face = other;
  }
  if (face == FRONT) {
    front_face = other;
  }
  if (face == BACK) {
    back_face = other;
  }
}


void toolbox::curvi::BlockBoundary::setFace(Coordinate coord, bool upper, Face other) {
  if (coord == coords[2]) {
    if (upper) {
      right_face = other;
    } else {
      left_face = other;
    }
  }
  if (coord == coords[1]) {
    if (upper) {
      top_face = other;
    } else {
      bottom_face = other;
    }
  }

  if (coord == coords[0]) {
    if (upper) {
      back_face = other;
    } else {
      front_face = other;
    }
  }
}

void toolbox::curvi::BlockBoundary::split(Face& face, BlockBoundary& left, BlockBoundary& right) {

  /* We assume we split in the xi-mu plane*/
  Coordinate xi  = face.getCoord(1);
  Coordinate mu  = face.getCoord(0);
  Coordinate eta = getNormal(xi, mu);

  // Check if face is inside the block
  int face_index_offset[3];
  int face_index_range[3];

  // face.toNetCDF("fault_in.nc");
  face.getIndexOffset(face_index_offset);
  face.getIndexSize(face_index_range);

  bool face_is_in_block = face_index_offset[eta] > getFace(eta, false).getIndexOffset(eta);
  face_is_in_block &= face_index_offset[eta] < getFace(eta, true).getIndexOffset(eta);

  if (!face_is_in_block) {
    std::cout << "Warning splitting face is outside of block in Coordinate " << coordToString(eta) << std::endl;
    std::cout << "Spliting face index: " << face_index_offset[eta] << std::endl;
    std::cout
      << "Block index range" << getFace(eta, false).getIndexOffset(eta) << "-" << getFace(eta, true).getIndexOffset(eta)
      << std::endl;

    std::cout << "Face:" << std::endl;
    std::cout << face << std::endl;
    std::cout << "Boundary:" << std::endl;
    std::cout << toString() << std::endl;
    std::cout << getFace(eta, false) << std::endl;
    std::cout << getFace(eta, true) << std::endl;
  }

  // Check if face covers mu and xi
  bool face_covers_mu = face_index_offset[mu] < getFace(mu, false).getIndexOffset(mu);
  face_covers_mu &= face_index_offset[mu] + face_index_range[mu] > getFace(mu, true).getIndexOffset(mu);

  bool face_covers_xi = face_index_offset[xi] < getFace(xi, false).getIndexOffset(xi);
  face_covers_xi &= face_index_offset[xi] + face_index_range[xi] > getFace(xi, true).getIndexOffset(xi);

  // Set unchanged faces in resulting blocks
  left.setFace(eta, false, getFace(eta, false));
  right.setFace(eta, true, getFace(eta, true));

  // split upperFace
  Face upperLeft, upperRight;
  Edge upperPath = getFace(mu, true).split(face, upperLeft, upperRight);

  // split lowerFace
  Face lowerLeft, lowerRight;
  Edge lowerPath = getFace(mu, false).split(face, lowerLeft, lowerRight);

  // split backFace
  Face backLeft, backRight;
  Edge backPath = getFace(xi, true).split(face, backLeft, backRight);

  // split frontFace
  Face frontLeft, frontRight;
  Edge frontPath = getFace(xi, false).split(face, frontLeft, frontRight);

  Coordinate coords_fault[2] = {mu, xi};

  FaceBoundary fault_boundary(coords_fault, face.getPosition(), frontPath, backPath, lowerPath, upperPath);

  int index_offset_fault[3];
  std::copy_n(index_offset, 3, index_offset_fault);
  index_offset_fault[eta] = face_index_offset[eta];
  Face fault(index_offset_fault, fault_boundary, face.getPerturbation(), true);
  fault.toNetCDF("fault_cut.nc");

  // Set split faces in resulting blocks
  left.setFace(mu, false, lowerLeft);
  right.setFace(mu, false, lowerRight);

  left.setFace(mu, true, upperLeft);
  right.setFace(mu, true, upperRight);

  left.setFace(xi, false, frontLeft);
  right.setFace(xi, false, frontRight);

  left.setFace(xi, true, backLeft);
  right.setFace(xi, true, backRight);

  left.setFace(eta, true, fault);
  right.setFace(eta, false, fault);

  int index_offset_left[3] = {
    left.getFace(Coordinate::Z, false).getIndexOffset(Coordinate::Z),
    left.getFace(Coordinate::Y, false).getIndexOffset(Coordinate::Y),
    left.getFace(Coordinate::X, false).getIndexOffset(Coordinate::X)
  };

  left.setIndexOffset(index_offset_left);

  int index_offset_right[3];
  std::copy_n(index_offset, 3, index_offset_right);
  index_offset_right[eta] = face.getIndexOffset(eta);
  right.setIndexOffset(index_offset_right);

  int index_size_left[3];
  std::copy_n(index_size, 3, index_size_left);
  index_size_left[eta] = face.getIndexOffset(eta) - index_offset[eta] + 1;
  left.setIndexSize(index_size_left);

  int index_size_right[3];
  std::copy_n(index_size, 3, index_size_right);
  index_size_right[eta] = index_size[eta] - index_size_left[eta] + 1;
  right.setIndexSize(index_size_right);
  /**/
}

void toolbox::curvi::BlockBoundary::setCoord(Coordinate a_coords[3]) { std::copy_n(a_coords, 3, coords); }

void toolbox::curvi::BlockBoundary::getCoord(Coordinate a_coords[3]) { std::copy_n(coords, 3, a_coords); }

toolbox::curvi::Coordinate toolbox::curvi::BlockBoundary::getCoord(int i) { return coords[i]; }


void toolbox::curvi::BlockBoundary::setIndexOffset(int a_index_offset[3]) {
  std::copy_n(a_index_offset, 3, index_offset);
}

void toolbox::curvi::BlockBoundary::setIndexSize(int a_index_size[3]) { std::copy_n(a_index_size, 3, index_size); }

void toolbox::curvi::BlockBoundary::getIndexOffset(int a_index_offset[3]) {
  std::copy_n(index_offset, 3, a_index_offset);
}

int toolbox::curvi::BlockBoundary::getIndexOffset(Coordinate coord) { return index_offset[coord]; }

void toolbox::curvi::BlockBoundary::getIndexSize(int a_index_size[3]) { std::copy_n(index_size, 3, a_index_size); }

int toolbox::curvi::BlockBoundary::getIndexSize(Coordinate coord) { return index_size[coord]; }

void toolbox::curvi::BlockBoundary::getBoundingBox(double offset[3], double size[3]) {
  double coord_max[3] = {
    std::numeric_limits<double>::lowest(),
    std::numeric_limits<double>::lowest(),
    std::numeric_limits<double>::lowest()
  };
  double coord_min[3] = {
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::max()
  };

  for (int f_ind = 0; f_ind < 6; f_ind++) {
    BlockFace f = static_cast<BlockFace>(f_ind);
    for (int c_ind = 0; c_ind < 3; c_ind++) {
      Coordinate c    = static_cast<Coordinate>(c_ind);
      Face       face = getFace(f);
      Coordinate face_coord[2];
      face.getCoord(face_coord);
      std::vector<int> index_range_0;
      face.getIndexRange(face_coord[0], index_range_0);
      std::vector<int> index_range_1;
      face.getIndexRange(face_coord[1], index_range_1);
      for (int i : index_range_0) {
        for (int j : index_range_1) {
          coord_max[c] = std::max(coord_max[c], face(c, i, j));
          coord_min[c] = std::min(coord_min[c], face(c, i, j));
        }
      }
    }
  }

  for (int c_ind = 0; c_ind < 3; c_ind++) {
    offset[c_ind] = coord_min[c_ind];
    size[c_ind]   = coord_max[c_ind] - coord_min[c_ind];
  }
}

void toolbox::curvi::BlockBoundary::toNetCDF(std::string prefix) {
  for (int c_int = 0; c_int < 3; c_int++) {
    Coordinate c = static_cast<Coordinate>(c_int);
    for (bool top : {false, true}) {
      std::stringstream ss;
      ss << prefix << "_" << (top ? "true" : "false") << "_" << coordToString(c) << ".nc";
      getFace(c, top).toNetCDF(ss.str());
    }
  }
}

toolbox::curvi::FaceBoundary toolbox::curvi::BlockBoundary::
  setupSplittingFace(Coordinate cycx[2], int fault_index, double fault_position, int indexOffset_fault[3]) {

  int        dmesh         = 8;
  Coordinate c_depth_fault = getNormal(cycx[0], cycx[1]);

  double offset[3];
  double size[3];

  double offset_fault[3];
  double size_fault[3];
  getBoundingBox(offset, size); // get bounding box of boundary

  int indexSize_fault[3];
  std::copy_n(index_size, 3, indexSize_fault);
  std::copy_n(index_offset, 3, indexOffset_fault);

  std::copy_n(offset, 3, offset_fault);
  offset_fault[cycx[0]] -= size[cycx[0]] / indexSize_fault[cycx[0]] * dmesh / 2; // add buffer
  offset_fault[cycx[1]] -= size[cycx[1]] / indexSize_fault[cycx[1]] * dmesh / 2; // add buffer
  offset_fault[c_depth_fault] = fault_position;

  std::copy_n(size, 3, size_fault);
  size_fault[cycx[0]] += size[cycx[0]] / indexSize_fault[cycx[0]] * dmesh; // add buffer
  size_fault[cycx[1]] += size[cycx[1]] / indexSize_fault[cycx[1]] * dmesh; // add buffer


  indexOffset_fault[cycx[0]] -= dmesh / 2; // add buffer
  indexOffset_fault[cycx[1]] -= dmesh / 2; // add buffer
  indexOffset_fault[c_depth_fault] = fault_index;

  indexSize_fault[c_depth_fault] = 1;
  indexSize_fault[cycx[0]] += dmesh; // add buffer
  indexSize_fault[cycx[1]] += dmesh; // add buffer

  return FaceBoundary(offset_fault, size_fault, indexSize_fault, cycx);
}

std::string toolbox::curvi::BlockBoundary::toString(int depth) {
  std::stringstream ss_i;
  for (int i = 0; i < depth; i++) {
    ss_i << "\t";
  }
  std::string indent = ss_i.str();

  std::stringstream ss;
  ss << indent << "BoundaryBlock" << std::endl;
  ss << indent << "Coordinates: " << coordToString(coords[0]) << " " << coordToString(coords[1]) << " "
     << coordToString(coords[2]) << " " << std::endl;

  ss << indent << "Index Offset: " << index_offset[0] << " " << index_offset[1] << " " << index_offset[2] << " "
     << std::endl;


  ss << indent << "Index Size: " << index_size[0] << " " << index_size[1] << " " << index_size[2] << " " << std::endl;

  return ss.str();
}

std::string toolbox::curvi::BlockBoundary::printFaces() {
  std::stringstream ss;
  ss << coords[0] << " " << false << std::endl;
  ss << getFace(coords[0], false) << std::endl;
  ss << coords[0] << " " << true << std::endl;
  ss << getFace(coords[0], true) << std::endl;
  ss << coords[1] << " " << false << std::endl;
  ss << getFace(coords[1], false) << std::endl;
  ss << coords[1] << " " << true << std::endl;
  ss << getFace(coords[1], true) << std::endl;
  ss << coords[2] << " " << false << std::endl;
  ss << getFace(coords[2], false) << std::endl;
  ss << coords[2] << " " << true << std::endl;
  ss << getFace(coords[2], true) << std::endl;
  return ss.str();
}


void toolbox::curvi::BlockBoundary::checkConformity() {
  for (int c_int = 0; c_int < 3; c_int++) {
    Coordinate c = static_cast<Coordinate>(c_int);
    for (bool top : {false, true}) {

      Coordinate face_coords[2];
      getFace(c, top).getCoord(face_coords);
      int size[3];
      getFace(c, top).getIndexSize(size);
      int offset[3];
      getFace(c, top).getIndexOffset(offset);
      bool conform = true;
      // check
      conform &= index_offset[face_coords[0]] == offset[face_coords[0]];
      conform &= index_offset[face_coords[1]] == offset[face_coords[1]];
      conform &= index_size[face_coords[0]] == size[face_coords[0]];
      conform &= index_size[face_coords[1]] == size[face_coords[1]];

      Coordinate z = getNormal(face_coords[1], face_coords[0]);
      conform &= z == c;
      if (!conform) {
        std::stringstream ss;
        ss << "blockBoundary and face do not conform" << std::endl;
        ss << c << " " << top << std::endl;
        ss << toString() << std::endl;
        ss << getFace(c, top) << std::endl;
        ss << getFace(coords[0], false) << std::endl;
        ss << getFace(coords[0], true) << std::endl;
        ss << getFace(coords[1], false) << std::endl;
        ss << getFace(coords[1], true) << std::endl;
        ss << getFace(coords[2], false) << std::endl;
        ss << getFace(coords[2], true) << std::endl;
        throw std::range_error(ss.str());
      }
    }
  }
}
