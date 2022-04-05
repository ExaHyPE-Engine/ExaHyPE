#pragma once

#include "tarch/la/Vector.h"

class TestEuler
{
    public:

        enum class Offloadable {
          Yes
        };

        static inline double maxEigenvalue(const double * __restrict__ Q,
            const tarch::la::Vector<3,double>& dummy1,
            const tarch::la::Vector<3,double>& dummy2,
            double dummy3,
            int normal, Offloadable)
        #ifdef UseManualInlining
        __attribute__((always_inline))
        #endif
        {
          constexpr double gamma = 1.4;
          const double irho = 1./Q[0]; // In test data make sure Q[0] is not zer0
          const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));

          const double u_n = Q[normal + 1] * irho;
          //const double c   = std::sqrt(gamma * p * irho);  // The "true" formula does not 
          const double c   = std::sqrt(abs(gamma * p * irho)); // have abs in it, we cheat this to be psoitive to prevent nan

          double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
          return result;
        }

        static inline void sourceTerm(
          const double * __restrict__ Q,
          const tarch::la::Vector<3,double>&  volumeCentre,
          const tarch::la::Vector<3,double>&  volumeH,
          double                                       t,
          double                                       dt,
          double * __restrict__ S,
          Offloadable
        ) 
        {
          // Not the real euler thing, just some silly function for testing
          for (int i=0;i<5;i++) S[i] = 1 + i;
        };
    
        static void flux(
          const double * __restrict__ Q,
          const tarch::la::Vector<3,double>&  faceCentre,
          const tarch::la::Vector<3,double>&  volumeH,
          double                                       t,
          int                                          normal,
          double * __restrict__ F,
          Offloadable
        ) 
        {
          constexpr double gamma = 1.4;
          const double irho = 1./Q[0];
          const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
          
          const double coeff = irho*Q[normal+1];
          F[0] = coeff*Q[0];
          F[1] = coeff*Q[1];
          F[2] = coeff*Q[2];
          F[3] = coeff*Q[3];
          F[4] = coeff*Q[4];
          F[normal+1] += p;
          F[4]        += coeff*p;
          
        };
    
        static void nonconservativeProduct(
          const double * __restrict__ Q,
          const double * __restrict__             deltaQ,
          const tarch::la::Vector<3,double>&  faceCentre,
          const tarch::la::Vector<3,double>&  volumeH,
          double                                       t,
          int                                          normal,
          double * __restrict__                        BgradQ,
          Offloadable
        ) 
        {
          // Again just some data manipulation
          for (int i=0;i<5;i++) BgradQ[i] = 10+i;
        };
};
