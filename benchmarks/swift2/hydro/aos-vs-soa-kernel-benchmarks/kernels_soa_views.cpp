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
    struct particles__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);

} particles__SoA__instance;

particles__SoA__instance._0 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._1 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._2 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._3 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._4 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._5 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._6 = (double *) alloca( sizeof(double) * size);

for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles__SoA__instance._0[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[0];
particles__SoA__instance._1[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[0];
particles__SoA__instance._2[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[1];
particles__SoA__instance._3[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[1];
particles__SoA__instance._4[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._uDot;
particles__SoA__instance._5[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._u_full;
particles__SoA__instance._6[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._density;
}

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
    [[clang::soa_conversion_data_item("xpart.u_full", "xpart.u_full")]]
    [[clang::soa_conversion_data_item("rho", "")]]
    #endif
    for (int pIdx = 0; pIdx < size; pIdx++) {
        particles__SoA__instance._0[pIdx] += particles__SoA__instance._1[pIdx] * DT_KICK_HYDRO;
        particles__SoA__instance._2[pIdx] += particles__SoA__instance._3[pIdx] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v_full[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_kick_extra
        const double delta_u = particles__SoA__instance._4[pIdx] * DT_KICK_THERM;
        particles__SoA__instance._5[pIdx] = std::max(particles__SoA__instance._5[pIdx] + delta_u, 0.5 * particles__SoA__instance._5[pIdx]);
        const double floor_A = 0.0; // does not exist in the minimal SPH scheme
        const double floor_u = gas_internal_energy_from_entropy(particles__SoA__instance._6[pIdx], floor_A);
        const double min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        const double energy_min = std::max(min_u, floor_u);

        if (particles__SoA__instance._5[pIdx] < energy_min) {
            particles__SoA__instance._5[pIdx] = energy_min;
            particles__SoA__instance._4[pIdx] = 0.0;
        }
    }
for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles[particles__SoA__instance__iter]._v_full[0] = particles__SoA__instance._0[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v_full[1] = particles__SoA__instance._2[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._uDot = particles__SoA__instance._4[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._u_full = particles__SoA__instance._5[particles__SoA__instance__iter];
}


}

template<typename part>
void Kick2Kernel(part *particles, int size) {
    struct particles__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);
    double * (_11);
    double * (_12);

} particles__SoA__instance;

particles__SoA__instance._0 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._1 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._2 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._3 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._4 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._5 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._6 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._7 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._8 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._9 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._10 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._11 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._12 = (double *) alloca( sizeof(double) * size);

for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles__SoA__instance._0[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[0];
particles__SoA__instance._1[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[0];
particles__SoA__instance._2[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v[0];
particles__SoA__instance._3[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[1];
particles__SoA__instance._4[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[1];
particles__SoA__instance._5[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v[1];
particles__SoA__instance._6[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._uDot;
particles__SoA__instance._7[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._u_full;
particles__SoA__instance._8[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._density;
particles__SoA__instance._9[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._u;
particles__SoA__instance._10[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._pressure;
particles__SoA__instance._11[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._soundSpeed;
particles__SoA__instance._12[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_sig_AV;
}

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
    [[clang::soa_conversion_data_item("xpart.u_full", "xpart.u_full")]]
    [[clang::soa_conversion_data_item("rho", "")]]
    [[clang::soa_conversion_data_item("u", "u")]]
    [[clang::soa_conversion_data_item("force.pressure", "force.pressure")]]
    [[clang::soa_conversion_data_item("force.soundspeed", "force.soundspeed")]]
    [[clang::soa_conversion_data_item("force.v_sig", "force.v_sig")]]
    #endif
    for (int pIdx = 0; pIdx < size; pIdx++) {
        particles[pIdx]._hDot *= particles[pIdx]._smoothingLength * hydro_dimension_inv;

        particles__SoA__instance._0[pIdx] += particles__SoA__instance._1[pIdx] * DT_KICK_HYDRO;
        particles__SoA__instance._3[pIdx] += particles__SoA__instance._4[pIdx] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v_full[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_kick_extra
        const double delta_u = particles__SoA__instance._6[pIdx] * DT_KICK_THERM;
        particles__SoA__instance._7[pIdx] = std::max(particles__SoA__instance._7[pIdx] + delta_u, 0.5 * particles__SoA__instance._7[pIdx]);
        const double floor_A = 0.0; // does not exist in the minimal SPH scheme
        const double floor_u = gas_internal_energy_from_entropy(particles__SoA__instance._8[pIdx], floor_A);
        const double min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        const double energy_min = std::max(min_u, floor_u);

        if (particles__SoA__instance._7[pIdx] < energy_min) {
            particles__SoA__instance._7[pIdx] = energy_min;
            particles__SoA__instance._6[pIdx] = 0.0;
        }

        // hydro_reset_predicted_values
        particles__SoA__instance._2[pIdx] = particles__SoA__instance._0[pIdx];
        particles__SoA__instance._5[pIdx] = particles__SoA__instance._3[pIdx];

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v[2] = particles[pIdx]._v_full[2];
        #endif

        /* Re-set the entropy */
        particles__SoA__instance._9[pIdx] = particles__SoA__instance._7[pIdx];

        /* Re-compute the pressure */
        const double pressure = gas_pressure_from_internal_energy(particles__SoA__instance._8[pIdx], particles__SoA__instance._9[pIdx]);

        /* Compute the new sound speed */
        const double soundspeed = gas_soundspeed_from_pressure(particles__SoA__instance._8[pIdx], pressure);

        /* Update variables */
        particles__SoA__instance._10[pIdx] = pressure;
        particles__SoA__instance._11[pIdx] = soundspeed;

        particles__SoA__instance._12[pIdx] = std::max(particles__SoA__instance._12[pIdx], 2.0 * soundspeed);
    }
for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles[particles__SoA__instance__iter]._v_full[0] = particles__SoA__instance._0[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v[0] = particles__SoA__instance._2[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v_full[1] = particles__SoA__instance._3[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v[1] = particles__SoA__instance._5[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._uDot = particles__SoA__instance._6[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._u_full = particles__SoA__instance._7[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._u = particles__SoA__instance._9[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._pressure = particles__SoA__instance._10[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._soundSpeed = particles__SoA__instance._11[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v_sig_AV = particles__SoA__instance._12[particles__SoA__instance__iter];
}


}

template<typename part>
void DriftKernel(part *particles, int size) {
    struct particles__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);
    double * (_11);
    double * (_12);
    double * (_13);
    double * (_14);
    double * (_15);
    double * (_16);
    double * (_17);
    double * (_18);
    double * (_19);

} particles__SoA__instance;

particles__SoA__instance._0 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._1 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._2 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._3 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._4 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._5 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._6 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._7 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._8 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._9 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._10 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._11 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._12 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._13 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._14 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._15 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._16 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._17 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._18 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._19 = (double *) alloca( sizeof(double) * size);

for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles__SoA__instance._0[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._x[0];
particles__SoA__instance._1[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[0];
particles__SoA__instance._2[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._x[1];
particles__SoA__instance._3[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[1];
particles__SoA__instance._4[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v[0];
particles__SoA__instance._5[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[0];
particles__SoA__instance._6[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v[1];
particles__SoA__instance._7[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[2];
particles__SoA__instance._8[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._u;
particles__SoA__instance._9[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._uDot;
particles__SoA__instance._10[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._smoothingLength;
particles__SoA__instance._11[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._hDot;
particles__SoA__instance._12[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._density;
particles__SoA__instance._13[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._pressure;
particles__SoA__instance._14[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._soundSpeed;
particles__SoA__instance._15[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_sig_AV;
particles__SoA__instance._16[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter].xpart.x_diff[0];
particles__SoA__instance._17[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter].xpart.x_diff_sort[0];
particles__SoA__instance._18[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter].xpart.x_diff[1];
particles__SoA__instance._19[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter].xpart.x_diff_sort[1];
}

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
        particles__SoA__instance._0[pIdx] += particles__SoA__instance._1[pIdx] * DT_DRIFT;
        particles__SoA__instance._2[pIdx] += particles__SoA__instance._3[pIdx] * DT_DRIFT;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._x[2] += particles[pIdx]._v_full[2] * DT_DRIFT;
        #endif

        particles__SoA__instance._4[pIdx] += particles__SoA__instance._5[pIdx] * DT_KICK_HYDRO;
        particles__SoA__instance._6[pIdx] += particles[pIdx]._a[1] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_predict_extra
        particles__SoA__instance._8[pIdx] += particles__SoA__instance._9[pIdx] * DT_THERM;

        const double h_inv = 1.0 / particles__SoA__instance._10[pIdx];

        const double w1 = particles__SoA__instance._11[pIdx] * h_inv * DT_DRIFT;
        if (std::abs(w1) < 0.2) {
            particles__SoA__instance._10[pIdx] *= approx_expf(w1); /* 4th order expansion of exp(w) */
        } else {
            particles__SoA__instance._10[pIdx] *= expf(w1);
        }

        const double w2 = -hydro_dimension * w1;
        if (std::abs(w2) < 0.2) {
            particles__SoA__instance._12[pIdx] *= approx_expf(w2); /* 4th order expansion of exp(w) */
        } else {
            particles__SoA__instance._12[pIdx] *= expf(w2);
        }

        const double floor_A = 0.0; // does not exist in the minimal SPH scheme
        const double floor_u = gas_internal_energy_from_entropy(particles__SoA__instance._12[pIdx], floor_A);
        const double min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        particles__SoA__instance._8[pIdx] = std::max(particles__SoA__instance._8[pIdx], floor_u);
        particles__SoA__instance._8[pIdx] = std::max(particles__SoA__instance._8[pIdx], min_u);

        const double pressure = gas_pressure_from_internal_energy(particles__SoA__instance._12[pIdx], particles__SoA__instance._8[pIdx]);
        const double soundspeed = gas_soundspeed_from_pressure(particles__SoA__instance._12[pIdx], pressure);
        particles__SoA__instance._13[pIdx] = pressure;
        particles__SoA__instance._14[pIdx] = soundspeed;
        particles__SoA__instance._15[pIdx] = std::max(particles__SoA__instance._15[pIdx], 2.0 * soundspeed);

        // compute offsets since last cell construction
        const double dx0 = particles__SoA__instance._1[pIdx] * DT_DRIFT;
        particles__SoA__instance._16[pIdx] -= dx0;
        particles__SoA__instance._17[pIdx] -= dx0;

        const double dx1 = particles__SoA__instance._3[pIdx] * DT_DRIFT;
        particles__SoA__instance._18[pIdx] -= dx1;
        particles__SoA__instance._19[pIdx] -= dx1;

        #ifdef HYDRO_DIMENSION_3D
        const double dx2 = particles[pIdx]._v_full[2] * DT_DRIFT;
        particles[pIdx].xpart.x_diff[2] -= dx2;
        particles[pIdx].xpart.x_diff_sort[2] -= dx2;
        #endif
    }
for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles[particles__SoA__instance__iter]._x[0] = particles__SoA__instance._0[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._x[1] = particles__SoA__instance._2[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v[0] = particles__SoA__instance._4[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v[1] = particles__SoA__instance._6[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._u = particles__SoA__instance._8[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._smoothingLength = particles__SoA__instance._10[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._density = particles__SoA__instance._12[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._pressure = particles__SoA__instance._13[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._soundSpeed = particles__SoA__instance._14[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v_sig_AV = particles__SoA__instance._15[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter].xpart.x_diff[0] = particles__SoA__instance._16[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter].xpart.x_diff_sort[0] = particles__SoA__instance._17[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter].xpart.x_diff[1] = particles__SoA__instance._18[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter].xpart.x_diff_sort[1] = particles__SoA__instance._19[particles__SoA__instance__iter];
}


}

template<typename part>
void DriftKernel_MASKED(part *particles, int size) {
    struct particles__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);
    double * (_11);
    double * (_12);
    double * (_13);
    double * (_14);
    double * (_15);
    double * (_16);
    double * (_17);
    double * (_18);
    double * (_19);

} particles__SoA__instance;

particles__SoA__instance._0 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._1 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._2 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._3 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._4 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._5 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._6 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._7 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._8 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._9 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._10 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._11 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._12 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._13 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._14 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._15 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._16 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._17 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._18 = (double *) alloca( sizeof(double) * size);
particles__SoA__instance._19 = (double *) alloca( sizeof(double) * size);

for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles__SoA__instance._0[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._x[0];
particles__SoA__instance._1[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[0];
particles__SoA__instance._2[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._x[1];
particles__SoA__instance._3[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_full[1];
particles__SoA__instance._4[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v[0];
particles__SoA__instance._5[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[0];
particles__SoA__instance._6[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v[1];
particles__SoA__instance._7[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._a[2];
particles__SoA__instance._8[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._u;
particles__SoA__instance._9[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._uDot;
particles__SoA__instance._10[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._smoothingLength;
particles__SoA__instance._11[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._hDot;
particles__SoA__instance._12[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._density;
particles__SoA__instance._13[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._pressure;
particles__SoA__instance._14[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._soundSpeed;
particles__SoA__instance._15[particles__SoA__instance__iter] = particles[particles__SoA__instance__iter]._v_sig_AV;
}

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
        particles__SoA__instance._0[pIdx] += particles__SoA__instance._1[pIdx] * DT_DRIFT;
        particles__SoA__instance._2[pIdx] += particles__SoA__instance._3[pIdx] * DT_DRIFT;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._x[2] += particles[pIdx]._v_full[2] * DT_DRIFT;
        #endif

        particles__SoA__instance._4[pIdx] += particles__SoA__instance._5[pIdx] * DT_KICK_HYDRO;
        particles__SoA__instance._6[pIdx] += particles[pIdx]._a[1] * DT_KICK_HYDRO;

        #ifdef HYDRO_DIMENSION_3D
        particles[pIdx]._v[2] += particles[pIdx]._a[2] * DT_KICK_HYDRO;
        #endif

        // hydro_predict_extra
        particles__SoA__instance._8[pIdx] += particles__SoA__instance._9[pIdx] * DT_THERM;

        const double h_inv = 1.0 / particles__SoA__instance._10[pIdx];

        const double w1 = particles__SoA__instance._11[pIdx] * h_inv * DT_DRIFT;
//        if (std::abs(w1) < 0.2) {
//            particles[pIdx]._smoothingLength *= approx_expf(w1); /* 4th order expansion of exp(w) */
//        } else {
//            particles[pIdx]._smoothingLength *= expf(w1);
//        }

        const bool w1_MASK = std::abs(w1) < 0.2;
        particles__SoA__instance._10[pIdx] *= (w1_MASK * approx_expf(w1) + !w1_MASK * expf(w1));

        const double w2 = -hydro_dimension * w1;
//        if (std::abs(w2) < 0.2) {
//            particles[pIdx]._density *= approx_expf(w2); /* 4th order expansion of exp(w) */
//        } else {
//            particles[pIdx]._density *= expf(w2);
//        }

        const bool w2_MASK = std::abs(w2) < 0.2;
        particles__SoA__instance._12[pIdx] *= (w2_MASK * approx_expf(w2) + !w2_MASK * expf(w2));

        const double floor_A = 0.0; // does not exist in the minimal SPH scheme
        const double floor_u = gas_internal_energy_from_entropy(particles__SoA__instance._12[pIdx], floor_A);
        const double min_u = HYDRO_PROPS.minimal_internal_energy / COSMO.a_factor_internal_energy;
        particles__SoA__instance._8[pIdx] = std::max(particles__SoA__instance._8[pIdx], floor_u);
        particles__SoA__instance._8[pIdx] = std::max(particles__SoA__instance._8[pIdx], min_u);

        const double pressure = gas_pressure_from_internal_energy(particles__SoA__instance._12[pIdx], particles__SoA__instance._8[pIdx]);
        const double soundspeed = gas_soundspeed_from_pressure(particles__SoA__instance._12[pIdx], pressure);
        particles__SoA__instance._13[pIdx] = pressure;
        particles__SoA__instance._14[pIdx] = soundspeed;
        particles__SoA__instance._15[pIdx] = std::max(particles__SoA__instance._15[pIdx], 2.0 * soundspeed);

        // compute offsets since last cell construction
        const double dx0 = particles__SoA__instance._1[pIdx] * DT_DRIFT;
        particles__SoA__instance._16[pIdx] -= dx0;
        particles__SoA__instance._17[pIdx] -= dx0;

        const double dx1 = particles__SoA__instance._3[pIdx] * DT_DRIFT;
        particles__SoA__instance._18[pIdx] -= dx1;
        particles__SoA__instance._19[pIdx] -= dx1;

        #ifdef HYDRO_DIMENSION_3D
        const double dx2 = particles[pIdx]._v_full[2] * DT_DRIFT;
        particles[pIdx].xpart.x_diff[2] -= dx2;
        particles[pIdx].xpart.x_diff_sort[2] -= dx2;
        #endif
    }
for (int particles__SoA__instance__iter = 0; particles__SoA__instance__iter < size; particles__SoA__instance__iter++) {
particles[particles__SoA__instance__iter]._x[0] = particles__SoA__instance._0[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._x[1] = particles__SoA__instance._2[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v[0] = particles__SoA__instance._4[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v[1] = particles__SoA__instance._6[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._u = particles__SoA__instance._8[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._smoothingLength = particles__SoA__instance._10[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._density = particles__SoA__instance._12[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._pressure = particles__SoA__instance._13[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._soundSpeed = particles__SoA__instance._14[particles__SoA__instance__iter];
particles[particles__SoA__instance__iter]._v_sig_AV = particles__SoA__instance._15[particles__SoA__instance__iter];
}


}

template<typename part>
void HydroKernel(part *pLocal, int pLocalSize, part *pActive, int pActiveSize) {
    constexpr double a = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme
    constexpr double H = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme

    struct pActive__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);

} pActive__SoA__instance;

pActive__SoA__instance._0 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._1 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._2 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._3 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._4 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._5 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._6 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._7 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._8 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._9 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._10 = (double *) alloca( sizeof(double) * pActiveSize);

for (int pActive__SoA__instance__iter = 0; pActive__SoA__instance__iter < pActiveSize; pActive__SoA__instance__iter++) {
pActive__SoA__instance._0[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._x[0];
pActive__SoA__instance._1[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._x[1];
pActive__SoA__instance._2[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._smoothingLength;
pActive__SoA__instance._3[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._mass;
pActive__SoA__instance._4[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._density;
pActive__SoA__instance._5[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._pressure;
pActive__SoA__instance._6[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._f;
pActive__SoA__instance._7[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._v[0];
pActive__SoA__instance._8[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._v[1];
pActive__SoA__instance._9[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._soundSpeed;
pActive__SoA__instance._10[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._balsara;
}

struct pLocal__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);
    double * (_11);
    double * (_12);
    double * (_13);
    double * (_14);
    double * (_15);

} pLocal__SoA__instance;

pLocal__SoA__instance._0 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._1 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._2 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._3 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._4 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._5 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._6 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._7 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._8 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._9 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._10 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._11 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._12 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._13 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._14 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._15 = (double *) alloca( sizeof(double) * pLocalSize);

for (int pLocal__SoA__instance__iter = 0; pLocal__SoA__instance__iter < pLocalSize; pLocal__SoA__instance__iter++) {
pLocal__SoA__instance._0[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._x[0];
pLocal__SoA__instance._1[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._x[1];
pLocal__SoA__instance._2[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._smoothingLength;
pLocal__SoA__instance._3[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._mass;
pLocal__SoA__instance._4[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._density;
pLocal__SoA__instance._5[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._pressure;
pLocal__SoA__instance._6[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._f;
pLocal__SoA__instance._7[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._v[0];
pLocal__SoA__instance._8[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._v[1];
pLocal__SoA__instance._9[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._soundSpeed;
pLocal__SoA__instance._10[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._balsara;
pLocal__SoA__instance._11[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._uDot;
pLocal__SoA__instance._12[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._a[0];
pLocal__SoA__instance._13[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._a[1];
pLocal__SoA__instance._14[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._hDot;
pLocal__SoA__instance._15[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._v_sig_AV;
}

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
    [[clang::soa_conversion_data_item("force.0", "")]]
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

        auto hi = pLocal__SoA__instance._2[pLocalIdx];
        auto hig = hi * kernel_gamma;
        auto hig2 = hig * hig;
        const double mi = pLocal__SoA__instance._3[pLocalIdx];
        const double rhoi = pLocal__SoA__instance._4[pLocalIdx];
        const double pressurei = pLocal__SoA__instance._5[pLocalIdx];

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
        [[clang::soa_conversion_data_item("force.0", "")]]
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

            dx[0] = pLocal__SoA__instance._0[pLocalIdx] - pActive__SoA__instance._0[pActiveIdx];
            r2 += dx[0] * dx[0];
            dx[1] = pLocal__SoA__instance._1[pLocalIdx] - pActive__SoA__instance._1[pActiveIdx];
            r2 += dx[1] * dx[1];

            #ifdef HYDRO_DIMENSION_3D
            dx[2] = pLocal[pLocalIdx]._x[2] - pActive[pActiveIdx]._x[2];
            r2 += dx[2] * dx[2];
            #endif

            auto hjg = pActive__SoA__instance._2[pActiveIdx] * kernel_gamma;
            auto hjg2 = hjg * hjg;

            /* Hit or miss? */
            if ((r2 > 0.0) & (r2 < hig2 | r2 < hjg2)) {

                /* Cosmological factors entering the EoMs */
                const double fac_mu = pow_three_gamma_minus_five_over_two(a);
                const double a2_Hubble = a * a * H;

                /* Get r and 1/r. */
                const double r = sqrtf(r2);
                const double r_inv = r ? 1.0 / r : 0.0;

                /* Recover some data */
                const double mj = pActive__SoA__instance._3[pActiveIdx];
                const double rhoj = pActive__SoA__instance._4[pActiveIdx];
                const double pressurej = pActive__SoA__instance._5[pActiveIdx];

                /* Get the kernel for hi. */
                const double hi_inv = 1.0 / hi;
                const double hid_inv = pow_dimension_plus_one(hi_inv); /* 1/h^(d+1) */
                const double xi = r * hi_inv;
                double wi, wi_dx;
                kernel_deval(xi, &wi, &wi_dx);
                const double wi_dr = hid_inv * wi_dx;

                /* Get the kernel for hj. */
                const double hj_inv = 1.0 / pActive__SoA__instance._2[pActiveIdx];
                const double hjd_inv = pow_dimension_plus_one(hj_inv); /* 1/h^(d+1) */
                const double xj = r * hj_inv;
                double wj, wj_dx;
                kernel_deval(xj, &wj, &wj_dx);
                const double wj_dr = hjd_inv * wj_dx;

                /* Variable smoothing length term */
                const double f_ij = 1.0 - pLocal__SoA__instance._6[pLocalIdx] / mj;
                const double f_ji = 1.0 - pActive__SoA__instance._6[pActiveIdx] / mi;

                /* Compute gradient terms */
                const double P_over_rho2_i = pressurei / (rhoi * rhoi) * f_ij;
                const double P_over_rho2_j = pressurej / (rhoj * rhoj) * f_ji;


                /* Compute dv dot r. */
                #ifdef HYDRO_DIMENSION_3D
                const double dvdr = (pLocal[pLocalIdx]._v[0] - pActive[pActiveIdx]._v[0]) * dx[0] +
                                   (pLocal[pLocalIdx]._v[1] - pActive[pActiveIdx]._v[1]) * dx[1] +
                                   (pLocal[pLocalIdx]._v[2] - pActive[pActiveIdx]._v[2]) * dx[2];
                #else
                const double dvdr = (pLocal__SoA__instance._7[pLocalIdx] - pActive__SoA__instance._7[pActiveIdx]) * dx[0] +
                                   (pLocal__SoA__instance._8[pLocalIdx] - pActive__SoA__instance._8[pActiveIdx]) * dx[1];
                #endif

                /* Add Hubble flow */
                const double dvdr_Hubble = dvdr + a2_Hubble * r2;

                /* Are the particles moving towards each others ? */
                const double omega_ij = std::min(dvdr_Hubble, 0.0);
                const double mu_ij = fac_mu * r_inv * omega_ij; /* This is 0 or negative */

                /* Compute signal velocity */
                const double v_sig = hydro_signal_velocity(dx, pLocal__SoA__instance._9[pLocalIdx], pActive__SoA__instance._9[pActiveIdx], mu_ij, const_viscosity_beta);

                /* Grab balsara switches */
                const double balsara_i = pLocal__SoA__instance._10[pLocalIdx];
                const double balsara_j = pActive__SoA__instance._10[pActiveIdx];

                /* Construct the full viscosity term */
                const double rho_ij = 0.5 * (rhoi + rhoj);
                const double visc = -0.25 * v_sig * (balsara_i + balsara_j) * mu_ij / rho_ij;

                /* Convolve with the kernel */
                const double visc_acc_term =
                        0.5 * visc * (wi_dr * f_ij + wj_dr * f_ji) * r_inv;

                /* SPH acceleration term */
                const double sph_acc_term =
                        (P_over_rho2_i * wi_dr + P_over_rho2_j * wj_dr) * r_inv;

                /* Adaptive softening acceleration term */
                const double adapt_soft_acc_term = 0.0;  // doesn't exist in the minimal SPH scheme

                /* Assemble the acceleration */
                const double acc = sph_acc_term + visc_acc_term + adapt_soft_acc_term;

                /* Use the force Luke ! */
                pLocal__SoA__instance._12[pLocalIdx] -= mj * acc * dx[0];
                pLocal__SoA__instance._13[pLocalIdx] -= mj * acc * dx[1];
                #ifdef HYDRO_DIMENSION_3D
                pLocal[pLocalIdx]._a[2] -= mj * acc * dx[2];
                #endif

                /* Get the time derivative for u. */
                const double sph_du_term_i = P_over_rho2_i * dvdr * r_inv * wi_dr;

                /* Viscosity term */
                const double visc_du_term = 0.5 * visc_acc_term * dvdr_Hubble;

                /* Assemble the energy equation term */
                const double du_dt_i = sph_du_term_i + visc_du_term;
                pLocal__SoA__instance._11[pLocalIdx] += du_dt_i * mj;
                pLocal__SoA__instance._14[pLocalIdx] -= mj * dvdr * r_inv / rhoj * wi_dr * f_ij;
                pLocal__SoA__instance._15[pLocalIdx] = std::max(pLocal__SoA__instance._15[pLocalIdx], v_sig);
            }
        }
    }
for (int pLocal__SoA__instance__iter = 0; pLocal__SoA__instance__iter < pLocalSize; pLocal__SoA__instance__iter++) {
pLocal[pLocal__SoA__instance__iter]._uDot = pLocal__SoA__instance._11[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._a[0] = pLocal__SoA__instance._12[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._a[1] = pLocal__SoA__instance._13[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._hDot = pLocal__SoA__instance._14[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._v_sig_AV = pLocal__SoA__instance._15[pLocal__SoA__instance__iter];
}


/** SOA epilogue loop skipped for target pActive: no items to write back */


}

template<typename part>
void HydroKernel_MASKED(part *pLocal, int pLocalSize, part *pActive, int pActiveSize) {
    constexpr double a = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme
    constexpr double H = 1.0; // SWIFT parameter, ignored by the Minimal SPH scheme

    struct pActive__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);

} pActive__SoA__instance;

pActive__SoA__instance._0 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._1 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._2 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._3 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._4 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._5 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._6 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._7 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._8 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._9 = (double *) alloca( sizeof(double) * pActiveSize);
pActive__SoA__instance._10 = (double *) alloca( sizeof(double) * pActiveSize);

for (int pActive__SoA__instance__iter = 0; pActive__SoA__instance__iter < pActiveSize; pActive__SoA__instance__iter++) {
pActive__SoA__instance._0[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._x[0];
pActive__SoA__instance._1[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._x[1];
pActive__SoA__instance._2[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._smoothingLength;
pActive__SoA__instance._3[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._mass;
pActive__SoA__instance._4[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._density;
pActive__SoA__instance._5[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._pressure;
pActive__SoA__instance._6[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._f;
pActive__SoA__instance._7[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._v[0];
pActive__SoA__instance._8[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._v[1];
pActive__SoA__instance._9[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._soundSpeed;
pActive__SoA__instance._10[pActive__SoA__instance__iter] = pActive[pActive__SoA__instance__iter]._balsara;
}

struct pLocal__SoA {
    double * (_0);
    double * (_1);
    double * (_2);
    double * (_3);
    double * (_4);
    double * (_5);
    double * (_6);
    double * (_7);
    double * (_8);
    double * (_9);
    double * (_10);
    double * (_11);
    double * (_12);
    double * (_13);
    double * (_14);
    double * (_15);

} pLocal__SoA__instance;

pLocal__SoA__instance._0 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._1 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._2 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._3 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._4 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._5 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._6 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._7 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._8 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._9 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._10 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._11 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._12 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._13 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._14 = (double *) alloca( sizeof(double) * pLocalSize);
pLocal__SoA__instance._15 = (double *) alloca( sizeof(double) * pLocalSize);

for (int pLocal__SoA__instance__iter = 0; pLocal__SoA__instance__iter < pLocalSize; pLocal__SoA__instance__iter++) {
pLocal__SoA__instance._0[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._x[0];
pLocal__SoA__instance._1[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._x[1];
pLocal__SoA__instance._2[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._smoothingLength;
pLocal__SoA__instance._3[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._mass;
pLocal__SoA__instance._4[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._density;
pLocal__SoA__instance._5[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._pressure;
pLocal__SoA__instance._6[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._f;
pLocal__SoA__instance._7[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._v[0];
pLocal__SoA__instance._8[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._v[1];
pLocal__SoA__instance._9[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._soundSpeed;
pLocal__SoA__instance._10[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._balsara;
pLocal__SoA__instance._11[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._uDot;
pLocal__SoA__instance._12[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._a[0];
pLocal__SoA__instance._13[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._a[1];
pLocal__SoA__instance._14[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._hDot;
pLocal__SoA__instance._15[pLocal__SoA__instance__iter] = pLocal[pLocal__SoA__instance__iter]._v_sig_AV;
}

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
    [[clang::soa_conversion_data_item("force.0", "")]]
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

        auto hi = pLocal__SoA__instance._2[pLocalIdx];
        auto hig = hi * kernel_gamma;
        auto hig2 = hig * hig;
        const double mi = pLocal__SoA__instance._3[pLocalIdx];
        const double rhoi = pLocal__SoA__instance._4[pLocalIdx];
        const double pressurei = pLocal__SoA__instance._5[pLocalIdx];

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
        [[clang::soa_conversion_data_item("force.0", "")]]
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

            dx[0] = pLocal__SoA__instance._0[pLocalIdx] - pActive__SoA__instance._0[pActiveIdx];
            r2 += dx[0] * dx[0];
            dx[1] = pLocal__SoA__instance._1[pLocalIdx] - pActive__SoA__instance._1[pActiveIdx];
            r2 += dx[1] * dx[1];

            #ifdef HYDRO_DIMENSION_3D
            dx[2] = pLocal[pLocalIdx]._x[2] - pActive[pActiveIdx]._x[2];
            r2 += dx[2] * dx[2];
            #endif

            auto hjg = pActive__SoA__instance._2[pActiveIdx] * kernel_gamma;
            auto hjg2 = hjg * hjg;

            /* Hit or miss? */
            const bool distMask = (r2 > 0.0) & (r2 < hig2 | r2 < hjg2);

            /* Cosmological factors entering the EoMs */
            const double fac_mu = pow_three_gamma_minus_five_over_two(a);
            const double a2_Hubble = a * a * H;

            /* Get r and 1/r. */
            const double r = sqrtf(r2);
            const double r_inv = r ? 1.0 / r : 0.0;

            /* Recover some data */
            const double mj = pActive__SoA__instance._3[pActiveIdx];
            const double rhoj = pActive__SoA__instance._4[pActiveIdx];
            const double pressurej = pActive__SoA__instance._5[pActiveIdx];

            /* Get the kernel for hi. */
            const double hi_inv = 1.0 / hi;
            const double hid_inv = pow_dimension_plus_one(hi_inv); /* 1/h^(d+1) */
            const double xi = r * hi_inv;
            double wi, wi_dx;
            kernel_deval(xi, &wi, &wi_dx);
            const double wi_dr = hid_inv * wi_dx;

            /* Get the kernel for hj. */
            const double hj_inv = 1.0 / pActive__SoA__instance._2[pActiveIdx];
            const double hjd_inv = pow_dimension_plus_one(hj_inv); /* 1/h^(d+1) */
            const double xj = r * hj_inv;
            double wj, wj_dx;
            kernel_deval(xj, &wj, &wj_dx);
            const double wj_dr = hjd_inv * wj_dx;

            /* Variable smoothing length term */
            const double f_ij = 1.0 - pLocal__SoA__instance._6[pLocalIdx] / mj;
            const double f_ji = 1.0 - pActive__SoA__instance._6[pActiveIdx] / mi;

            /* Compute gradient terms */
            const double P_over_rho2_i = pressurei / (rhoi * rhoi) * f_ij;
            const double P_over_rho2_j = pressurej / (rhoj * rhoj) * f_ji;


            /* Compute dv dot r. */
            #ifdef HYDRO_DIMENSION_3D
            const double dvdr = (pLocal[pLocalIdx]._v[0] - pActive[pActiveIdx]._v[0]) * dx[0] +
                               (pLocal[pLocalIdx]._v[1] - pActive[pActiveIdx]._v[1]) * dx[1] +
                               (pLocal[pLocalIdx]._v[2] - pActive[pActiveIdx]._v[2]) * dx[2];
            #else
            const double dvdr = (pLocal__SoA__instance._7[pLocalIdx] - pActive__SoA__instance._7[pActiveIdx]) * dx[0] +
                               (pLocal__SoA__instance._8[pLocalIdx] - pActive__SoA__instance._8[pActiveIdx]) * dx[1];
            #endif

            /* Add Hubble flow */
            const double dvdr_Hubble = dvdr + a2_Hubble * r2;

            /* Are the particles moving towards each others ? */
            const double omega_ij = std::min(dvdr_Hubble, 0.0);
            const double mu_ij = fac_mu * r_inv * omega_ij; /* This is 0 or negative */

            /* Compute signal velocity */
            const double v_sig = hydro_signal_velocity(dx, pLocal__SoA__instance._9[pLocalIdx], pActive__SoA__instance._9[pActiveIdx], mu_ij, const_viscosity_beta);

            /* Grab balsara switches */
            const double balsara_i = pLocal__SoA__instance._10[pLocalIdx];
            const double balsara_j = pActive__SoA__instance._10[pActiveIdx];

            /* Construct the full viscosity term */
            const double rho_ij = 0.5 * (rhoi + rhoj);
            const double visc = -0.25 * v_sig * (balsara_i + balsara_j) * mu_ij / rho_ij;

            /* Convolve with the kernel */
            const double visc_acc_term =
                    0.5 * visc * (wi_dr * f_ij + wj_dr * f_ji) * r_inv;

            /* SPH acceleration term */
            const double sph_acc_term =
                    (P_over_rho2_i * wi_dr + P_over_rho2_j * wj_dr) * r_inv;

            /* Adaptive softening acceleration term */
            const double adapt_soft_acc_term = 0.0;  // doesn't exist in the minimal SPH scheme

            /* Assemble the acceleration */
            const double acc = sph_acc_term + visc_acc_term + adapt_soft_acc_term;

            /* Use the force Luke ! */
            pLocal__SoA__instance._12[pLocalIdx] -= mj * acc * dx[0] * distMask;
            pLocal__SoA__instance._13[pLocalIdx] -= mj * acc * dx[1] * distMask;
            #ifdef HYDRO_DIMENSION_3D
            pLocal[pLocalIdx]._a[2] -= mj * acc * dx[2] * distMask;
            #endif

            /* Get the time derivative for u. */
            const double sph_du_term_i = P_over_rho2_i * dvdr * r_inv * wi_dr;

            /* Viscosity term */
            const double visc_du_term = 0.5 * visc_acc_term * dvdr_Hubble;

            /* Assemble the energy equation term */
            const double du_dt_i = sph_du_term_i + visc_du_term;
            pLocal__SoA__instance._11[pLocalIdx] += du_dt_i * mj * distMask;
            pLocal__SoA__instance._14[pLocalIdx] -= mj * dvdr * r_inv / rhoj * wi_dr * f_ij * distMask;
            pLocal__SoA__instance._15[pLocalIdx] = std::max(pLocal__SoA__instance._15[pLocalIdx], v_sig * distMask);
        }
    }
for (int pLocal__SoA__instance__iter = 0; pLocal__SoA__instance__iter < pLocalSize; pLocal__SoA__instance__iter++) {
pLocal[pLocal__SoA__instance__iter]._uDot = pLocal__SoA__instance._11[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._a[0] = pLocal__SoA__instance._12[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._a[1] = pLocal__SoA__instance._13[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._hDot = pLocal__SoA__instance._14[pLocal__SoA__instance__iter];
pLocal[pLocal__SoA__instance__iter]._v_sig_AV = pLocal__SoA__instance._15[pLocal__SoA__instance__iter];
}


/** SOA epilogue loop skipped for target pActive: no items to write back */


}
