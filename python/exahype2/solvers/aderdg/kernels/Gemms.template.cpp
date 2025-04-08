// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Gemms.h"

{% if USE_LIBXSMM %}

#include "AMRRoutines_libxsmm.c"
#include "FusedSpaceTimePredictorVolumeIntegral_libxsmm.c"

{% endif %}{# USE_LIBXSMM #}
