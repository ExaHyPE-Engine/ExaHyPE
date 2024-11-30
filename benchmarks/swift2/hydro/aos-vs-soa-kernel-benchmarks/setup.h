#pragma once

#define HYDRO_DIMENSION_2D 2
#define HYDRO_GAMMA_5_3
#define CUBIC_SPLINE_KERNEL

#define DT_DRIFT ((double) 1e-7)
#define DT_THERM DT_DRIFT

#define DT_KICK_HYDRO ((double) 5e-8)
#define DT_KICK_THERM DT_KICK_HYDRO

constexpr auto N_PART = 2048;
constexpr auto STEPS = 10;
constexpr auto LEVEL = 5;

constexpr auto SPLIT = 64;

#define USE_MASKED_KERNELS

#ifdef USE_MASKED_KERNELS
 #define HYDRO_KERNEL HydroKernel_MASKED
 #define DRIFT_KERNEL DriftKernel_MASKED
#else
#define HYDRO_KERNEL HydroKernel
#define DRIFT_KERNEL DriftKernel
#endif
