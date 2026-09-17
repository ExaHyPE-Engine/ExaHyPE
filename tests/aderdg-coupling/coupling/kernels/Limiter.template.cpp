// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Limiter.h"

#include "peano4/Loop.h"

void {{FULL_QUALIFIED_NAMESPACE}}::regularToLimiterFace(
  const {{REGULAR_SOLVER_CORRECTION_PRECISION}}* Qfrom,
  {{LIMITER_SOLVER_CORRECTION_PRECISION}}* Qto,
  int numVars){

  constexpr int toPoints    = {{LIMITER_SOLVER_ORDER}} + 1;

#if DIMENSIONS==3
  const int toSize    = toPoints*toPoints*numVars;
#else
  const int toSize    = toPoints*numVars;
#endif

{% if DIFF_ORDERS %}

  constexpr int fromPoints  = {{REGULAR_SOLVER_ORDER}} + 1;

  constexpr {{REGULAR_SOLVER_CORRECTION_PRECISION}} proj[toPoints*fromPoints] = {
    {{PROJ_REGULAR_TO_LIMITER}}
  };

  std::fill_n(Qto, toSize, 0.0);

  //For each point we project to
  peano4::dForEachExclusive(toPoints, DIMENSIONS-1, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dofTo) {
    const int dofToSerialised = dofTo[0]
  #if DIMENSIONS==3
    + dofTo[1] * toPoints
  #endif
    ;

    //Compute the contribution from each contributing point
    peano4::dForEachExclusive(fromPoints, DIMENSIONS-1, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dofFrom) {
      const int dofFromSerialised = dofFrom[0]
  #if DIMENSIONS==3
        + dofFrom[1] * fromPoints
  #endif
    ;

      for(int var=0; var<numVars; var++){
        Qto[dofToSerialised*numVars+var] += Qfrom[dofFromSerialised*numVars+var] * proj[dofTo[0]*fromPoints+dofFrom[0]]
  #if DIMENSIONS==3
          * proj[dofTo[1]*fromPoints+dofFrom[1]]
  #endif
        ;
      }
    });

  });

{% else %}{# Both solvers have the same order #}
  std::copy_n(Qfrom, toSize, Qto);
{% endif %}
  
}

void {{FULL_QUALIFIED_NAMESPACE}}::limiterToRegularFace(
  const {{LIMITER_SOLVER_CORRECTION_PRECISION}}* Qfrom,
  {{REGULAR_SOLVER_CORRECTION_PRECISION}}* Qto,
  int numVars){

  constexpr int toPoints    = {{REGULAR_SOLVER_ORDER}} + 1;

#if DIMENSIONS==3
  const int toSize    = toPoints*toPoints*numVars;
#else
  const int toSize    = toPoints*numVars;
#endif

{% if DIFF_ORDERS %}

  constexpr int fromPoints  = {{LIMITER_SOLVER_ORDER}} + 1;

  constexpr {{LIMITER_SOLVER_CORRECTION_PRECISION}} proj[toPoints*fromPoints] = {
    {{PROJ_LIMITER_TO_REGULAR}}
  };

  std::fill_n(Qto, toSize, 0.0);

  //For each point we project to
  peano4::dForEachExclusive(toPoints, DIMENSIONS-1, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dofTo) {
    const int dofToSerialised = dofTo[0]
  #if DIMENSIONS==3
    + dofTo[1] * toPoints
  #endif
    ;

    //Compute the contribution from each contributing point
    peano4::dForEachExclusive(fromPoints, DIMENSIONS-1, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dofFrom) {
      const int dofFromSerialised = dofFrom[0]
  #if DIMENSIONS==3
        + dofFrom[1] * fromPoints
  #endif
    ;

      for(int var=0; var<numVars; var++){
        Qto[dofToSerialised*numVars+var] += Qfrom[dofFromSerialised*numVars+var] * proj[dofTo[0]*fromPoints+dofFrom[0]]
  #if DIMENSIONS==3
          * proj[dofTo[1]*fromPoints+dofFrom[1]]
  #endif
        ;
      }
    });

  });

{% else %}{# Both solvers have the same order #}
  std::copy_n(Qfrom, toSize, Qto);
{% endif %}

}