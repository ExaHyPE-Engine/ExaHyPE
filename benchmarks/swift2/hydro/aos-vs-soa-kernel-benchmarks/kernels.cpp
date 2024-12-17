#include <algorithm>
#include <cmath>

#include "setup.h"
#include "kernels.h"

HYDRO_STATS_T HYDRO_STATS {
    .h_max = 0,
    .h_max_active = 0,
} ;

const hydro_props HYDRO_PROPS{
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

const cosmology COSMO {
        .a2_inv = 1.0,
        .a_factor_internal_energy = 1.0,
        .a_factor_Balsara_eps = 1.0,
        .H = 0.0,
};

/*
 * Remarks
 *
 * 1. Originally the velocity calculation part is implemented as a loop.
 * This is incompatible with the AoS-SoA loop transformation, because
 * the transformation needs the indices to be constexpr-evaluable at
 * post-processing time (i.e. before unrolling which only happens within LLVM).
 *
 * 2. Originally the hydro_kick_extra part is implemented as a function call.
 * In principle, the AoS-SoA transformation could inline and rewrite the body
 * automatically (at least in many cases), but this is not the top priority now.
 *
 * 3. I had to replace the MAX and MIN macros with std::max/min function calls.
 * Somehow, AST nodes derived from macro expansions behave differently, and my
 * AST queries don't find them.
 */
template<typename part>
void Kick1Kernel(part *particles, int size) {
    [[clang::soa_conversion_target(particles)]]
    [[clang::soa_conversion_target_size(size)]]
    [[clang::soa_conversion_allocation_strategy(stack)]]
    #ifndef USE_VIEWS
    #include "part_full_soa.h"
    #else
    [[clang::soa_conversion_data_item("xpart.v_full[0]", "xpart.v_full[0]")]]
    [[clang::soa_conversion_data_item("a_hydro[0]", "")]]
    [[clang::soa_conversion_data_item("xpart.v_full[1]", "xpart.v_full[1]")]]
    [[clang::soa_conversion_data_item("a_hydro[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("xpart.v_full[2]", "xpart.v_full[2]")]]
    [[clang::soa_conversion_data_item("a_hydro[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("u_dt", "u_dt")]]
    [[clang::soa_conversion_data_item("_u_full", "_u_full")]]
    [[clang::soa_conversion_data_item("rho", "")]]
    #endif
    for (int pIdx = 0; pIdx < size; pIdx++) {
        particles[pIdx]._v_full[0] += particles[pIdx]._a[0] * DT_KICK_HYDRO;
        particles[pIdx]._v_full[1] += particles[pIdx]._a[1] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v_full[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_kick_extra
        const auto delta_u = particles[pIdx]._uDot * DT_KICK_THERM;
        particles[pIdx]._u_full = std::max(particles[pIdx]._u_full + delta_u, 0.5 * particles[pIdx]._u_full);
        const auto floor_A = 0.0; // does not exist in the minimal SPH scheme
        const auto floor_u = gas_internal_energy_from_entropy(particles[pIdx]._density, floor_A);
        const auto min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        const auto energy_min = std::max(min_u, floor_u);

        if (particles[pIdx]._u_full < energy_min) {
            particles[pIdx]._u_full = energy_min;
            particles[pIdx]._uDot = 0.0;
        }
    }
}

template<typename part>
void Kick2Kernel(part *particles, int size) {
    [[clang::soa_conversion_target(particles)]]
    [[clang::soa_conversion_target_size(size)]]
    [[clang::soa_conversion_allocation_strategy(stack)]]
    #ifndef USE_VIEWS
    #include "part_full_soa.h"
    #else
    [[clang::soa_conversion_data_item("xpart.v_full[0]", "xpart.v_full[0]")]]
    [[clang::soa_conversion_data_item("a_hydro[0]", "")]]
    [[clang::soa_conversion_data_item("v[0]", "v[0]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[1]", "xpart.v_full[1]")]]
    [[clang::soa_conversion_data_item("a_hydro[1]", "")]]
    [[clang::soa_conversion_data_item("v[1]", "v[1]")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("xpart.v_full[2]", "xpart.v_full[2]")]]
    [[clang::soa_conversion_data_item("a_hydro[2]", "")]]
    [[clang::soa_conversion_data_item("v[2]", "v[2]")]]
    #endif
    [[clang::soa_conversion_data_item("u_dt", "u_dt")]]
    [[clang::soa_conversion_data_item("_u_full", "_u_full")]]
    [[clang::soa_conversion_data_item("rho", "")]]
    [[clang::soa_conversion_data_item("u", "u")]]
    [[clang::soa_conversion_data_item("force.pressure", "force.pressure")]]
    [[clang::soa_conversion_data_item("force.soundspeed", "force.soundspeed")]]
    [[clang::soa_conversion_data_item("force.v_sig", "force.v_sig")]]
    #endif
    for (int pIdx = 0; pIdx < size; pIdx++) {
        particles[pIdx]._hDot *= particles[pIdx]._smoothingLength* hydro_dimension_inv;

        particles[pIdx]._v_full[0] += particles[pIdx]._a[0] * DT_KICK_HYDRO;
        particles[pIdx]._v_full[1] += particles[pIdx]._a[1] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v_full[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_kick_extra
        const auto delta_u = particles[pIdx]._uDot * DT_KICK_THERM;
        particles[pIdx]._u_full = std::max(particles[pIdx]._u_full + delta_u, 0.5 * particles[pIdx]._u_full);
        const auto floor_A = 0.0; // does not exist in the minimal SPH scheme
        const auto floor_u = gas_internal_energy_from_entropy(particles[pIdx]._density, floor_A);
        const auto min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        const auto energy_min = std::max(min_u, floor_u);

        if (particles[pIdx]._u_full < energy_min) {
            particles[pIdx]._u_full = energy_min;
            particles[pIdx]._uDot = 0.0;
        }

        // hydro_reset_predicted_values
        particles[pIdx]._v[0] = particles[pIdx]._v_full[0];
        particles[pIdx]._v[1] = particles[pIdx]._v_full[1];

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v[2] = particles[pIdx]._v_full[2];
        #endif

        /* Re-set the entropy */
        particles[pIdx]._u = particles[pIdx]._u_full;

        /* Re-compute the pressure */
        const auto pressure = gas_pressure_from_internal_energy(particles[pIdx]._density, particles[pIdx]._u);

        /* Compute the new sound speed */
        const auto soundspeed = gas_soundspeed_from_pressure(particles[pIdx]._density, pressure);

        /* Update variables */
        particles[pIdx]._pressure = pressure;
        particles[pIdx]._soundSpeed = soundspeed;

        particles[pIdx]._v_sig_AV = std::max(particles[pIdx]._v_sig_AV, 2.0 * soundspeed);
    }
}

template<typename part>
void DriftKernel(part *particles, int size) {
    [[clang::soa_conversion_target(particles)]]
    [[clang::soa_conversion_target_size(size)]]
    [[clang::soa_conversion_allocation_strategy(stack)]]
    #ifndef USE_VIEWS
    #include "part_full_soa.h"
    #else
    [[clang::soa_conversion_data_item("x[0]", "x[0]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[0]", "")]]
    [[clang::soa_conversion_data_item("x[1]", "x[1]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("x[2]", "x[2]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("v[0]", "v[0]")]]
    [[clang::soa_conversion_data_item("a_hydro[0]", "")]]
    [[clang::soa_conversion_data_item("v[1]", "v[1]")]]
    [[clang::soa_conversion_data_item("a_hydro[2]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("v[2]", "v[2]")]]
    [[clang::soa_conversion_data_item("a_hydro[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("u", "u")]]
    [[clang::soa_conversion_data_item("u_dt", "")]]
    [[clang::soa_conversion_data_item("h", "h")]]
    [[clang::soa_conversion_data_item("force.h_dt", "")]]
    [[clang::soa_conversion_data_item("rho", "rho")]]
    [[clang::soa_conversion_data_item("force.pressure", "force.pressure")]]
    [[clang::soa_conversion_data_item("force.soundspeed", "force.soundspeed")]]
    [[clang::soa_conversion_data_item("force.v_sig", "force.v_sig")]]
    [[clang::soa_conversion_data_item("xpart.x_diff[0]", "xpart.x_diff[0]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff_sort[0]", "xpart.x_diff_sort[0]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff[1]", "xpart.x_diff[1]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff_sort[1]", "xpart.x_diff_sort[1]")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("xpart.x_diff[2]", "xpart.x_diff[2]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff_sort[2]", "xpart.x_diff_sort[2]")]]
    #endif
    #endif
    for (int pIdx = 0; pIdx < size; pIdx++) {
        particles[pIdx]._x[0] += particles[pIdx]._v_full[0] * DT_DRIFT;
        particles[pIdx]._x[1] += particles[pIdx]._v_full[1] * DT_DRIFT;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._x[2] += particles[pIdx]._v_full[2] * DT_DRIFT;
        #endif

        particles[pIdx]._v[0] += particles[pIdx]._a[0] * DT_KICK_HYDRO;
        particles[pIdx]._v[1] += particles[pIdx]._a[1] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_predict_extra
        particles[pIdx]._u += particles[pIdx]._uDot * DT_THERM;

        const auto h_inv = 1.0 / particles[pIdx].h;

        const auto w1 = particles[pIdx]._hDot * h_inv * DT_DRIFT;
        if (std::abs(w1) < 0.2f) {
            particles[pIdx]._smoothingLength*= approx_expf(w1); /* 4th order expansion of exp(w) */
        } else {
            particles[pIdx]._smoothingLength*= std::expf(w1);
        }

        const auto w2 = -hydro_dimension * w1;
        if (std::abs(w2) < 0.2f) {
            particles[pIdx]._density *= approx_expf(w2); /* 4th order expansion of exp(w) */
        } else {
            particles[pIdx]._density *= std::expf(w2);
        }

        const auto floor_A = 0.0; // does not exist in the minimal SPH scheme
        const auto floor_u = gas_internal_energy_from_entropy(particles[pIdx]._density, floor_A);
        const auto min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        particles[pIdx]._u = std::max(particles[pIdx].u, floor_u);
        particles[pIdx]._u = std::max(particles[pIdx].u, min_u);

        const auto pressure = gas_pressure_from_internal_energy(particles[pIdx]._density, particles[pIdx]._u);
        const auto soundspeed = gas_soundspeed_from_pressure(particles[pIdx]._density, pressure);
        particles[pIdx]._pressure = pressure;
        particles[pIdx]._soundSpeed = soundspeed;
        particles[pIdx]._v_sig_AV = std::max(particles[pIdx]._v_sig_AV, 2.0 * soundspeed);

        // compute offsets since last cell construction
        const auto dx0 = particles[pIdx]._v_full[0] * DT_DRIFT;
        particles[pIdx].xpart.x_diff[0] -= dx0;
        particles[pIdx].xpart.x_diff_sort[0] -= dx0;

        const auto dx1 = particles[pIdx]._v_full[1] * DT_DRIFT;
        particles[pIdx].xpart.x_diff[1] -= dx1;
        particles[pIdx].xpart.x_diff_sort[1] -= dx1;

        #ifdef HYDRO_DIMENSION_3D
        const auto dx2 = particles[pIdx]._v_full[2] * DT_DRIFT;
        particles[pIdx].xpart.x_diff[2] -= dx2;
        particles[pIdx].xpart.x_diff_sort[2] -= dx2;
        #endif
    }
}

template<typename part>
void DriftKernel_MASKED(part *particles, int size) {
    [[clang::soa_conversion_target(particles)]]
    [[clang::soa_conversion_target_size(size)]]
    [[clang::soa_conversion_allocation_strategy(stack)]]
    #ifndef USE_VIEWS
    #include "part_full_soa.h"
    #else
    [[clang::soa_conversion_data_item("x[0]", "x[0]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[0]", "")]]
    [[clang::soa_conversion_data_item("x[1]", "x[1]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("x[2]", "x[2]")]]
    [[clang::soa_conversion_data_item("xpart.v_full[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("v[0]", "v[0]")]]
    [[clang::soa_conversion_data_item("a_hydro[0]", "")]]
    [[clang::soa_conversion_data_item("v[1]", "v[1]")]]
    [[clang::soa_conversion_data_item("a_hydro[2]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("v[2]", "v[2]")]]
    [[clang::soa_conversion_data_item("a_hydro[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("u", "u")]]
    [[clang::soa_conversion_data_item("u_dt", "")]]
    [[clang::soa_conversion_data_item("h", "h")]]
    [[clang::soa_conversion_data_item("force.h_dt", "")]]
    [[clang::soa_conversion_data_item("rho", "rho")]]
    [[clang::soa_conversion_data_item("force.pressure", "force.pressure")]]
    [[clang::soa_conversion_data_item("force.soundspeed", "force.soundspeed")]]
    [[clang::soa_conversion_data_item("force.v_sig", "force.v_sig")]]
    [[clang::soa_conversion_data_item("xpart.x_diff[0]", "xpart.x_diff[0]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff_sort[0]", "xpart.x_diff_sort[0]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff[1]", "xpart.x_diff[1]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff_sort[1]", "xpart.x_diff_sort[1]")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("xpart.x_diff[2]", "xpart.x_diff[2]")]]
    [[clang::soa_conversion_data_item("xpart.x_diff_sort[2]", "xpart.x_diff_sort[2]")]]
    #endif
    #endif
    for (int pIdx = 0; pIdx < size; pIdx++) {
        particles[pIdx]._x[0] += particles[pIdx]._v_full[0] * DT_DRIFT;
        particles[pIdx]._x[1] += particles[pIdx]._v_full[1] * DT_DRIFT;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._x[2] += particles[pIdx]._v_full[2] * DT_DRIFT;
        #endif

        particles[pIdx]._v[0] += particles[pIdx]._a[0] * DT_KICK_HYDRO;
        particles[pIdx]._v[1] += particles[pIdx]._a[1] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_predict_extra
        particles[pIdx]._u += particles[pIdx]._uDot * DT_THERM;

        const auto h_inv = 1.0 / particles[pIdx]._smoothingLength;

        const auto w1 = particles[pIdx]._hDot * h_inv * DT_DRIFT;
//        if (std::abs(w1) < 0.2f) {
//            particles[pIdx]._smoothingLength*= approx_expf(w1); /* 4th order expansion of exp(w) */
//        } else {
//            particles[pIdx]._smoothingLength*= std::expf(w1);
//        }

        const bool w1_MASK = std::abs(w1) < 0.2f;
        particles[pIdx]._smoothingLength*= (w1_MASK * approx_expf(w1) + !w1_MASK * std::expf(w1));

        const auto w2 = -hydro_dimension * w1;
//        if (std::abs(w2) < 0.2f) {
//            particles[pIdx]._density *= approx_expf(w2); /* 4th order expansion of exp(w) */
//        } else {
//            particles[pIdx]._density *= std::expf(w2);
//        }

        const bool w2_MASK = std::abs(w2) < 0.2f;
        particles[pIdx]._density *= (w2_MASK * approx_expf(w2) + !w2_MASK * std::expf(w2));

        const auto floor_A = 0.0; // does not exist in the minimal SPH scheme
        const auto floor_u = gas_internal_energy_from_entropy(particles[pIdx]._density, floor_A);
        const auto min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        particles[pIdx]._u = std::max(particles[pIdx]._u, (double) floor_u);
        particles[pIdx]._u = std::max(particles[pIdx]._u, (double) min_u);

        const auto pressure = gas_pressure_from_internal_energy(particles[pIdx]._density, particles[pIdx]._u);
        const auto soundspeed = gas_soundspeed_from_pressure(particles[pIdx]._density, pressure);
        particles[pIdx]._pressure = pressure;
        particles[pIdx]._soundSpeed = soundspeed;
        particles[pIdx]._v_sig_AV = std::max(particles[pIdx]._v_sig_AV, 2.0 * soundspeed);
    }
}

template<typename part>
void HydroKernel(part *pLocal, int pLocalSize, part *pActive, int pActiveSize) {
    constexpr auto a = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme
    constexpr auto H = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme

    [[clang::soa_conversion_target(pLocal)]]
    [[clang::soa_conversion_target_size(pLocalSize)]]
    [[clang::soa_conversion_allocation_strategy(stack)]]
    #ifndef USE_VIEWS
    #include "part_full_soa.h"
    #else
    [[clang::soa_conversion_data_item("x[0]", "")]]
    [[clang::soa_conversion_data_item("x[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("x[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("h", "")]]
    [[clang::soa_conversion_data_item("mass", "")]]
    [[clang::soa_conversion_data_item("rho", "")]]
    [[clang::soa_conversion_data_item("force.pressure", "")]]
    [[clang::soa_conversion_data_item("force.f", "")]]
    [[clang::soa_conversion_data_item("v[0]", "")]]
    [[clang::soa_conversion_data_item("v[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("v[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("force.soundspeed", "")]]
    [[clang::soa_conversion_data_item("force.balsara", "")]]
    [[clang::soa_conversion_data_item("u_dt", "u_dt")]]
    [[clang::soa_conversion_data_item("a_hydro[0]", "a_hydro[0]")]]
    [[clang::soa_conversion_data_item("a_hydro[1]", "a_hydro[1]")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("a_hydro[2]", "a_hydro[2]")]]
    #endif
    [[clang::soa_conversion_data_item("force.h_dt", "force.h_dt")]]
    [[clang::soa_conversion_data_item("force.v_sig", "force.v_sig")]]
    #endif
    for (int pLocalIdx = 0; pLocalIdx < pLocalSize; pLocalIdx++) {

        auto hi = pLocal[pLocalIdx].h;
        auto hig = hi * kernel_gamma;
        auto hig2 = hig * hig;
        const auto mi = pLocal[pLocalIdx].mass;
        const auto rhoi = pLocal[pLocalIdx]._density;
        const auto pressurei = pLocal[pLocalIdx]._pressure;

        [[clang::soa_conversion_target(pActive)]]
        [[clang::soa_conversion_target_size(pActiveSize)]]
        [[clang::soa_conversion_data_movement_strategy(move_to_outer)]]
        [[clang::soa_conversion_allocation_strategy(stack)]]
        #ifndef USE_VIEWS
        #include "part_full_soa.h"
        #else
        [[clang::soa_conversion_data_item("x[0]", "")]]
        [[clang::soa_conversion_data_item("x[1]", "")]]
        #ifdef HYDRO_DIMENSION_3D
        [[clang::soa_conversion_data_item("x[2]", "")]]
        #endif
        [[clang::soa_conversion_data_item("h", "")]]
        [[clang::soa_conversion_data_item("mass", "")]]
        [[clang::soa_conversion_data_item("rho", "")]]
        [[clang::soa_conversion_data_item("force.pressure", "")]]
        [[clang::soa_conversion_data_item("force.f", "")]]
        [[clang::soa_conversion_data_item("v[0]", "")]]
        [[clang::soa_conversion_data_item("v[1]", "")]]
        #ifdef HYDRO_DIMENSION_3D
        [[clang::soa_conversion_data_item("v[2]", "")]]
        #endif
        [[clang::soa_conversion_data_item("force.soundspeed", "")]]
        [[clang::soa_conversion_data_item("force.balsara", "")]]
        #endif
        for (int pActiveIdx = 0; pActiveIdx < pActiveSize; pActiveIdx++) {
            auto r2 = 0.0;
            double dx[3] = {0};

            dx[0] = pLocal[pLocalIdx]._x[0] - pActive[pActiveIdx]._x[0];
            r2 += dx[0] * dx[0];
            dx[1] = pLocal[pLocalIdx]._x[1] - pActive[pActiveIdx]._x[1];
            r2 += dx[1] * dx[1];

            #ifdef HYDRO_DIMENSION_3D
            dx[2] = pLocal[pLocalIdx]._x[2] - pActive[pActiveIdx]._x[2];
            r2 += dx[2] * dx[2];
            #endif

            auto hjg = pActive[pActiveIdx]._smoothingLength* kernel_gamma;
            auto hjg2 = hjg * hjg;

            /* Hit or miss? */
            if ((r2 > 0.0) & (r2 < hig2 | r2 < hjg2)) {

                /* Cosmological factors entering the EoMs */
                const auto fac_mu = pow_three_gamma_minus_five_over_two(a);
                const auto a2_Hubble = a * a * H;

                /* Get r and 1/r. */
                const auto r = std::sqrtf(r2);
                const auto r_inv = r ? 1.0 / r : 0.0;

                /* Recover some data */
                const auto mj = pActive[pActiveIdx].mass;
                const auto rhoj = pActive[pActiveIdx]._density;
                const auto pressurej = pActive[pActiveIdx]._pressure;

                /* Get the kernel for hi. */
                const auto hi_inv = 1.0 / hi;
                const auto hid_inv = pow_dimension_plus_one(hi_inv); /* 1/h^(d+1) */
                const auto xi = r * hi_inv;
                double wi, wi_dx;
                kernel_deval(xi, &wi, &wi_dx);
                const double wi_dr = hid_inv * wi_dx;

                /* Get the kernel for hj. */
                const auto hj_inv = 1.0 / pActive[pActiveIdx].h;
                const auto hjd_inv = pow_dimension_plus_one(hj_inv); /* 1/h^(d+1) */
                const auto xj = r * hj_inv;
                double wj, wj_dx;
                kernel_deval(xj, &wj, &wj_dx);
                const double wj_dr = hjd_inv * wj_dx;

                /* Variable smoothing length term */
                const auto f_ij = 1.0 - pLocal[pLocalIdx].force.f / mj;
                const auto f_ji = 1.0 - pActive[pActiveIdx].force.f / mi;

                /* Compute gradient terms */
                const auto P_over_rho2_i = pressurei / (rhoi * rhoi) * f_ij;
                const auto P_over_rho2_j = pressurej / (rhoj * rhoj) * f_ji;


                /* Compute dv dot r. */
                #ifdef HYDRO_DIMENSION_3D
                const auto dvdr = (pLocal[pLocalIdx]._v[0] - pActive[pActiveIdx]._v[0]) * dx[0] +
                                   (pLocal[pLocalIdx]._v[1] - pActive[pActiveIdx]._v[1]) * dx[1] +
                                   (pLocal[pLocalIdx]._v[2] - pActive[pActiveIdx]._v[2]) * dx[2];
                #else
                const auto dvdr = (pLocal[pLocalIdx]._v[0] - pActive[pActiveIdx]._v[0]) * dx[0] +
                                   (pLocal[pLocalIdx]._v[1] - pActive[pActiveIdx]._v[1]) * dx[1];
                #endif

                /* Add Hubble flow */
                const auto dvdr_Hubble = dvdr + a2_Hubble * r2;

                /* Are the particles moving towards each others ? */
                const auto omega_ij = std::min(dvdr_Hubble, 0.0);
                const auto mu_ij = fac_mu * r_inv * omega_ij; /* This is 0 or negative */

                /* Compute signal velocity */
                const auto v_sig = hydro_signal_velocity(dx, pLocal[pLocalIdx]._soundSpeed, pActive[pActiveIdx]._soundSpeed, mu_ij, const_viscosity_beta);

                /* Grab balsara switches */
                const auto balsara_i = pLocal[pLocalIdx]._balsara;
                const auto balsara_j = pActive[pActiveIdx]._balsara;

                /* Construct the full viscosity term */
                const auto rho_ij = 0.5 * (rhoi + rhoj);
                const auto visc = -0.25 * v_sig * (balsara_i + balsara_j) * mu_ij / rho_ij;

                /* Convolve with the kernel */
                const auto visc_acc_term =
                        0.5 * visc * (wi_dr * f_ij + wj_dr * f_ji) * r_inv;

                /* SPH acceleration term */
                const auto sph_acc_term =
                        (P_over_rho2_i * wi_dr + P_over_rho2_j * wj_dr) * r_inv;

                /* Adaptive softening acceleration term */
                const auto adapt_soft_acc_term = 0.0;  // doesn't exist in the minimal SPH scheme

                /* Assemble the acceleration */
                const auto acc = sph_acc_term + visc_acc_term + adapt_soft_acc_term;

                /* Use the force Luke ! */
                pLocal[pLocalIdx]._a[0] -= mj * acc * dx[0];
                pLocal[pLocalIdx]._a[1] -= mj * acc * dx[1];
                #ifdef HYDRO_DIMENSION_3D
                pLocal[pLocalIdx]._a[2] -= mj * acc * dx[2];
                #endif

                /* Get the time derivative for u. */
                const auto sph_du_term_i = P_over_rho2_i * dvdr * r_inv * wi_dr;

                /* Viscosity term */
                const auto visc_du_term = 0.5 * visc_acc_term * dvdr_Hubble;

                /* Assemble the energy equation term */
                const auto du_dt_i = sph_du_term_i + visc_du_term;
                pLocal[pLocalIdx]._uDot += du_dt_i * mj;
                pLocal[pLocalIdx]._hDot -= mj * dvdr * r_inv / rhoj * wi_dr * f_ij;
                pLocal[pLocalIdx]._v_sig_AV = std::max(pLocal[pLocalIdx]._v_sig_AV, v_sig);
            }
        }
    }
}

template<typename part>
void HydroKernel_MASKED(part *pLocal, int pLocalSize, part *pActive, int pActiveSize) {
    constexpr auto a = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme
    constexpr auto H = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme

    [[clang::soa_conversion_target(pLocal)]]
    [[clang::soa_conversion_target_size(pLocalSize)]]
    [[clang::soa_conversion_allocation_strategy(stack)]]
    #ifndef USE_VIEWS
    #include "part_full_soa.h"
    #else
    [[clang::soa_conversion_data_item("x[0]", "")]]
    [[clang::soa_conversion_data_item("x[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("x[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("h", "")]]
    [[clang::soa_conversion_data_item("mass", "")]]
    [[clang::soa_conversion_data_item("rho", "")]]
    [[clang::soa_conversion_data_item("force.pressure", "")]]
    [[clang::soa_conversion_data_item("force.f", "")]]
    [[clang::soa_conversion_data_item("v[0]", "")]]
    [[clang::soa_conversion_data_item("v[1]", "")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("v[2]", "")]]
    #endif
    [[clang::soa_conversion_data_item("force.soundspeed", "")]]
    [[clang::soa_conversion_data_item("force.balsara", "")]]
    [[clang::soa_conversion_data_item("u_dt", "u_dt")]]
    [[clang::soa_conversion_data_item("a_hydro[0]", "a_hydro[0]")]]
    [[clang::soa_conversion_data_item("a_hydro[1]", "a_hydro[1]")]]
    #ifdef HYDRO_DIMENSION_3D
    [[clang::soa_conversion_data_item("a_hydro[2]", "a_hydro[2]")]]
    #endif
    [[clang::soa_conversion_data_item("force.h_dt", "force.h_dt")]]
    [[clang::soa_conversion_data_item("force.v_sig", "force.v_sig")]]
    #endif
    for (int pLocalIdx = 0; pLocalIdx < pLocalSize; pLocalIdx++) {

        auto hi = pLocal[pLocalIdx]._smoothingLength;
        auto hig = hi * kernel_gamma;
        auto hig2 = hig * hig;
        const auto mi = pLocal[pLocalIdx]._mass;
        const auto rhoi = pLocal[pLocalIdx]._density;
        const auto pressurei = pLocal[pLocalIdx]._pressure;

        [[clang::soa_conversion_target(pActive)]]
        [[clang::soa_conversion_target_size(pActiveSize)]]
        [[clang::soa_conversion_data_movement_strategy(move_to_outer)]]
        [[clang::soa_conversion_allocation_strategy(stack)]]
        #ifndef USE_VIEWS
        #include "part_full_soa.h"
        #else
        [[clang::soa_conversion_data_item("x[0]", "")]]
        [[clang::soa_conversion_data_item("x[1]", "")]]
        #ifdef HYDRO_DIMENSION_3D
        [[clang::soa_conversion_data_item("x[2]", "")]]
        #endif
        [[clang::soa_conversion_data_item("h", "")]]
        [[clang::soa_conversion_data_item("mass", "")]]
        [[clang::soa_conversion_data_item("rho", "")]]
        [[clang::soa_conversion_data_item("force.pressure", "")]]
        [[clang::soa_conversion_data_item("force.f", "")]]
        [[clang::soa_conversion_data_item("v[0]", "")]]
        [[clang::soa_conversion_data_item("v[1]", "")]]
        #ifdef HYDRO_DIMENSION_3D
        [[clang::soa_conversion_data_item("v[2]", "")]]
        #endif
        [[clang::soa_conversion_data_item("force.soundspeed", "")]]
        [[clang::soa_conversion_data_item("force.balsara", "")]]
        #endif
        for (int pActiveIdx = 0; pActiveIdx < pActiveSize; pActiveIdx++) {
            auto r2 = 0.0;
            double dx[3] = {0};

            dx[0] = pLocal[pLocalIdx]._x[0] - pActive[pActiveIdx]._x[0];
            r2 += dx[0] * dx[0];
            dx[1] = pLocal[pLocalIdx]._x[1] - pActive[pActiveIdx]._x[1];
            r2 += dx[1] * dx[1];

            #ifdef HYDRO_DIMENSION_3D
            dx[2] = pLocal[pLocalIdx]._x[2] - pActive[pActiveIdx]._x[2];
            r2 += dx[2] * dx[2];
            #endif

            auto hjg = pActive[pActiveIdx]._smoothingLength * kernel_gamma;
            auto hjg2 = hjg * hjg;

            /* Hit or miss? */
            const bool distMask = (r2 > 0.0) & (r2 < hig2 | r2 < hjg2);

            /* Cosmological factors entering the EoMs */
            const auto fac_mu = pow_three_gamma_minus_five_over_two(a);
            const auto a2_Hubble = a * a * H;

            /* Get r and 1/r. */
            const auto r = std::sqrtf(r2);
            const auto r_inv = r ? 1.0 / r : 0.0;

            /* Recover some data */
            const auto mj = pActive[pActiveIdx]._mass;
            const auto rhoj = pActive[pActiveIdx]._density;
            const auto pressurej = pActive[pActiveIdx]._pressure;

            /* Get the kernel for hi. */
            const auto hi_inv = 1.0 / hi;
            const auto hid_inv = pow_dimension_plus_one(hi_inv); /* 1/h^(d+1) */
            const auto xi = r * hi_inv;
            double wi, wi_dx;
            kernel_deval(xi, &wi, &wi_dx);
            const double wi_dr = hid_inv * wi_dx;

            /* Get the kernel for hj. */
            const auto hj_inv = 1.0 / pActive[pActiveIdx]._smoothingLength;
            const auto hjd_inv = pow_dimension_plus_one(hj_inv); /* 1/h^(d+1) */
            const auto xj = r * hj_inv;
            double wj, wj_dx;
            kernel_deval(xj, &wj, &wj_dx);
            const double wj_dr = hjd_inv * wj_dx;

            /* Variable smoothing length term */
            const auto f_ij = 1.0 - pLocal[pLocalIdx]._f / mj;
            const auto f_ji = 1.0 - pActive[pActiveIdx]._f / mi;

            /* Compute gradient terms */
            const auto P_over_rho2_i = pressurei / (rhoi * rhoi) * f_ij;
            const auto P_over_rho2_j = pressurej / (rhoj * rhoj) * f_ji;


            /* Compute dv dot r. */
            #ifdef HYDRO_DIMENSION_3D
            const auto dvdr = (pLocal[pLocalIdx]._v[0] - pActive[pActiveIdx]._v[0]) * dx[0] +
                               (pLocal[pLocalIdx]._v[1] - pActive[pActiveIdx]._v[1]) * dx[1] +
                               (pLocal[pLocalIdx]._v[2] - pActive[pActiveIdx]._v[2]) * dx[2];
            #else
            const auto dvdr = (pLocal[pLocalIdx]._v[0] - pActive[pActiveIdx]._v[0]) * dx[0] +
                               (pLocal[pLocalIdx]._v[1] - pActive[pActiveIdx]._v[1]) * dx[1];
            #endif

            /* Add Hubble flow */
            const auto dvdr_Hubble = dvdr + a2_Hubble * r2;

            /* Are the particles moving towards each others ? */
            const auto omega_ij = std::min(dvdr_Hubble, 0.0);
            const auto mu_ij = fac_mu * r_inv * omega_ij; /* This is 0 or negative */

            /* Compute signal velocity */
            const auto v_sig = hydro_signal_velocity(dx, pLocal[pLocalIdx]._soundSpeed, pActive[pActiveIdx]._soundSpeed, mu_ij, const_viscosity_beta);

            /* Grab balsara switches */
            const auto balsara_i = pLocal[pLocalIdx]._balsara;
            const auto balsara_j = pActive[pActiveIdx]._balsara;

            /* Construct the full viscosity term */
            const auto rho_ij = 0.5 * (rhoi + rhoj);
            const auto visc = -0.25 * v_sig * (balsara_i + balsara_j) * mu_ij / rho_ij;

            /* Convolve with the kernel */
            const auto visc_acc_term =
                    0.5 * visc * (wi_dr * f_ij + wj_dr * f_ji) * r_inv;

            /* SPH acceleration term */
            const auto sph_acc_term =
                    (P_over_rho2_i * wi_dr + P_over_rho2_j * wj_dr) * r_inv;

            /* Adaptive softening acceleration term */
            const auto adapt_soft_acc_term = 0.0;  // doesn't exist in the minimal SPH scheme

            /* Assemble the acceleration */
            const auto acc = sph_acc_term + visc_acc_term + adapt_soft_acc_term;

            /* Use the force Luke ! */
            pLocal[pLocalIdx]._a[0] -= mj * acc * dx[0] * distMask;
            pLocal[pLocalIdx]._a[1] -= mj * acc * dx[1] * distMask;
            #ifdef HYDRO_DIMENSION_3D
            pLocal[pLocalIdx]._a[2] -= mj * acc * dx[2] * distMask;
            #endif

            /* Get the time derivative for u. */
            const auto sph_du_term_i = P_over_rho2_i * dvdr * r_inv * wi_dr;

            /* Viscosity term */
            const auto visc_du_term = 0.5 * visc_acc_term * dvdr_Hubble;

            /* Assemble the energy equation term */
            const auto du_dt_i = sph_du_term_i + visc_du_term;
            pLocal[pLocalIdx]._uDot += du_dt_i * mj * distMask;
            pLocal[pLocalIdx]._hDot -= mj * dvdr * r_inv / rhoj * wi_dr * f_ij * distMask;
            pLocal[pLocalIdx]._v_sig_AV = std::max(pLocal[pLocalIdx]._v_sig_AV, (double) v_sig * distMask);
        }
    }
}
