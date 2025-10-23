#include "toolbox/curvi/transformation/Transformation.h"

void toolbox::curvi::Transformation::metricDerivativesAndJacobian3D(
  double**            dudx,
  const Block&        in,
  const double* const dx,
  std::vector<int>    range_z,
  std::vector<int>    range_y,
  std::vector<int>    range_x,
  Block&              out
) {

  double jacobian;
  double q_x, r_x, s_x;
  double q_y, r_y, s_y;
  double q_z, r_z, s_z;

  double x_der_x, x_der_y, x_der_z;
  double y_der_x, y_der_y, y_der_z;
  double z_der_x, z_der_y, z_der_z;

  std::vector<double> gl_vals;
  gl_vals.resize(range_z.size() * range_y.size() * range_x.size() * NCOORDS);
  idx4    id_xyz(NCOORDS, range_z.size(), range_y.size(), range_x.size());
  double* gl_vals_x = gl_vals.data() + id_xyz(Coordinate::X, 0, 0, 0);
  double* gl_vals_y = gl_vals.data() + id_xyz(Coordinate::Y, 0, 0, 0);
  double* gl_vals_z = gl_vals.data() + id_xyz(Coordinate::Z, 0, 0, 0);

  for (int k : range_z) {
    for (int j : range_y) {
      for (int i : range_x) {
        // std::cout << "k " << k << " j " << j << " i " << i << std::endl;
        for (int d = 0; d < NCOORDS; d++) {
          gl_vals[id_xyz(in.getCoord(d), k - range_z.front(), j - range_y.front(), i - range_x.front())] = in(
            in.getCoord(d),
            Coordinate::Z,
            k,
            Coordinate::Y,
            j,
            Coordinate::X,
            i
          );
          // in(in.getCoord(d),k,j,i);
        }
      }
    }
  }

  for (int k : range_z) {
    int k_0 = k - range_z.front();
    for (int j : range_y) {
      int j_0 = j - range_y.front();
      for (int i : range_x) {
        int i_0 = i - range_x.front();
        computeDerivatives_x_3D(dudx, k_0, j_0, i_0, gl_vals_x, range_x.size(), x_der_x, dx[0]);
        computeDerivatives_y_3D(dudx, k_0, j_0, i_0, gl_vals_x, range_y.size(), x_der_y, dx[1]);
        computeDerivatives_z_3D(dudx, k_0, j_0, i_0, gl_vals_x, range_z.size(), x_der_z, dx[2]);

        computeDerivatives_x_3D(dudx, k_0, j_0, i_0, gl_vals_y, range_x.size(), y_der_x, dx[0]);
        computeDerivatives_y_3D(dudx, k_0, j_0, i_0, gl_vals_y, range_y.size(), y_der_y, dx[1]);
        computeDerivatives_z_3D(dudx, k_0, j_0, i_0, gl_vals_y, range_z.size(), y_der_z, dx[2]);

        computeDerivatives_x_3D(dudx, k_0, j_0, i_0, gl_vals_z, range_x.size(), z_der_x, dx[0]);
        computeDerivatives_y_3D(dudx, k_0, j_0, i_0, gl_vals_z, range_y.size(), z_der_y, dx[1]);
        computeDerivatives_z_3D(dudx, k_0, j_0, i_0, gl_vals_z, range_z.size(), z_der_z, dx[2]);

        //	  std::cout << std::endl;

        double jacobian = x_der_x * (y_der_y * z_der_z - y_der_z * z_der_y)
                          - x_der_y * (y_der_x * z_der_z - y_der_z * z_der_x)
                          + x_der_z * (y_der_x * z_der_y - y_der_y * z_der_x);

        q_x = (1.0 / jacobian) * (y_der_y * z_der_z - z_der_y * y_der_z);
        r_x = (1.0 / jacobian) * (z_der_x * y_der_z - y_der_x * z_der_z);
        s_x = (1.0 / jacobian) * (y_der_x * z_der_y - z_der_x * y_der_y);

        q_y = (1.0 / jacobian) * (z_der_y * x_der_z - x_der_y * z_der_z);
        r_y = (1.0 / jacobian) * (x_der_x * z_der_z - z_der_x * x_der_z);
        s_y = (1.0 / jacobian) * (z_der_x * x_der_y - x_der_x * z_der_y);

        q_z = (1.0 / jacobian) * (x_der_y * y_der_z - y_der_y * x_der_z);
        r_z = (1.0 / jacobian) * (y_der_x * x_der_z - x_der_x * y_der_z);
        s_z = (1.0 / jacobian) * (x_der_x * y_der_y - y_der_x * x_der_y);

        out(Coordinate::X, Coordinate::Z, k, Coordinate::Y, j, Coordinate::X, i) = std::sqrt(
          q_x * q_x + q_y * q_y + q_z * q_z
        );
        out(Coordinate::Y, Coordinate::Z, k, Coordinate::Y, j, Coordinate::X, i) = std::sqrt(
          r_x * r_x + r_y * r_y + r_z * r_z
        );
        out(Coordinate::Z, Coordinate::Z, k, Coordinate::Y, j, Coordinate::X, i) = std::sqrt(
          s_x * s_x + s_y * s_y + s_z * s_z
        );
      }
    }
  }
}


void toolbox::curvi::Transformation::
  computeDerivatives_x_3D(double** dudx, int k, int j, int i, double* values, int num_nodes, double& der_x, double dx) {
  idx3 id_xyz(num_nodes, num_nodes, num_nodes);
  der_x = 0.0;
  for (int n = 0; n < num_nodes; n++) {
    der_x += dudx[i][n] * values[id_xyz(k, j, n)] / dx;
  }
}

void toolbox::curvi::Transformation::
  computeDerivatives_y_3D(double** dudx, int k, int j, int i, double* values, int num_nodes, double& der_y, double dy) {
  idx3 id_xyz(num_nodes, num_nodes, num_nodes);
  der_y = 0.0;
  for (int n = 0; n < num_nodes; n++) {
    // std::cout << values[id_xyz(k,n,i)] << std::endl;
    der_y += dudx[j][n] * values[id_xyz(k, n, i)] / dy;
  }
}

void toolbox::curvi::Transformation::
  computeDerivatives_z_3D(double** dudx, int k, int j, int i, double* values, int num_nodes, double& der_z, double dz) {
  idx3 id_xyz(num_nodes, num_nodes, num_nodes);
  der_z = 0.0;
  for (int n = 0; n < num_nodes; n++) {
    der_z += dudx[k][n] * values[id_xyz(n, j, i)] / dz;
  }
}