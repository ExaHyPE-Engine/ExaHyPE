#ifndef TEMPLATE_DIFFUSEINTERFACE_HEADER
#define TEMPLATE_DIFFUSEINTERFACE_HEADER

#include "../../../../ExaHyPE/kernels/GaussLegendreBasis.h"
#include "../../../../ExaHyPE/kernels/KernelUtils.h"

template <class Shortcuts, int basisSize>
class DiffuseInterface {
public:
  DiffuseInterface(Topography* a_topo, DomainInformation* a_info) {
    topography = a_topo;
    info       = a_info;

    int nx = std::pow(3, info->meshLevel);
    int ny = std::pow(3, info->meshLevel);
    // TODO: how do i get a good approximation for the real buffered nz ?
    int nz = std::pow(3, info->meshLevel);

    std::vector<double> alpha;
    alpha.resize(nx * ny * nz * (basisSize) * (basisSize) * (basisSize));
    kernels::initGaussLegendreNodesAndWeights(std::set<int>()); // empty set as it is not used

    info->getDx(dx);

    patch_size = (dx * 2.0) / basisSize;

    // patch_size  = (dx * 2.0 );
    patch_dx     = dx / 3.0;
    patch_cells  = int(std::ceil(patch_size / patch_dx));
    domain_cells = int(std::ceil(info->domainSize[0] / dx));

    integration_nodes.resize(basisSize);
    integration_weights.resize(basisSize);

    for (int node = 0; node < basisSize; node++) {
      integration_nodes[node]   = kernels::legendre::nodes[basisSize - 1][node] * patch_dx;
      integration_weights[node] = kernels::legendre::weights[basisSize - 1][node] * patch_dx;
    }

    patch.resize((patch_cells * basisSize) * (patch_cells * basisSize) * (patch_cells * basisSize));

    kernels::idx6 idx(patch_cells, patch_cells, patch_cells, basisSize, basisSize, basisSize);

    volume_testfunction = 0;

    for (int cell_z = 0; cell_z < patch_cells; cell_z++) {
      double offset_z = (cell_z - patch_cells / 2.0) * patch_dx;
      for (int cell_y = 0; cell_y < patch_cells; cell_y++) {
        double offset_y = (cell_y - patch_cells / 2.0) * patch_dx;
        for (int cell_x = 0; cell_x < patch_cells; cell_x++) {
          double offset_x = (cell_x - patch_cells / 2.0) * patch_dx;
          for (int node_z = 0; node_z < basisSize; node_z++) {
            double z = offset_z + integration_nodes[node_z];
            for (int node_y = 0; node_y < basisSize; node_y++) {
              double y = offset_y + integration_nodes[node_y];
              for (int node_x = 0; node_x < basisSize; node_x++) {
                double x                                                   = offset_x + integration_nodes[node_x];
                patch[idx(cell_z, cell_y, cell_x, node_z, node_y, node_x)] = testFunction(x, y, z);

                volume_testfunction += patch[idx(cell_z, cell_y, cell_x, node_z, node_y, node_x)]
                                       * integration_weights[node_z] * integration_weights[node_y]
                                       * integration_weights[node_x];
              }
            }
          }
        }
      }
    }
    initLimits();
  };

  double getAlpha(const double* const x) {
    std::vector<double> patch_x;
    std::vector<double> patch_z;
    std::vector<double> patch_y;
    int                 cell_index = int(std::floor((x[2] - info->domainOffset[2]) / dx)) * domain_cells
                     + int(std::floor((x[0] - info->domainOffset[0]) / dx));

    if (x[1] < topography_limits_min[cell_index] - dx) {
      //      std::cout << "." << std::endl;
      return 0;
    }

    if (x[1] > topography_limits_max[cell_index] + dx) {
      //      std::cout << "." << std::endl;
      return 1;
    }

    patch_x.resize(patch_cells * basisSize);
    patch_z.resize(patch_cells * basisSize);

    for (int cell = 0; cell < patch_cells; cell++) {
      for (int node = 0; node < basisSize; node++) {
        patch_z[cell * basisSize + node] = x[2] + (cell - patch_cells / 2.0) * patch_dx + integration_nodes[node];
        patch_x[cell * basisSize + node] = x[0] + (cell - patch_cells / 2.0) * patch_dx + integration_nodes[node];
      }
    }

    topography->topography_onPatch(patch_x, patch_z, patch_y);

    std::vector<double> topography_heaviside;
    gen_heaviside_topography(x, patch_y, topography_heaviside);
    double alpha = convolution_with_patch(topography_heaviside);
    return alpha;
  };

  double testFunction(double x, double y, double z) {
    double r2       = x * x + y * y + z * z;
    double r2_patch = patch_size * patch_size;
    if (r2 > r2_patch) {
      return 0;
    } else {
      return 1.0 / (r2 + 1);
      // return std::exp(r2_patch/(r2-r2_patch));
    }
  }

  void gen_heaviside_topography(
    const double* const x, std::vector<double>& patch_y, std::vector<double>& topography_heaviside
  ) {
    kernels::idx6 idx(patch_cells, patch_cells, patch_cells, basisSize, basisSize, basisSize);
    kernels::idx2 idx2(patch_cells * basisSize, patch_cells * basisSize);

    topography_heaviside.resize(patch_cells * basisSize * patch_cells * basisSize * patch_cells * basisSize);

    for (int cell_z = 0; cell_z < patch_cells; cell_z++) {
      for (int node_z = 0; node_z < basisSize; node_z++) {
        for (int cell_x = 0; cell_x < patch_cells; cell_x++) {
          for (int node_x = 0; node_x < basisSize; node_x++) {
            for (int cell_y = 0; cell_y < patch_cells; cell_y++) {
              double offset_y = x[1] + (cell_y - patch_cells / 2.0) * patch_dx;
              for (int node_y = 0; node_y < basisSize; node_y++) {
                double y = offset_y + integration_nodes[node_y];
                topography_heaviside[idx(
                  cell_z, cell_y, cell_x, node_z, node_y, node_x
                )]       = patch_y[idx2(cell_z * basisSize + node_z, cell_x * basisSize + node_x)] < y ? 1 : 0;
              }
            }
          }
        }
      }
    }
  }

  double convolution_with_patch(std::vector<double>& topography_heaviside) {
    double        alpha = 0;
    kernels::idx6 idx(patch_cells, patch_cells, patch_cells, basisSize, basisSize, basisSize);
    for (int cell_z = 0; cell_z < patch_cells; cell_z++) {
      for (int node_z = 0; node_z < basisSize; node_z++) {
        for (int cell_x = 0; cell_x < patch_cells; cell_x++) {
          for (int node_x = 0; node_x < basisSize; node_x++) {
            for (int cell_y = 0; cell_y < patch_cells; cell_y++) {
              for (int node_y = 0; node_y < basisSize; node_y++) {
                double weight = integration_weights[node_z] * integration_weights[node_y] * integration_weights[node_x];

                alpha += topography_heaviside[idx(cell_z, cell_y, cell_x, node_z, node_y, node_x)]
                         * patch[idx(cell_z, cell_y, cell_x, node_z, node_y, node_x)] * weight;
              }
            }
          }
        }
      }
    }
    return alpha / volume_testfunction;
  }

  void initLimits() {
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;

    topography_limits_max.resize(domain_cells * domain_cells);
    topography_limits_min.resize(domain_cells * domain_cells);

    x.resize(basisSize);
    z.resize(basisSize);

    for (int i = 0; i < domain_cells; i++) {
      for (int j = 0; j < domain_cells; j++) {
        for (int node = 0; node < basisSize; node++) {
          x[node] = kernels::legendre::nodes[basisSize - 1][node] * dx + i * dx + info->domainOffset[0];
          z[node] = kernels::legendre::nodes[basisSize - 1][node] * dx + j * dx + info->domainOffset[2];
        }

        topography->topography_onPatch(x, z, y);

        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();
        for (int node = 0; node < basisSize * basisSize; node++) {
          min = std::min(min, y[node]);
          max = std::max(max, y[node]);
        }
        topography_limits_max[j * domain_cells + i] = max;
        topography_limits_min[j * domain_cells + i] = min;
      }
    }
  }

  int getArea(int x, int z) {
    int cell_x = (x - info->domainOffset[0]) / info->domainSize[0];
    int cell_z = (z - info->domainOffset[2]) / info->domainSize[2];
    return
  }

private:
  Topography*        topography;
  DomainInformation* info;

  double dx;

  double patch_size;
  double patch_dx;

  int patch_cells;
  int domain_cells;

  std::vector<double> topography_limits_max;
  std::vector<double> topography_limits_min;
  std::vector<double> patch;
  std::vector<double> integration_nodes;
  std::vector<double> integration_weights;

  double volume_testfunction;
};

#endif
