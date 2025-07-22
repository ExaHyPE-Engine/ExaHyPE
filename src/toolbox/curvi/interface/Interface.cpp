
#include "toolbox/curvi/interface/Interface.h"

toolbox::curvi::Interface::
  Interface(std::string file, double a_offset[3], double a_size[3], uint a_elements[3], int a_order):
  order(a_order) {

  YAMLInput input(file);
  std::copy_n(a_offset, 3, offset);
  std::copy_n(a_size, 3, size);
  std::copy_n(a_elements, 3, elements);

  /*Setup domain*/
  input.parseModes(modes);

  for (int d = 0; d < 3; d++) {
    grid[d] = elements[d] * (modes[d] - 1) + 1;
  }

  std::cout << "Domain" << std::endl;
  std::cout << "Offset:\t[" << offset[0] << ", " << offset[1] << ", " << offset[2] << "]" << std::endl;
  std::cout << "Size:\t[" << size[0] << ", " << size[1] << ", " << size[2] << "]" << std::endl;
  std::cout << "Grid:\t[" << grid[0] << ", " << grid[1] << ", " << grid[2] << "]" << std::endl;
  /*-------------*/

  std::cout << "Settings" << std::endl;
  std::cout
    << "Elts:\t[" << elements[Coordinate::Z] << ", " << elements[Coordinate::Y] << ", " << elements[Coordinate::X]
    << "]" << std::endl;
  std::cout
    << "Modes:\t[" << modes[Coordinate::Z] << ", " << modes[Coordinate::Y] << ", " << modes[Coordinate::X] << "]"
    << std::endl;
  std::cout << "Order:\t" << order << std::endl;

  /*Init Topography*/
  root = input.parse(offset, size, grid);
};

toolbox::curvi::Interface::~Interface() { delete root; }

void toolbox::curvi::Interface::initTree() {
  root->init(modes);
  std::cout << root->toString() << std::endl;
}

toolbox::curvi::Root* toolbox::curvi::Interface::getRoot() { return root; }


double toolbox::curvi::Interface::invertProjection(Coordinate coord, double coords[3]) {

  Coordinate mu = coord;
  Coordinate xi, eta;
  getPlane(mu, xi, eta);

  BlockBoundary boundary = root->traverseProjection(coords);

  Face upper = boundary.getFace(mu, true);
  Face lower = boundary.getFace(mu, false);


  std::vector<int> range_xi;
  lower.getIndexRange(xi, range_xi);
  std::vector<int> range_eta;
  lower.getIndexRange(eta, range_eta);
  double offset_xi  = lower(xi, xi, range_xi.front(), eta, range_eta.front());
  double offset_eta = lower(eta, xi, range_xi.front(), eta, range_eta.front());
  double size_xi    = lower(xi, xi, range_xi.back(), eta, range_eta.front()) - offset_xi;
  double size_eta   = lower(eta, xi, range_xi.front(), eta, range_eta.back()) - offset_eta;


  double index_xi  = (coords[xi] - offset_xi) / size_xi * (range_xi.back() - range_xi.front()) + range_xi.front();
  double index_eta = (coords[eta] - offset_eta) / size_eta * (range_eta.back() - range_eta.front()) + range_eta.front();

  int index_xi_l  = std::floor(index_xi);
  int index_eta_l = std::floor(index_eta);
  int index_xi_c  = std::ceil(index_xi);
  int index_eta_c = std::ceil(index_eta);

  // interpolate
  // lagrange coefficients
  double l_xi_c  = (index_xi - index_xi_l) / double(index_xi_c - index_xi_l);
  double l_xi_l  = (index_xi - index_xi_c) / double(index_xi_l - index_xi_c);
  double l_eta_c = (index_eta - index_eta_l) / double(index_eta_c - index_eta_l);
  double l_eta_l = (index_eta - index_eta_c) / double(index_eta_l - index_eta_c);

  double mu_u = l_xi_c * l_eta_c * upper(mu, xi, index_xi_c, eta, index_eta_c)
                + l_xi_c * l_eta_l * upper(mu, xi, index_xi_c, eta, index_eta_l)
                + l_xi_l * l_eta_c * upper(mu, xi, index_xi_l, eta, index_eta_c)
                + l_xi_l * l_eta_l * upper(mu, xi, index_xi_l, eta, index_eta_l);

  double mu_l = l_xi_c * l_eta_c * lower(mu, xi, index_xi_c, eta, index_eta_c)
                + l_xi_c * l_eta_l * lower(mu, xi, index_xi_c, eta, index_eta_l)
                + l_xi_l * l_eta_c * lower(mu, xi, index_xi_l, eta, index_eta_c)
                + l_xi_l * l_eta_l * lower(mu, xi, index_xi_l, eta, index_eta_l);

  double rel_pos_mu;
  rel_pos_mu       = (coords[mu] - mu_l) / (mu_u - mu_l);
  double rel_index = (rel_pos_mu * boundary.getIndexSize(mu) + boundary.getIndexOffset(mu)) / grid[mu];
  return rel_index * size[mu] + offset[mu];
}

toolbox::curvi::Block toolbox::curvi::Interface::getElement(double* nodes, int ez, int ey, int ex) {
  /*Check if element is in boundary*/
  if ((ez >= 0 && ey >= 0 && ex >= 0)
      && (ez < elements[Coordinate::Z] && ey < elements[Coordinate::Y] && ex < elements[Coordinate::X])) {


    int index_size[3];
    index_size[Coordinate::X] = modes[Coordinate::X];
    index_size[Coordinate::Y] = modes[Coordinate::Y];
    index_size[Coordinate::Z] = modes[Coordinate::Z];

    int index_offset[3];
    index_offset[Coordinate::X] = (modes[Coordinate::X] - 1) * ex;
    index_offset[Coordinate::Y] = (modes[Coordinate::Y] - 1) * ey;
    index_offset[Coordinate::Z] = (modes[Coordinate::Z] - 1) * ez;

    int center_index[3];
    std::copy_n(index_offset, 3, center_index);

    center_index[0]++;
    center_index[1]++;
    center_index[2]++;

    BlockBoundary boundary = root->traverse(center_index);
    Block         element(boundary, index_offset, index_size);


    std::function<void(const Block&, int, int, int, Block&)> interpolation =
      [&](const Block& in, int elt_z, int elt_y, int elt_x, Block& out) {
        std::vector<int> range_z;
        in.getIndexRange(
          Coordinate::Z,
          elt_z * (modes[Coordinate::Z] - 1),
          (elt_z + 1) * (modes[Coordinate::Z] - 1) + 1,
          range_z
        );
        std::vector<int> range_y;
        in.getIndexRange(
          Coordinate::Y,
          elt_y * (modes[Coordinate::Y] - 1),
          (elt_y + 1) * (modes[Coordinate::Y] - 1) + 1,
          range_y
        );
        std::vector<int> range_x;
        in.getIndexRange(
          Coordinate::X,
          elt_x * (modes[Coordinate::X] - 1),
          (elt_x + 1) * (modes[Coordinate::X] - 1) + 1,
          range_x
        );

        int index_offset_out[3];
        out.getIndexOffset(index_offset_out);

        std::vector<int> range_z_in;
        in.getIndexRange(Coordinate::Z, range_z_in);
        std::vector<int> range_y_in;
        in.getIndexRange(Coordinate::Y, range_y_in);
        std::vector<int> range_x_in;
        in.getIndexRange(Coordinate::X, range_x_in);

        double position[3];
        for (int k = 0; k < order + 1; k++) {
          int idx_z   = index_offset_out[Coordinate::Z] + k;
          position[0] = range_z.front() + nodes[k] * (modes[Coordinate::Z] - 1);
          for (int j = 0; j < order + 1; j++) {
            int idx_y   = index_offset_out[Coordinate::Y] + j;
            position[1] = range_y.front() + nodes[j] * (modes[Coordinate::Y] - 1);
            for (int i = 0; i < order + 1; i++) {
              int idx_x   = index_offset_out[Coordinate::X] + i;
              position[2] = range_x.front() + nodes[i] * (modes[Coordinate::X] - 1);

              double result[NCOORDS] = {0.0, 0.0, 0.0};
              Interpolation::Lagrange::interpolate3D(position, in, range_z, range_y, range_x, result);

              for (int d = 0; d < NCOORDS; d++) {
                out(d, Coordinate::Z, idx_z, Coordinate::Y, idx_y, Coordinate::X, idx_x) = result[d];
              }
            }
          }
        }
      };

    return element.resample(
      interpolation,
      modes[Coordinate::Z],
      modes[Coordinate::Y],
      modes[Coordinate::X],
      order + 1,
      order + 1,
      order + 1
    );
  }
}