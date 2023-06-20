#ifndef TEMPLATE_DIFFUSEINTERFACE_HEADER
#define TEMPLATE_DIFFUSEINTERFACE_HEADER

#include "../../../../ExaHyPE/kernels/GaussLegendreBasis.h"
#include "../../../../ExaHyPE/kernels/KernelUtils.h"

enum Position { above, at, below };

template <class Shortcuts, int basisSize>
class DiffuseInterface {
public:
  DiffuseInterface(
    Topography*        a_topo,
    DomainInformation* a_domainInfo,
    SolverInformation* a_solverInfo,
    double             a_interface_factor,
    double             a_max_distance_to_direct_surface
  ) {
    topography = a_topo;
    info       = a_domainInfo;
    isDG       = a_solverInfo->isDG();

    max_distance_to_direct_surface = a_max_distance_to_direct_surface;

    dx = info->getDx();

    interface_factor = a_interface_factor;
    interval_size    = interface_factor * dx;

    // number of cells in one dimension
    domain_cells = int(std::ceil(info->domainSize[0] / dx));

    integration_nodes.resize(basisSize);
    for (int node = 0; node < basisSize; node++) {
      if (isDG) {
        // todo replace with solver nodes
        integration_nodes[node] = kernels::legendre::nodes[basisSize - 1][node];
      } else {
        integration_nodes[node] = (node + 1) / (basisSize + 1);
      }
    }
    initLimits();
  };

  ~DiffuseInterface() { delete topography; };

  // position 0: above
  //          1: at
  //          2: below topography
  int findRelativePosition(double& distance, double y_min, double y_max, int i, int j) {
    kernels::idx2 idx_cells(domain_cells, domain_cells);
    distance = topography_limits_min[idx_cells(j, i)] - y_max;
    if (distance > 0) {
      return Position::above;
    }

    distance = y_min - topography_limits_max[idx_cells(j, i)];
    if (distance > 0) {
      return Position::below;
    }
    distance = 0.0;
    return Position::at;
  }

  void getAlphaPatch(const double* const x, double* alpha) {
    kernels::idx2 idx_cells(domain_cells, domain_cells);
    kernels::idx4 idx(basisSize, basisSize, basisSize, Dimensions);

    constexpr int center = basisSize / 2;

    // find cell id on surface of patch
    int cell_j = static_cast<int>(std::floor((x[idx(center, center, center, 2)] - info->domainOffset[2]) / dx));
    int cell_i = static_cast<int>(std::floor((x[idx(center, center, center, 0)] - info->domainOffset[0]) / dx));

    double patch_distance = 0;
    // position 0: above 1: at 2: below topography
    int position = findRelativePosition(
      patch_distance, x[idx(0, 0, 0, 1)], x[idx(0, basisSize - 1, 0, 1)], cell_i, cell_j
    );

    // compute distance of patch to underlaying surface
    if (patch_distance >= max_distance_to_direct_surface) {
      if (position == Position::above) {
        std::fill_n(alpha, basisSize * basisSize * basisSize, 0.0);
        return;
      }
      if (position == Position::below) {
        std::fill_n(alpha, basisSize * basisSize * basisSize, 1.0);
        return;
      }
    }

    double cell_dx = (x[idx(0, basisSize - 1, 0, 1)] - x[idx(0, 0, 0, 1)]);
    // add distance from top center to edge as buffer
    patch_distance += sqrt(3.0 / 2.0) * dx;

    // minmal measuerd distance for each patch node
    double minDistance[basisSize * basisSize * basisSize];
    std::fill_n(minDistance, basisSize * basisSize * basisSize, patch_distance);
    // boolean if the patch is done
    bool done[basisSize * basisSize * basisSize];
    std::fill_n(done, basisSize * basisSize * basisSize, false);

    Position positions[basisSize * basisSize * basisSize];
    getMinDistancePatch(patch_distance, x, minDistance, positions, done);
    computeAlphaPatch(alpha, minDistance, positions);

    // std::copy_n(minDistance,basisSize*basisSize*basisSize,alpha);
  }

  void computeAlphaPatch(double* alpha, double* minDistance, Position* positions) {
    kernels::idx3 idx_alpha(basisSize, basisSize, basisSize);
    for (int node_z = 0; node_z < basisSize; node_z++) {
      for (int node_y = 0; node_y < basisSize; node_y++) {
        for (int node_x = 0; node_x < basisSize; node_x++) {
          Position position_node = positions[idx_alpha(node_z, node_y, node_x)];

          double r = minDistance[idx_alpha(node_z, node_y, node_x)] / interval_size;
          if (r < 0.5) {
            if (position_node == Position ::above)
              r = -r;
            alpha[idx_alpha(node_z, node_y, node_x)] = (std::sin(r * M_PI) + 1) * 0.5;
          } else {
            alpha[idx_alpha(node_z, node_y, node_x)] = position_node == Position::below ? 1 : 0;
          }
#ifdef Asserts
          assertion1(
            alpha[idx_alpha(node_z, node_y, node_x)] >= 0 && alpha[idx_alpha(node_z, node_y, node_x)] <= 1,
            alpha[idx_alpha(node_z, node_y, node_x)]
          );
#endif
        }
      }
    }
  }

  void spanPatchAroundCenter(
    std::vector<double>& patch_x, std::vector<double>& patch_z, double center_x, double center_z, double patch_radius
  ) {

    kernels::idx4 idx(basisSize, basisSize, basisSize, Dimensions);
    kernels::idx3 idx_node(basisSize, basisSize, basisSize);

    constexpr int center = basisSize / 2;

    // Todo: Is this basis size a valid coice?
    int                 num_circles = basisSize;
    std::vector<double> radius;
    radius.resize(num_circles);

    int num_args[num_circles];
    num_args[0] = 1;
    num_args[1] = basisSize;

    double delta_radius = patch_radius / static_cast<double>(num_circles - 1);
    for (int i = 0; i < num_circles; i++) {
      radius[i] = delta_radius * i;
    }

    int node_counter = 0;

    patch_x.resize(num_args[0]);
    patch_z.resize(num_args[0]);

    patch_x[node_counter] = center_x;
    patch_z[node_counter] = center_z;

    node_counter++;

    for (int j = 1; j < num_circles; j++) {
      num_args[j] = static_cast<int>(std::ceil(radius[j] / radius[1])) * num_args[1];
      patch_x.resize(patch_x.size() + num_args[j]);
      patch_z.resize(patch_z.size() + num_args[j]);
      for (int i = 0; i < num_args[j]; i++) {
        double arg            = 2 * M_PI * i / static_cast<double>(num_args[j]);
        patch_x[node_counter] = std::cos(arg) * radius[j] + center_x;
        patch_z[node_counter] = std::sin(arg) * radius[j] + center_z;

        /*truncate on domain boundary*/
        patch_x[node_counter] = std::min(
          std::max(patch_x[node_counter], info->domainOffset[0]), info->domainOffset[0] + info->domainSize[0]
        );
        patch_z[node_counter] = std::min(
          std::max(patch_z[node_counter], info->domainOffset[2]), info->domainOffset[2] + info->domainSize[2]
        );
        node_counter++;
      }
    }
  }

  void findMinDistanceToNode(
    double&              min_distance,
    Position&            position,
    const double* const  x,
    std::vector<double>& patch_x,
    std::vector<double>& patch_y,
    std::vector<double>& patch_z
  ) {

    double new_min_distance = std::numeric_limits<double>::max();
    for (int i = 0; i < patch_x.size(); i++) {
      double distance = std::sqrt(
        (patch_x[i] - x[0]) * (patch_x[i] - x[0]) + (patch_y[i] - x[1]) * (patch_y[i] - x[1])
        + (patch_z[i] - x[2]) * (patch_z[i] - x[2])
      );
      if (distance < new_min_distance) {
        new_min_distance = distance;
        position         = (patch_y[i] - x[1]) > 0 ? Position::above : Position::below;
      }
    }
    min_distance = new_min_distance;
  }

  void getMinDistancePatch(
    double patch_distance, const double* const x, double* min_distance, Position* positions, bool* done
  ) {

    /*span patch around center of the cell*/
    std::vector<double> patch_x;
    std::vector<double> patch_z;

    kernels::idx4 idx(basisSize, basisSize, basisSize, Dimensions);
    kernels::idx3 idx_node(basisSize, basisSize, basisSize);

    double center_x = (x[idx(0, 0, basisSize - 1, 0)] + x[idx(0, 0, 0, 0)]) / 2.0;
    double center_z = (x[idx(basisSize - 1, 0, 0, 2)] + x[idx(0, 0, 0, 2)]) / 2.0;
    spanPatchAroundCenter(patch_x, patch_z, center_x, center_z, patch_distance);

    /*evaluate topography on patch*/
    std::vector<double> patch_y;
    topography->topography_onSet(patch_x, patch_z, patch_y);

    /*compute distance to surface for each node*/

    for (int node_z = 0; node_z < basisSize; node_z++) {
      for (int node_y = 0; node_y < basisSize; node_y++) {
        for (int node_x = 0; node_x < basisSize; node_x++) {
          double   new_min_distance = 0;
          Position new_position;
          findMinDistanceToNode(
            new_min_distance, new_position, x + idx(node_z, node_y, node_x, 0), patch_x, patch_y, patch_z
          );

          // If new min distance doesn't change we are done
          if (new_min_distance < min_distance[idx_node(node_z, node_y, node_x)]) {
            positions[idx_node(node_z, node_y, node_x)]    = new_position;
            min_distance[idx_node(node_z, node_y, node_x)] = new_min_distance;
          } else {
            done[idx_node(node_z, node_y, node_x)] = true;
          }
        }
      }
    }

    bool all_done = true;
    for (int i = 0; i < basisSize * basisSize * basisSize; i++) {
      if (!done[i]) {
        all_done = false;
        break;
      }
    }

    /* if(!isDG){ */
    /*   for(int node_z = 0; node_z < basisSize; node_z++){ */
    /*     for(int node_y = 0; node_y < basisSize; node_y++){ */
    /*       for(int node_x = 0; node_x < basisSize; node_x++){ */
    /*         std::cout << min_distance[idx_node(node_z,node_y,node_x)] << std::endl; */
    /*       } */
    /*     } */
    /*   } */
    /*   std::cout << "  "<< std::endl; */
    /* } */

    if (all_done) { // no closer nodes found -> we are done
      return;
    } else {
      double patch_distance = std::numeric_limits<double>::max();

      for (int node_z = 0; node_z < basisSize; node_z++) {
        for (int node_y = 0; node_y < basisSize; node_y++) {
          for (int node_x = 0; node_x < basisSize; node_x++) {

            if (!done[idx_node(node_z, node_y, node_x)]) {
              double node_distance = 0.0;
              node_distance        = std::sqrt(
                                (x[idx(node_z, node_y, node_x, 0)] - center_x
                                ) * (x[idx(node_z, node_y, node_x, 0)] - center_x)
                                + (x[idx(node_z, node_y, node_x, 2)] - center_z
                                  ) * (x[idx(node_z, node_y, node_x, 2)] - center_z)
                              )
                              + min_distance[idx_node(node_z, node_y, node_x)];

              patch_distance = std::min(node_distance, patch_distance);
            }
          }
        }
      }
      getMinDistancePatch(patch_distance, x, min_distance, positions, done);
      return;
    }
  }

  double getAlpha(const double* const x) {
    kernels::idx2 idx2(domain_cells, domain_cells);
    int           cell_j = static_cast<int>(std::floor((x[2] - info->domainOffset[2]) / dx));
    int           cell_i = static_cast<int>(std::floor((x[0] - info->domainOffset[0]) / dx));

    double min_dist_limit = topography_limits_min[idx2(cell_j, cell_i)] - x[1];
    if (min_dist_limit > max_distance_to_direct_surface) {
      return 0;
    }

    double max_dist_limit = x[1] - topography_limits_max[idx2(cell_j, cell_i)];
    if (max_dist_limit > max_distance_to_direct_surface) {
      return 1;
    }

    double min_distance = std::min(std::abs(max_dist_limit), std::abs(min_dist_limit));
    min_distance        = getMinDistance(min_distance, x[0], x[1], x[2]);

    double alpha;
    if (std::abs(min_distance) < interval_size * 0.5) {
      double r = min_distance / interval_size;
      alpha    = (std::sin(r * M_PI) + 1) * 0.5;
    } else {
      alpha = min_distance < 0 ? 0 : 1;
    }
    return alpha;
  };

  double getMinDistance(double max_distance, double x, double y, double z) {

    std::vector<double> patch_x;
    std::vector<double> patch_z;
    std::vector<double> patch_y;
    std::vector<double> radius;

    int num_circles = 5; // Todo: Is this a valid choice?
    int num_args[num_circles];
    num_args[0] = 1;
    num_args[1] = 10; // Todo: Is this a valid coice?

    radius.resize(num_circles);

    double delta_radius = std::abs(max_distance) / static_cast<double>(num_circles - 1);
    for (int i = 0; i < num_circles; i++) {
      radius[i] = delta_radius * i;
    }

    int node_counter = 0;

    patch_x.resize(1);
    patch_z.resize(1);
    patch_x[0] = x;
    patch_z[0] = z;

    node_counter++;

    for (int j = 1; j < num_circles; j++) {
      num_args[j] = static_cast<int>(std::ceil(radius[j] / radius[1])) * num_args[1];
      patch_x.resize(patch_x.size() + num_args[j]);
      patch_z.resize(patch_z.size() + num_args[j]);
      for (int i = 0; i < num_args[j]; i++) {
        double arg            = 2 * M_PI * i / static_cast<double>(num_args[j]);
        patch_x[node_counter] = std::cos(arg) * radius[j] + x;
        patch_z[node_counter] = std::sin(arg) * radius[j] + z;

        patch_x[node_counter] = std::min(
          std::max(patch_x[node_counter], info->domainOffset[0]), info->domainOffset[0] + info->domainSize[0]
        );
        patch_z[node_counter] = std::min(
          std::max(patch_z[node_counter], info->domainOffset[2]), info->domainOffset[2] + info->domainSize[2]
        );

        node_counter++;
      }
    }

    topography->topography_onSet(patch_x, patch_z, patch_y);

    double new_max_distance = max_distance;
    double distance;
    node_counter = 0;
    for (int j = 0; j < num_circles; j++) {
      for (int i = 0; i < num_args[j]; i++) {
        distance = std::sqrt((patch_y[node_counter] - y) * (patch_y[node_counter] - y) + radius[j] * radius[j]);
        if (std::abs(new_max_distance) > std::abs(distance)) {
          new_max_distance = distance;
        }
        node_counter++;
      }
    }

    if (std::abs(new_max_distance - max_distance) < 1.0e-10) { // no closer nodes found
      return max_distance;
    } else {
      return getMinDistance(new_max_distance, x, y, z);
    }
  }

  void initLimits() {
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;

    topography_limits_max.resize(domain_cells * domain_cells);
    topography_limits_min.resize(domain_cells * domain_cells);

    x.resize(basisSize);
    z.resize(basisSize);

    kernels::idx2 idx2(domain_cells, domain_cells);
    for (int j = 0; j < domain_cells; j++) {
      for (int i = 0; i < domain_cells; i++) {
        for (int node = 0; node < basisSize; node++) {
          if (isDG) {
            x[node] = kernels::legendre::nodes[basisSize - 1][node] * dx + i * dx + info->domainOffset[0];
            z[node] = kernels::legendre::nodes[basisSize - 1][node] * dx + j * dx + info->domainOffset[2];
          } else {
            x[node] = (node + 1) / static_cast<double>(basisSize + 1) * dx + i * dx + info->domainOffset[0];
            z[node] = (node + 1) / static_cast<double>(basisSize + 1) * dx + j * dx + info->domainOffset[2];
          }
        }

        topography->topography_onPatch(x, z, y);

        double min = std::numeric_limits<double>::max();
        double max = -std::numeric_limits<double>::max();

        for (int node = 0; node < basisSize * basisSize; node++) {
          min = std::min(min, y[node]);
          max = std::max(max, y[node]);
        }
        topography_limits_max[idx2(j, i)] = max;
        topography_limits_min[idx2(j, i)] = min;
      }
    }
  }

private:
  Topography*        topography;
  DomainInformation* info;

  bool isDG;

  double dx;
  double interval_size;
  double max_distance_to_direct_surface;
  double interface_factor;
  int    domain_cells;

  std::vector<double> topography_limits_max;
  std::vector<double> topography_limits_min;
  std::vector<double> integration_nodes;

  double volume_testfunction;
};

#endif
