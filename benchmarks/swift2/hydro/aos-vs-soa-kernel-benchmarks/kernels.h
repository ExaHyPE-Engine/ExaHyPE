#pragma once

#include "swift.h"

extern struct HYDRO_STATS_T {
    double h_max = 0;
    double h_max_active = 0;
} HYDRO_STATS;

extern const hydro_props HYDRO_PROPS;

extern const cosmology COSMO;

template<typename part>
void Kick1Kernel(part *particles, int size);

template<typename part>
void Kick2Kernel(part *particles, int size);

template<typename part>
void DriftKernel(part *particles, int size);

template<typename part>
void DriftKernel_MASKED(part *particles, int size);

template<typename part>
void HydroKernel(part *pLocal, int pLocalSize, part *pActive, int pActiveSize);

template<typename part>
void HydroKernel_MASKED(part *pLocal, int pLocalSize, part *pActive, int pActiveSize);
