// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Gemms.h"

{% if USE_LIBXSMM %}

#include "asm_limiter.c"

{% endif %}{# USE_LIBXSMM #}
