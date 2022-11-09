#ifndef PEANO_ADIABATIC_INDEX_H
#define PEANO_ADIABATIC_INDEX_H


/* Powers of the adiabatic index */
#if defined(HYDRO_GAMMA_5_3)

#define hydro_gamma 1.66666666666666667f
#define hydro_gamma_minus_one 0.66666666666666667f
#define hydro_gamma_plus_one 2.66666666666666667f
#define hydro_one_over_gamma_minus_one 1.5f
#define hydro_gamma_plus_one_over_two_gamma 0.8f
#define hydro_gamma_minus_one_over_two_gamma 0.2f
#define hydro_gamma_minus_one_over_gamma_plus_one 0.25f
#define hydro_two_over_gamma_plus_one 0.75f
#define hydro_two_over_gamma_minus_one 3.f
#define hydro_gamma_minus_one_over_two 0.33333333333333333f
#define hydro_two_gamma_over_gamma_minus_one 5.f
#define hydro_one_over_gamma 0.6f

#elif defined(HYDRO_GAMMA_7_5)

#define hydro_gamma 1.4f
#define hydro_gamma_minus_one 0.4f
#define hydro_gamma_plus_one 2.4f
#define hydro_one_over_gamma_minus_one 2.5f
#define hydro_gamma_plus_one_over_two_gamma 0.857142857f
#define hydro_gamma_minus_one_over_two_gamma 0.142857143f
#define hydro_gamma_minus_one_over_gamma_plus_one 0.166666667f
#define hydro_two_over_gamma_plus_one 0.833333333
#define hydro_two_over_gamma_minus_one 5.f
#define hydro_gamma_minus_one_over_two 0.2f
#define hydro_two_gamma_over_gamma_minus_one 7.f
#define hydro_one_over_gamma 0.714285714f

#elif defined(HYDRO_GAMMA_4_3)

#define hydro_gamma 1.33333333333333333f
#define hydro_gamma_minus_one 0.33333333333333333f
#define hydro_gamma_plus_one 2.33333333333333333f
#define hydro_one_over_gamma_minus_one 3.f
#define hydro_gamma_plus_one_over_two_gamma 0.875f
#define hydro_gamma_minus_one_over_two_gamma 0.125f
#define hydro_gamma_minus_one_over_gamma_plus_one 0.142857143f
#define hydro_two_over_gamma_plus_one 0.857142857f
#define hydro_two_over_gamma_minus_one 6.f
#define hydro_gamma_minus_one_over_two 0.166666666666666666f
#define hydro_two_gamma_over_gamma_minus_one 8.f
#define hydro_one_over_gamma 0.75f

#elif defined(HYDRO_GAMMA_2_1)

#define hydro_gamma 2.f
#define hydro_gamma_minus_one 1.f
#define hydro_gamma_plus_one 3.f
#define hydro_one_over_gamma_minus_one 1.f
#define hydro_gamma_plus_one_over_two_gamma 0.75f
#define hydro_gamma_minus_one_over_two_gamma 0.25f
#define hydro_gamma_minus_one_over_gamma_plus_one 0.33333333333333333f
#define hydro_two_over_gamma_plus_one 0.66666666666666666f
#define hydro_two_over_gamma_minus_one 2.f
#define hydro_gamma_minus_one_over_two 0.5f
#define hydro_two_gamma_over_gamma_minus_one 4.f
#define hydro_one_over_gamma 0.5f

#else

#error "An adiabatic index needs to be chosen!"

#endif


#endif /* PEANO_ADIABATIC_INDEX_H */
