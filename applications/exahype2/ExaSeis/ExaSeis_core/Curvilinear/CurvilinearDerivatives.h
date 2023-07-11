#ifndef EXASEIS_TEMPLATE_CURVILINEAR_DERIVATIVES
#define EXASEIS_TEMPLATE_CURVILINEAR_DERIVATIVES
// #include "kernels/KernelUtils.h"
#include "exahype2/aderdg/kernels/Utils/KernelUtils.h"

namespace ExaSeis {
  template <class Shortcuts, int num_nodes>
  class Derivatives {
  public:
    static void metricDerivatives(
      double dudx[][num_nodes], const double* const coordinates, const double* const dx, double* derivatives
    ) {
      Shortcuts s;

      double x_der_x, x_der_y, x_der_z;
      double y_der_x, y_der_y, y_der_z;
      double z_der_x, z_der_y, z_der_z;

      kernels::idx4 id_der(num_nodes, num_nodes, num_nodes, 10);

      for (int k = 0; k < num_nodes; k++) {
        for (int j = 0; j < num_nodes; j++) {
          for (int i = 0; i < num_nodes; i++) {

            computeDerivatives_x_3D(dudx, k, j, i, coordinates, s.curve_grid + 0, x_der_x, dx[0]);
            computeDerivatives_y_3D(dudx, k, j, i, coordinates, s.curve_grid + 0, x_der_y, dx[1]);
            computeDerivatives_z_3D(dudx, k, j, i, coordinates, s.curve_grid + 0, x_der_z, dx[2]);
            computeDerivatives_x_3D(dudx, k, j, i, coordinates, s.curve_grid + 1, y_der_x, dx[0]);
            computeDerivatives_y_3D(dudx, k, j, i, coordinates, s.curve_grid + 1, y_der_y, dx[1]);
            computeDerivatives_z_3D(dudx, k, j, i, coordinates, s.curve_grid + 1, y_der_z, dx[2]);
            computeDerivatives_x_3D(dudx, k, j, i, coordinates, s.curve_grid + 2, z_der_x, dx[0]);
            computeDerivatives_y_3D(dudx, k, j, i, coordinates, s.curve_grid + 2, z_der_y, dx[1]);
            computeDerivatives_z_3D(dudx, k, j, i, coordinates, s.curve_grid + 2, z_der_z, dx[2]);

            double jacobian = x_der_x * (y_der_y * z_der_z - y_der_z * z_der_y)
                              - x_der_y * (y_der_x * z_der_z - y_der_z * z_der_x)
                              + x_der_z * (y_der_x * z_der_y - y_der_y * z_der_x);

            derivatives[id_der(k, j, i, 0)] = jacobian;
            derivatives[id_der(k, j, i, 1)] = (1.0 / jacobian) * (y_der_y * z_der_z - z_der_y * y_der_z);
            derivatives[id_der(k, j, i, 4)] = (1.0 / jacobian) * (z_der_x * y_der_z - y_der_x * z_der_z);
            derivatives[id_der(k, j, i, 7)] = (1.0 / jacobian) * (y_der_x * z_der_y - z_der_x * y_der_y);

            derivatives[id_der(k, j, i, 2)] = (1.0 / jacobian) * (z_der_y * x_der_z - x_der_y * z_der_z);
            derivatives[id_der(k, j, i, 5)] = (1.0 / jacobian) * (x_der_x * z_der_z - z_der_x * x_der_z);
            derivatives[id_der(k, j, i, 8)] = (1.0 / jacobian) * (z_der_x * x_der_y - x_der_x * z_der_y);

            derivatives[id_der(k, j, i, 3)] = (1.0 / jacobian) * (x_der_y * y_der_z - y_der_y * x_der_z);
            derivatives[id_der(k, j, i, 6)] = (1.0 / jacobian) * (y_der_x * x_der_z - x_der_x * y_der_z);
            derivatives[id_der(k, j, i, 9)] = (1.0 / jacobian) * (x_der_x * y_der_y - y_der_x * x_der_y);
          }
        }
      }
    }

  private:
    static void computeDerivatives_x_3D(
      double dudx[][num_nodes], int k, int j, int i, const double* values, int coordinate, double& der_x, const double dx
    ) {
      Shortcuts s;

      kernels::idx4 id_xyz(num_nodes, num_nodes, num_nodes, s.Size);
      der_x = 0.0;
      for (int n = 0; n < num_nodes; n++) {
        der_x += dudx[i][n] * values[id_xyz(k, j, n, coordinate)] / dx;
      }
    }

    static void computeDerivatives_y_3D(
      double dudx[][num_nodes], int k, int j, int i, const double* values, int coordinate, double& der_y, const double dy
    ) {
      Shortcuts     s;
      kernels::idx4 id_xyz(num_nodes, num_nodes, num_nodes, s.Size);
      der_y = 0.0;
      for (int n = 0; n < num_nodes; n++) {
        der_y += dudx[j][n] * values[id_xyz(k, n, i, coordinate)] / dy;
      }
    }

    static void computeDerivatives_z_3D(
      double dudx[][num_nodes], int k, int j, int i, const double* values, int coordinate, double& der_z, const double dz
    ) {
      Shortcuts     s;
      kernels::idx4 id_xyz(num_nodes, num_nodes, num_nodes, s.Size);

      der_z = 0.0;
      for (int n = 0; n < num_nodes; n++) {
        der_z += dudx[k][n] * values[id_xyz(n, j, i, coordinate)] / dz;
      }
    }
  };
} // namespace ExaSeis

#endif
