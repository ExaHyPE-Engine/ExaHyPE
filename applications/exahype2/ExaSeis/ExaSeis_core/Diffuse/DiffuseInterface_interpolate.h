#ifndef TEMPLATE_DIFFUSEINTERFACE_HEADER
#define TEMPLATE_DIFFUSEINTERFACE_HEADER

#include "../../../../ExaHyPE/kernels/GaussLegendreBasis.h"
#include "../../../../ExaHyPE/kernels/KernelUtils.h"

template <class Shortcuts, int basisSize>
class DiffuseInterface {
public:
  DiffuseInterface(Topography* a_topo, DomainInformation* a_info, int a_interface_factor) {
    interface_factor = a_interface_factor;
    topography       = a_topo;
    info             = a_info;
    isDG             = a_info->isDG();

    kernels::initGaussLegendreNodesAndWeights(std::set<int>()); // empty set as it is not used

    info->getDx(dx);

    interval_size  = dx / basisSize * interface_factor;
    max_patch_size = (dx * 5.0);

    _nx = int(std::ceil(info->domainSize[0] / dx));
    _nz = int(std::ceil(info->domainSize[2] / dx));

    integration_nodes.resize(basisSize);

    topo_x.resize(_nx);
    topo_z.resize(_nz);
    topo_y.resize(_nx * _nz);

    normals_x.resize(_nx * _nz);
    normals_y.resize(_nx * _nz);
    normals_z.resize(_nx * _nz);

    for (int node = 0; node < basisSize; node++) {
      if (isDG) {
        integration_nodes[node] = kernels::legendre::nodes[basisSize - 1][node];
      } else {
        integration_nodes[node] = (node + 1) / (basisSize + 1);
      }
    }
    initCells();
  };

  void getAlphaPatch(const double* const x, double* alpha) {
    kernels::idx2 idx2(_nz, _nx);
    int           j = static_cast<int>(std::floor((x[2] - info->domainOffset[2]) / dx));
    int           i = static_cast<int>(std::floor((x[0] - info->domainOffset[0]) / dx));

    kernels::idx4 id_xyzn(basisSize, basisSize, basisSize, Dimensions);

    double min_dist_limit = topography_limits_min[idx2(cell_j, cell_i)] - x[id_xyzn(0, basisSize - 1, 0, 1)];
    if (min_dist_limit > max_patch_size) {
      std::fill_n(alpha, 0.0, basisSize * basisSize * basisSize);
      return;
    }

    double max_dist_limit = x[id_xyzn(0, 0, 0, 1)] - topography_limits_max[idx2(cell_j, cell_i)];
    if (max_dist_limit > max_patch_size) {
      std::fill_n(alpha, 0.0, basisSize * basisSize * basisSize);
      return;
    }

    // identify position of topography relative to cell
    for (int node_z = 0; node_z < basisSize; node_z++) {
      for (int node_x = 0; node_x < basisSize; node_x++) {
        topo_y[idy(i, node_z, j, node_x)];
      }
    }
  }

  void initCells() {
    topography_limits_max.resize(domain_cells * domain_cells);
    topography_limits_min.resize(domain_cells * domain_cells);

    kernels::idx2 id_cells(_nz, _nx);
    kernels::idx2 idx(_nx, basisSize);
    kernels::idx2 idz(_nz, basisSize);
    kernels::idx3 idy(_nz, basisSize, _nx, basisSize);

    for (int i = 0; i < _nx; i++) {
      for (int node = 0; node < basisSize; node++) {
        if (isDG) {
          topo_x[idx(i, node)] = kernels::legendre::nodes[basisSize - 1][node] * dx + i * dx + info->domainOffset[0];
        } else {
          topo_x[idx(i, node)] = (node + 1) / static_cast<double>(basisSize + 1) * dx + i * dx + info->domainOffset[0];
        }
      }
    }

    for (int i = 0; i < _nz; i++) {
      for (int node = 0; node < basisSize; node++) {
        if (isDG) {
          topo_z[idz(i, node)] = kernels::legendre::nodes[basisSize - 1][node] * dx + i * dx + info->domainOffset[2];
        } else {
          topo_z[idz(i, node)] = (node + 1) / static_cast<double>(basisSize + 1) * dx + i * dx + info->domainOffset[2];
        }
      }
    }

    topography->topography_onPatch(topo_x, topo_z, topo_y);
    for (int j = 0; j < _nz; j++) {
      for (int i = 0; i < _nx; i++) {
        double min = std::numeric_limits<double>::max();
        double max = -std::numeric_limits<double>::max();
        for (int node_z = 0; node_z < basisSize; node_z++) {
          for (int node_x = 0; node_x < basisSize; node_x++) {
            min = std::min(min, y[idy(j, node_z, i, node_x)]);
            max = std::max(max, y[idy(j, node_z, i, node_x)]);
          }
        }
        topography_limits_max[id_cells(j, i)] = max;
        topography_limits_min[id_cells(j, i)] = min;
      }
    }

    // init normals
    double grad_x[basisSize];
    for (int j = 0; j < _nz; j++) {
      for (int i = 0; i < _nx; i++) {
        kernels::idx2 idx_grad(basisSize, basisSize);
        double        grad_x[basisSize * basisSize];
        double        grad_z[basisSize * basisSize];
        // compute gradient
        for (int node_z = 0; node_z < basisSize; node_z++) {
          for (int node_x = 0; node_x < basisSize; node_x++) {
            for (int n = 0; n < basisSize; n++) {
              grad_x[idx_grad(
                node_z, node_x
              )] += kernels::legendre::dudx[num_nodes - 1][node_x][n] * topo_y[idy(j, node_z, i, n)];
              grad_z[idx_grad(
                node_z, node_x
              )] += kernels::legendre::dudx[num_nodes - 1][node_z][n] * topo_y[idy(j, n, i, node_x)];
            }
          }
        }
      }
    }
  }

private:
  Topography*        topography;
  DomainInformation* info;

  bool   isDG;
  double dx;

  double interval_size;
  double interface_factor;

  double max_patch_size;

  int _nx;
  int _nz;

  std::vector<double> topography_limits_max;
  std::vector<double> topography_limits_min;

  std::vector<double> integration_nodes;

  std::vector<double> topo_x;
  std::vector<double> topo_y;
  std::vector<double> topo_z;

  std::vector<double> normals_x;
  std::vector<double> normals_y;
  std::vector<double> normals_z;
};

#endif
