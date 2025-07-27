#pragma once

#include "../Numerics/idx.h"

namespace ExaSeis {
  template <class Shortcuts, typename T, int num_nodes, int numberOfData>
  class Derivatives {
  public:
    static void metricDerivatives(
      double dudx[][num_nodes], const T* const coordinates, const double* const dx, T* derivatives
    ) {
      T x_der_x, x_der_y, x_der_z;
      T y_der_x, y_der_y, y_der_z;
      T z_der_x, z_der_y, z_der_z;

      kernels::idx4 id_der(num_nodes, num_nodes, num_nodes, 10);

      for (int k = 0; k < num_nodes; k++) {
        for (int j = 0; j < num_nodes; j++) {
          for (int i = 0; i < num_nodes; i++) {

            computeDerivatives_x_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 0, x_der_x, dx[0]);
            computeDerivatives_y_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 0, x_der_y, dx[1]);
            computeDerivatives_z_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 0, x_der_z, dx[2]);
            computeDerivatives_x_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 1, y_der_x, dx[0]);
            computeDerivatives_y_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 1, y_der_y, dx[1]);
            computeDerivatives_z_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 1, y_der_z, dx[2]);
            computeDerivatives_x_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 2, z_der_x, dx[0]);
            computeDerivatives_y_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 2, z_der_y, dx[1]);
            computeDerivatives_z_3D(dudx, k, j, i, coordinates, Shortcuts::curve_grid + 2, z_der_z, dx[2]);

            T jacobian = x_der_x * (y_der_y * z_der_z - y_der_z * z_der_y)
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
      double dudx[][num_nodes], int k, int j, int i, const T* values, int coordinate, T& der_x, const double dx
    ) {

      kernels::idx4 id_xyz(num_nodes, num_nodes, num_nodes, numberOfData);
      der_x = 0.0;
      for (int n = 0; n < num_nodes; n++) {
        der_x += dudx[i][n] * values[id_xyz(k, j, n, coordinate)] / dx;
      }
    }

    static void computeDerivatives_y_3D(
      double dudx[][num_nodes], int k, int j, int i, const T* values, int coordinate, T& der_y, const double dy
    ) {

      kernels::idx4 id_xyz(num_nodes, num_nodes, num_nodes, numberOfData);
      der_y = 0.0;
      for (int n = 0; n < num_nodes; n++) {
        der_y += dudx[j][n] * values[id_xyz(k, n, i, coordinate)] / dy;
      }
    }

    static void computeDerivatives_z_3D(
      double dudx[][num_nodes], int k, int j, int i, const T* values, int coordinate, T& der_z, const double dz
    ) {

      kernels::idx4 id_xyz(num_nodes, num_nodes, num_nodes, numberOfData);

      der_z = 0.0;
      for (int n = 0; n < num_nodes; n++) {
        der_z += dudx[k][n] * values[id_xyz(n, j, i, coordinate)] / dz;
      }
    }
  };
} // namespace ExaSeis
