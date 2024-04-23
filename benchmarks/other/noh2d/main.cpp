//#define DISABLE_TRACING

#include <spacetree/spacetree.h>
#include <spacetree/initial_conditions.h>
#include <spacetree/sort.h>

#include <lang/trace.h>
#include <io/writer/dma.h>
#include <vtk/writer/point.h>

using Geo = Geometry<_2D, double>;

#include "align.h"
#include "hydro_part.h"
#include "hydro_iact.h"

#include "setup.h"

#define dt_therm DT_DRIFT
#define dt_kick_therm DT_KICK_HYDRO

using Tree = Spacetree<Geo, part, 64, 2, ThreadPoolTaskBackend>;

struct HYDRO_STATS_T {
    float h_max = 0;
    float h_max_active = 0;
} HYDRO_STATS;

const auto HYDRO_PROPS = hydro_props{
        .eta_neighbours = 1.2348,
        .h_tolerance = 1e-4,
        .h_max = 4.0 / N_PART,
        .h_min = 1e-5,
        .max_smoothing_iterations = 30,
        .use_mass_weighted_num_ngb = 0,
        .minimal_internal_energy = 0.,
        .viscosity = {
                .alpha = hydro_props_default_viscosity_alpha
        },
};

const auto COSMO = cosmology{
        .a2_inv = 1.0,
        .a_factor_internal_energy = 1.0,
        .a_factor_Balsara_eps = 1.0,
        .H = 0.0,
};

#include "runner_do_ghost.h"

auto GetDefaultParticle = [](const Geo::Point &pos) {
    auto v = pos - 0.5;
    auto v_norm = std::sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] /= v_norm;
    v[1] /= v_norm;
    auto p = part{
            ._x = pos,
            .v = {(float) -v[0], (float) -v[1], 0},
            .mass = 1.0 / N_PART / N_PART,
            .h = 4.f / N_PART,
            .u = 1.0e-6 / (5.0 / 3 - 1),
    };
    hydro_first_init_part(&p, &p.xpart);
    hydro_convert_quantities(&p, &p.xpart, &COSMO, &HYDRO_PROPS, nullptr);
    return p;
};

auto createTree() {
    auto tree = Tree({Geo::Region::fromInclusiveBounds(Geo::Point(0), Geo::Point(1))});
    tree.split(3);

    tree.onNode(InitialConditionsTask<Tree, GetDefaultParticle>(Geo::Region::fromInclusiveBounds(Geo::Point(0.005), Geo::Point(0.995)), N_PART), 8u);

    return tree;
}

auto ReferenceDensityKernel = [](Tree *leaf) {
    for (int i = 0; i < leaf->items.size(); i++) {
        auto &localPart = leaf->items[i];
        hydro_init_part(&localPart, nullptr);
    }

    do_density(leaf);

    runner_do_ghost(leaf, &HYDRO_PROPS, &HYDRO_STATS);
};

auto ReferenceHydroKernel = [](Tree *supernode, Tree *leaf) {
    constexpr float a = 1.0f; // SWIFT parameter, ignored by the Minimal SPH scheme
    constexpr float H = 1.0f; // SWIFT parameter, ignored by the Minimal SPH scheme

    if (leaf->items.empty()) [[unlikely]] return;

    auto activeRegion = Tree::Geo::Region(leaf->range.low - 0.001, leaf->range.high + 0.001);
    auto *activeRegionParent = leaf->getRegionParent(activeRegion, true);
    auto activeRegionIter = Tree::InsideRegionLeafsIterator(activeRegionParent, activeRegion);

    auto nonsymIact = [](Tree * __restrict__ leaf, Tree * __restrict__ activeCell) {
        for (auto localIdx = 0; localIdx < leaf->items.size(); localIdx++) {
            auto *pi = &leaf->items[localIdx];
            auto hi = pi->h;
            auto hig = hi * kernel_gamma;
            auto hig2 = hig * hig;

            if (!activeCell->range.overlaps({pi->_x - hig, pi->_x + hig})) continue;

            for (auto activeIdx = 0; activeIdx < activeCell->items.size(); activeIdx++) {
                auto *pj = &activeCell->items[activeIdx];
                auto r2 = 0.0f;
                float dx[3] = {0};

                for (int k = 0; k < Geo::D::N; k++) {
                    dx[k] = pi->_x[k] - pj->_x[k];
                    r2 += dx[k] * dx[k];
                }

                auto hjg = pj->h * kernel_gamma;
                auto hjg2 = hjg * hjg;

                /* Hit or miss? */
                if (r2 < hig2 | r2 < hjg2) {

                    /* Interact */
                    runner_iact_nonsym_force(r2, dx, hi, pj->h, pi, pj, a, H);
                }
            }
        }
    };

    auto symIact = [](Tree * __restrict__ leaf, Tree * __restrict__ activeCell) {
        for (auto localIdx = 0; localIdx < leaf->items.size(); localIdx++) {
            auto *pi = &leaf->items[localIdx];
            auto hi = pi->h;
            auto hig = hi * kernel_gamma;
            auto hig2 = hig * hig;

            if (!activeCell->range.overlaps({pi->_x - hig, pi->_x + hig})) continue;

            for (auto activeIdx = 0; activeIdx < activeCell->items.size(); activeIdx++) {
                auto *pj = &activeCell->items[activeIdx];
                auto r2 = 0.0f;
                float dx[3] = {0};

                for (int k = 0; k < Geo::D::N; k++) {
                    dx[k] = pi->_x[k] - pj->_x[k];
                    r2 += dx[k] * dx[k];
                }

                auto hjg = pj->h * kernel_gamma;
                auto hjg2 = hjg * hjg;

                assert(r2 != 0)

                /* Hit or miss? */
                if (r2 < hig2 | r2 < hjg2) {

                    /* Interact */
                    runner_iact_force(r2, dx, hi, pj->h, pi, pj, a, H);
                }
            }
        }
    };

    auto selfIact = [](Tree *leaf) {
        for (auto localIdx = 0; localIdx < leaf->items.size(); localIdx++) {
            auto *pi = &leaf->items[localIdx];
            auto hi = pi->h;
            auto hig = hi * kernel_gamma;
            auto hig2 = hig * hig;

            for (auto activeIdx = localIdx + 1; activeIdx < leaf->items.size(); activeIdx++) {
                auto *pj = &leaf->items[activeIdx];
                auto r2 = 0.0f;
                float dx[3] = {0};

                for (int k = 0; k < Geo::D::N; k++) {
                    dx[k] = pi->_x[k] - pj->_x[k];
                    r2 += dx[k] * dx[k];
                }

                assert(r2 != 0)

                auto hjg = pj->h * kernel_gamma;
                auto hjg2 = hjg * hjg;

                /* Hit or miss? */
                if (r2 < hig2 | r2 < hjg2) {

                    /* Interact */
                    runner_iact_force(r2, dx, hi, pj->h, pi, pj, a, H);
                }
            }
        }
    };

    auto *activeCell = (Tree *) nullptr;
    while ((activeCell = activeRegionIter.next().value_or(nullptr))) {
        auto isActiveCellWithinSupernode = supernode->range.contains(activeCell->range);
        if (!isActiveCellWithinSupernode) [[unlikely]] {
            nonsymIact(leaf, activeCell);
            continue;
        }

        auto isActiveCellLocalCell = leaf == activeCell;
        if (isActiveCellLocalCell) [[unlikely]] {
            selfIact(leaf);
            continue;
        }

        auto isLocalLowerAddressThanActive = (u64) leaf < (u64) activeCell;
        if (isLocalLowerAddressThanActive) symIact(leaf, activeCell);
    }
};

auto Kick1Kernel = [](Tree *leaf) {
    for (int i = 0; i < leaf->items.size(); i++) {
        auto &localPart = leaf->items[i];

        localPart.xpart.v_full[0] += localPart.a_hydro[0] * DT_KICK_HYDRO;
        localPart.xpart.v_full[1] += localPart.a_hydro[1] * DT_KICK_HYDRO;

        hydro_kick_extra(&localPart, &localPart.xpart, dt_kick_therm, 0, 0, 0, 0, &COSMO, &HYDRO_PROPS, nullptr);
    }
};

auto Kick2Kernel = [](Tree *leaf) {
    for (int i = 0; i < leaf->items.size(); i++) {
        auto &localPart = leaf->items[i];
        hydro_end_force(&localPart, &COSMO);
    }

    for (int i = 0; i < leaf->items.size(); i++) {
        auto &localPart = leaf->items[i];

        localPart.xpart.v_full[0] += localPart.a_hydro[0] * DT_KICK_HYDRO;
        localPart.xpart.v_full[1] += localPart.a_hydro[1] * DT_KICK_HYDRO;

        hydro_kick_extra(&localPart, &localPart.xpart, dt_kick_therm, 0, 0, 0, 0, &COSMO, &HYDRO_PROPS, nullptr);

        hydro_reset_predicted_values(&localPart, &localPart.xpart, &COSMO, nullptr);
    }
};

auto DriftKernel = [](Tree *leaf) {
    for (int i = 0; i < leaf->items.size(); i++) {
        auto &localPart = leaf->items[i];

        localPart._x[0] += localPart.xpart.v_full[0] * DT_DRIFT;
        localPart._x[1] += localPart.xpart.v_full[1] * DT_DRIFT;

        /* Predict velocities (for hydro terms) */
        localPart.v[0] += localPart.a_hydro[0] * DT_KICK_HYDRO;
        localPart.v[1] += localPart.a_hydro[1] * DT_KICK_HYDRO;

        hydro_predict_extra(&localPart, &localPart.xpart, DT_DRIFT, dt_therm, 0, &COSMO, &HYDRO_PROPS, nullptr, nullptr);

        /* Compute offsets since last cell construction */
        for (int k = 0; k < 2; k++) {
            const float dx = localPart.xpart.v_full[k] * DT_DRIFT;
            localPart.xpart.x_diff[k] -= dx;
            localPart.xpart.x_diff_sort[k] -= dx;
        }
    }
};

void dumpData(Tree &q0, int i) {
    char filePathBuf[128];
    sprintf(filePathBuf, "./noh2d_%04d.vtk", i);
    auto fd = open(filePathBuf, DMA_FLAGS, 0644);
    assert(fd > 0)

    auto fileWriter = DefaultDmaWriter(fd);
    auto particlesWriter = VtkPointBinaryWriter<decltype(fileWriter), double>(&fileWriter);

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.add(p._x[0], p._x[1], 0);
    }, (u32) 1);

    particlesWriter.addPointDataHeader();

    particlesWriter.addScalarPointDataHeader("id");

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.addScalar(p.id);
    }, (u32) 1);

    particlesWriter.addScalarPointDataHeader("density");

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.addScalar(p.rho);
    }, (u32) 1);

    particlesWriter.addScalarPointDataHeader("pressure");

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.addScalar(p.force.pressure);
    }, (u32) 1);

    particlesWriter.addScalarPointDataHeader("mass");

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.addScalar(p.mass);
    }, (u32) 1);

    particlesWriter.addScalarPointDataHeader("smoothing_length");

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.addScalar(p.h);
    }, (u32) 1);

    particlesWriter.addScalarPointDataHeader("internal_energy");

    q0.onNode([&](auto *node) {
        for (auto &p: node->items) particlesWriter.addScalar(p.u);
    }, (u32) 1);

    particlesWriter.finish();
    close(fd);
}

int main() {
    TRACE("Initial conditions", auto tree = createTree();)
    TRACE("Initial Density", tree.onNode([](auto *node) { node->onNode(ReferenceDensityKernel, 1u); },
                                          Tree::LeafOrLevelIterator<LEVEL>(&tree));)
    TRACE("Initial Hydro", tree.onNode([](auto *node) { node->onNode([supernode = node](auto *node) {
        ReferenceHydroKernel(supernode, node);
    }, 1u); }, Tree::LeafOrLevelIterator<LEVEL>(&tree));)

    for (int i = 0; i < STEPS; i++) {
        TRACE("Timestep", {
            TRACE("Data dump",
                if (i % 10 == 0 && STEPS_BETWEEN_DUMPS != 0) dumpData(tree, i / STEPS_BETWEEN_DUMPS);
            )

            TRACE("Kick2 + Drift + Sort", {
               auto strayParticles = (SpacetreeSorter<Tree, [](auto *node){ Kick2Kernel(node); DriftKernel(node); }>(&tree).run());
               assert(strayParticles.empty())
            })

           TRACE("Kick1 + Density", tree.onNode([](auto *node) { node->onNode([](auto *node) {
                Kick1Kernel(node);
                ReferenceDensityKernel(node);
            }, 1u); }, Tree::LeafOrLevelIterator<LEVEL>(&tree));)

            TRACE("Hydro", tree.onNode([](auto *node) { node->onNode([supernode = node](auto *node) {
                ReferenceHydroKernel(supernode, node);
            }, 1u); }, Tree::LeafOrLevelIterator<LEVEL>(&tree));)
        })
    }

    return 0;
}
