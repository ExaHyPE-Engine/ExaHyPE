// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

namespace exahype2 {
  namespace dg {
    namespace tests{

      constexpr double QuadratureNodes1dP1[] = { 0.21132486540518713,0.7886751345948129 };
      constexpr double QuadratureNodes1dP2[] = { 0.1127016653792583,0.5,0.8872983346207417};

      constexpr double DerivativeOperatorLagrangeP3[] = {
          -6.6640004727045631938153746887110173702239990234375,9.7203088313703940315235740854404866695404052734375,-4.21756469699035818621268845163285732269287109375,1.161256338324529568950538305216468870639801025390625,
          -1.5151152295984677831341969067580066621303558349609375,-0.7688287844464174458636307463166303932666778564453125,2.941340462561433444221847821609117090702056884765625,-0.6573964485165488813578349436284042894840240478515625,
          0.65739644851654832624632263105013407766819000244140625,-2.941340462561433444221847821609117090702056884765625,0.768828784446416335640606121160089969635009765625,1.515115229598468449268011681851930916309356689453125,
          -1.161256338324528680772118605091236531734466552734375,4.21756469699035729803426875150762498378753662109375,-9.720308831370392255166734685190021991729736328125,6.66400047270456497017221408896148204803466796875};

      constexpr double BasisFunctionValuesLeftP0[] = {
        1.0
      };
      constexpr double BasisFunctionValuesLeftP3[] = {
        1.5267881254572663873858573424513451755046844482421875,-0.8136324494869271450880887641687877476215362548828125,
        0.40076152031165046540905905203544534742832183837890625,-0.11391719628198997138479597879268112592399120330810546875};

      /*
       * Contains implementations of the functions required to test the dg implementation
       * for euler equations.
       */
      void eulerFlux(
              const double * __restrict__                  Q,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double * __restrict__                        F
            );

      void eulerSource(
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            double                                       dt,
            double * __restrict__                        S
          );

      double eulerEigenvalue(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double                                       dt,
          int                                         normal
          );

      void eulerBoundaryConditions(
              const double * __restrict__                  Qinside,
              double * __restrict__                         Qoutside,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       t,
              double                                       dt,
              int                                          normal
            );

      void eulerInitial(
        double * __restrict__  Q,
        int                    node=0
            );

      void elasticInitial(
        double * __restrict__                        Q
            );

      void elasticFlux(
              const double * __restrict__                  Q,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double * __restrict__                        F
            );

      void elasticSource(
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       t,
            double                                       dt,
            double * __restrict__                        S
          );

      double elasticEigenvalue(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double                                      dt,
          int                                         normal
          );

      void elasticBoundaryConditions(
              const double * __restrict__                  Qinside,
              double * __restrict__                        Qoutside,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       t,
              double                                       dt,
              int                                          normal
            );

      void elasticNonConservativeProduct(  const double * __restrict__  Q,
          const double * __restrict__                                   deltaQ,
          const tarch::la::Vector<Dimensions,double>&                   faceCentre,
          double                                                        t,
          double                                                        dt,
          int                                                           normal,
          double * __restrict__                                         BgradQ
        );


      // test boundary condition function to check whether applying boundary conditions works
      void testBoundaryConditions(
              const double * __restrict__                  Qinside,
              double * __restrict__                        Qoutside,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       t,
              double                                       dt,
              int                                          normal
            );


      void secondTestBoundaryConditions(
              const double * __restrict__                  Qinside,
              double * __restrict__                        Qoutside,
              const tarch::la::Vector<Dimensions,double>&  x,
              double                                       t,
              double                                       dt,
              int                                          normal
            );


      double testEigenvalue(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          double                                      dt,
          int                                         normal
          );
    }
  }
}
