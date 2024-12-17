#pragma once

#include <cmath>

struct xpart {
    double x_diff[3];
    double x_diff_sort[3];
    double v_full[3];
    double a_grav[3];
    double u_full;
};

struct part {
    long long id;
    struct gpart* gpart;
    double x[3];
    double v[3];
    double a_hydro[3];
    double mass;
    double h;
    double u;
    double u_dt;
    double rho;

//    union {
    struct {
        double wcount;
        double wcount_dh;
        double rho_dh;
        double div_v;
        double rot_v[3];
    } density;

    struct {
        double f;
        double pressure;
        double soundspeed;
        double v_sig;
        double h_dt;
        double balsara;
    } force;
//    };

    xpart xpart;
};

#if defined(HYDRO_GAMMA_5_3)
#define hydro_gamma 1.66666666666666667
#define hydro_gamma_minus_one 0.66666666666666667
#define hydro_gamma_plus_one 2.66666666666666667
#define hydro_one_over_gamma_minus_one 1.5
#define hydro_gamma_plus_one_over_two_gamma 0.8
#define hydro_gamma_minus_one_over_two_gamma 0.2
#define hydro_gamma_minus_one_over_gamma_plus_one 0.25
#define hydro_two_over_gamma_plus_one 0.75
#define hydro_two_over_gamma_minus_one 3.0
#define hydro_gamma_minus_one_over_two 0.33333333333333333
#define hydro_two_gamma_over_gamma_minus_one 5.0
#define hydro_one_over_gamma 0.6
#endif

#ifdef HYDRO_DIMENSION_2D
#define hydro_dimension 2.0
#define hydro_dimension_inv 0.5
#define hydro_dimension_unit_sphere ((double)M_PI)
#define hydro_dimension_unit_sphere_inv ((double)M_1_PI)
#elifdef HYDRO_DIMENSION_3D
#define hydro_dimension 3.0
#define hydro_dimension_inv 0.3333333333
#define hydro_dimension_unit_sphere ((double)(4. * M_PI / 3.))
#define hydro_dimension_unit_sphere_inv ((double)(3. * M_1_PI / 4.))
#endif

#ifdef HYDRO_DIMENSION_2D
#define kernel_gamma ((double)(1.778002))
#define kernel_constant ((double)(80. * M_1_PI / 7.))
#elifdef HYDRO_DIMENSION_3D
#define kernel_gamma ((double)(1.825742))
#define kernel_constant ((double)(16. * M_1_PI))
#endif

inline static double pow_gamma_minus_one(double x) {
#if defined(HYDRO_GAMMA_5_3)
    const double cbrt = std::cbrt(x); /* x^(1/3) */
        return cbrt * cbrt;               /* x^(2/3) */
#elif defined(HYDRO_GAMMA_7_5)
    return std::pow(x, 0.4); /* x^(2/5) */
#elif defined(HYDRO_GAMMA_4_3)
    return std::cbrt(x); /* x^(1/3) */
#elif defined(HYDRO_GAMMA_2_1)
    return x;
#else
    return 0.0;
#endif
}

inline static double gas_internal_energy_from_entropy(double density, double entropy) {
    return entropy * pow_gamma_minus_one(density) * hydro_one_over_gamma_minus_one;
}

inline static double gas_pressure_from_internal_energy(double density, double u) {
    return hydro_gamma_minus_one * u * density;
}

inline static double gas_soundspeed_from_internal_energy(double density, double u) {
    return sqrtf(u * hydro_gamma * hydro_gamma_minus_one);
}

inline static double gas_soundspeed_from_pressure(double density, double P) {
    return sqrtf(hydro_gamma * P / density);
}

inline static double approx_expf(double x) {
    return 1.0 + x * (1.0 + x * (0.5 + x * (1.0 / 6.0 + 1.0 / 24.0 * x)));
}

inline static double pow_three_gamma_minus_five_over_two(double x) {

#if defined(HYDRO_GAMMA_5_3)

    return 1.0; /* x^(0) */

#elif defined(HYDRO_GAMMA_7_5)

    return powf(x, -0.4); /* x^(-2/5) */

#elif defined(HYDRO_GAMMA_4_3)

    return 1.0 / sqrtf(x); /* x^(-1/2) */

#elif defined(HYDRO_GAMMA_2_1)

    return sqrtf(x); /* x^(1/2) */

#else

    error("The adiabatic index is not defined !");
    return 0.0;

#endif
}

inline static double pow_dimension_plus_one(double x) {

#if defined(HYDRO_DIMENSION_3D)

    const double x2 = x * x;
  return x2 * x2;

#elif defined(HYDRO_DIMENSION_2D)

    return x * x * x;

#elif defined(HYDRO_DIMENSION_1D)

    return x * x;

#else

    error("The dimension is not defined !");
    return 0.0;

#endif
}

/*
 * Remarks
 *
 * Adapted hydro_signal_velocity s.t. particles aren't accepted as args
 */
inline static double hydro_signal_velocity(const double dx[3], double pLocalSoundSpeed, double pActiveSoundSpeed, const double mu_ij, const double beta) {
    const double ci = pLocalSoundSpeed;
    const double cj = pActiveSoundSpeed;

    return ci + cj - beta * mu_ij;
}

#if defined(CUBIC_SPLINE_KERNEL)

/* Coefficients for the kernel. */
#define kernel_name "Cubic spline (M4)"
#define kernel_degree 3 /*!< Degree of the polynomial */
#define kernel_ivals 2  /*!< Number of branches */
#if defined(HYDRO_DIMENSION_3D)
#define kernel_gamma ((double)(1.825742))
#define kernel_constant ((double)(16. * M_1_PI))
#elif defined(HYDRO_DIMENSION_2D)
#define kernel_gamma ((double)(1.778002))
#define kernel_constant ((double)(80. * M_1_PI / 7.))
#elif defined(HYDRO_DIMENSION_1D)
#define kernel_gamma ((double)(1.732051))
#define kernel_constant ((double)(8. / 3.))
#endif
static const double kernel_coeffs[(kernel_degree + 1) * (kernel_ivals + 1)]
    __attribute__((aligned(16))) = {3.0,  -3.0, 0.0,  0.5, /* 0 < u < 0.5 */
                                    -1.0, 3.0,  -3.0, 1.0,  /* 0.5 < u < 1 */
                                    0.0,  0.0,  0.0,  0.0}; /* 1 < u */
#else
#error "A kernel function must be chosen at configure time !!"
#endif

/* First some powers of gamma = H/h */
#define kernel_gamma_inv ((double)(1. / kernel_gamma))
#define kernel_gamma2 ((double)(kernel_gamma * kernel_gamma))

/* define gamma^d, gamma^(d+1), 1/gamma^d and 1/gamma^(d+1) */
#if defined(HYDRO_DIMENSION_3D)
#define kernel_gamma_dim ((double)(kernel_gamma * kernel_gamma * kernel_gamma))
#define kernel_gamma_dim_plus_one \
  ((double)(kernel_gamma * kernel_gamma * kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim \
  ((double)(1. / (kernel_gamma * kernel_gamma * kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one \
  ((double)(1. / (kernel_gamma * kernel_gamma * kernel_gamma * kernel_gamma)))
#elif defined(HYDRO_DIMENSION_2D)
#define kernel_gamma_dim ((double)(kernel_gamma * kernel_gamma))
#define kernel_gamma_dim_plus_one \
  ((double)(kernel_gamma * kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim ((double)(1. / (kernel_gamma * kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one \
  ((double)(1. / (kernel_gamma * kernel_gamma * kernel_gamma)))
#elif defined(HYDRO_DIMENSION_1D)
#define kernel_gamma_dim ((double)(kernel_gamma))
#define kernel_gamma_dim_plus_one ((double)(kernel_gamma * kernel_gamma))
#define kernel_gamma_inv_dim ((double)(1. / (kernel_gamma)))
#define kernel_gamma_inv_dim_plus_one \
  ((double)(1. / (kernel_gamma * kernel_gamma)))
#endif

/* The number of branches (doubleing point conversion) */
#define kernel_ivals_f ((double)(kernel_ivals))

/* Kernel self contribution (i.e. W(0,h)) */
#define kernel_root                                          \
  ((double)(kernel_coeffs[kernel_degree]) * kernel_constant * \
   kernel_gamma_inv_dim)

/* Kernel normalisation constant (volume term) */
#define kernel_norm ((double)(hydro_dimension_unit_sphere * kernel_gamma_dim))


inline static void kernel_deval(double u, double * __restrict W, double * __restrict dW_dx) {
    /* Go to the range [0,1[ from [0,H[ */
    const double x = u * kernel_gamma_inv;

    /* Pick the correct branch of the kernel */
    const int temp = (int)(x * kernel_ivals_f);
    const int ind = temp > kernel_ivals ? kernel_ivals : temp;
    const double *const coeffs = &kernel_coeffs[ind * (kernel_degree + 1)];

    /* First two terms of the polynomial ... */
    double w = coeffs[0] * x + coeffs[1];
    double dw_dx = coeffs[0];

    /* ... and the rest of them */
    for (int k = 2; k <= kernel_degree; k++) {
        dw_dx = dw_dx * x + w;
        w = x * w + coeffs[k];
    }

    w = std::max(w, 0.0);
    dw_dx = std::min(dw_dx, 0.0);

    /* Return everything */
    *W = w * kernel_constant * kernel_gamma_inv_dim;
    *dW_dx = dw_dx * kernel_constant * kernel_gamma_inv_dim_plus_one;
}


struct viscosity_global_data {
    double alpha;
};

struct hydro_props {
    double eta_neighbours;
    double target_neighbours;
    double h_tolerance;
    double delta_neighbours;
    double h_max;
    double h_min_ratio;
    double h_min;
    int max_smoothing_iterations;
    int use_mass_weighted_num_ngb;
    double CFL_condition;
    double log_max_h_change;
    double minimal_temperature;
    double minimal_internal_energy;
    double initial_temperature;
    double initial_internal_energy;
    double hydrogen_mass_fraction;
    double hydrogen_ionization_temperature;
    double mu_neutral;
    double mu_ionised;
    int particle_splitting;
    double particle_splitting_mass_threshold;
    int generate_random_ids;
    struct viscosity_global_data viscosity;
};

#define const_viscosity_beta 3.0
#define hydro_props_default_viscosity_alpha 0.8

struct cosmology {
    double a;
    double a2_inv;
    double a3_inv;
    double a_factor_internal_energy;
    double a_factor_pressure;
    double a_factor_sound_speed;
    double a_factor_Balsara_eps;
    double H;
    double time;
    double w;
    double h;
};

inline static void hydro_reset_acceleration(struct part *p) {
    /* Reset the acceleration. */
    p->a_hydro[0] = 0.0;
    p->a_hydro[1] = 0.0;
    p->a_hydro[2] = 0.0;

    /* Reset the time derivatives. */
    p->u_dt = 0.0;
    p->force.h_dt = 0.0;
    p->force.v_sig = 2.0 * p->force.soundspeed;
}

inline static void hydro_init_part(struct part *p, const struct hydro_space *hs) {
    p->density.wcount = 0.0;
    p->density.wcount_dh = 0.0;
    p->rho = 0.0;
    p->density.rho_dh = 0.0;
    p->density.div_v = 0.0;
    p->density.rot_v[0] = 0.0;
    p->density.rot_v[1] = 0.0;
    p->density.rot_v[2] = 0.0;
}

inline static void hydro_first_init_part(struct part *p, struct xpart *xp) {
    xp->v_full[0] = p->v[0];
    xp->v_full[1] = p->v[1];
    xp->v_full[2] = p->v[2];
    xp->u_full = p->u;

    hydro_reset_acceleration(p);
    hydro_init_part(p, nullptr);
}

inline static void hydro_convert_quantities(
        struct part *p, struct xpart *xp,
        const struct cosmology *cosmo, const struct hydro_props *hydro_props,
        const struct pressure_floor_props *pressure_floor) {

    /* Convert the physcial internal energy to the comoving one. */
    /* u' = a^(3(g-1)) u */
    const double factor = 1.0 / cosmo->a_factor_internal_energy;
    p->u *= factor;
    xp->u_full = p->u;

    /* Apply the minimal energy limit */
    const double min_comoving_energy =
            hydro_props->minimal_internal_energy / cosmo->a_factor_internal_energy;
    if (xp->u_full < min_comoving_energy) {
        xp->u_full = min_comoving_energy;
        p->u = min_comoving_energy;
        p->u_dt = 0.0;
    }

    /* Compute the pressure */
    const double pressure = gas_pressure_from_internal_energy(p->rho, p->u);

    /* Compute the sound speed */
    const double soundspeed = gas_soundspeed_from_internal_energy(p->rho, p->u);

    p->force.pressure = pressure;
    p->force.soundspeed = soundspeed;
}
