#include <gtest/gtest.h>
#include <stdexcept>
#include <numeric>
#include <random>
#include "FusanovSYCL.cpph"
#include "comp.h"
#include "test/solvers/TestEuler.h"

#include <CL/sycl.hpp>
using namespace sycl;


static queue mQ(default_selector{});


void init_pd(sycl::queue Q,  patchData3d &pD, const int NPT, const int srcSize, const int dstSize)
{
  const int LSRC = srcSize*NPT;
  const int LDST = dstSize*NPT;


  pD.npatches=NPT;
  pD.nsrc = LSRC;
  pD.ndst = LDST;
  pD.reco             = malloc_shared<double>(LSRC, Q);
  pD.result           = malloc_shared<double>(LDST, Q);
  pD.id               = malloc_shared<   int>(NPT,  Q);
  pD.x0               = malloc_shared<double>(NPT,  Q);
  pD.h0               = malloc_shared<double>(NPT,  Q);
  pD.x1               = malloc_shared<double>(NPT,  Q);
  pD.h1               = malloc_shared<double>(NPT,  Q);
  pD.x2               = malloc_shared<double>(NPT,  Q);
  pD.h2               = malloc_shared<double>(NPT,  Q);
  pD.t                = malloc_shared<double>(NPT,  Q);
  pD.dt               = malloc_shared<double>(NPT,  Q);
  pD.maxEigenvalue    = malloc_shared<double>(NPT,  Q);
  
  std::fill(pD.reco  , pD.reco  +LSRC,  1);
  std::fill(pD.result, pD.result+LDST, -1);
  std::iota(pD.id, pD.id+NPT, 0);
  std::iota(pD.x0           , pD.x0           +NPT,  1);
  std::iota(pD.h0           , pD.h0           +NPT,  1);
  std::iota(pD.x1           , pD.x1           +NPT,  1);
  std::iota(pD.h1           , pD.h1           +NPT,  1);
  std::iota(pD.x2           , pD.x2           +NPT,  1);
  std::iota(pD.h2           , pD.h2           +NPT,  1);
  std::iota(pD.t            , pD.t            +NPT,  1);
  std::iota(pD.dt           , pD.dt           +NPT,  1);
  std::fill(pD.maxEigenvalue, pD.maxEigenvalue+NPT, -1);
}

void dealloc_pd(sycl::queue Q,  patchData3d &pD)
{
  free(pD.reco         ,  Q);
  free(pD.result       ,  Q);
  free(pD.id           ,  Q);
  free(pD.x0           ,  Q);
  free(pD.h0           ,  Q);
  free(pD.x1           ,  Q);
  free(pD.h1           ,  Q);
  free(pD.x2           ,  Q);
  free(pD.h2           ,  Q);
  free(pD.t            ,  Q);
  free(pD.dt           ,  Q);
  free(pD.maxEigenvalue,  Q);
}

TEST(SolverTest, EulerLike)
{
  const size_t N(4), n(5);
  const int unknowns(5);
  const int srcPS = (n+2)*(n+2)*(n+2)*unknowns; // +aux!!!
  const int destPS = n*n*n*unknowns;// +aux!!!
  
   
  // Data preparation
  patchData3d xx;
  init_pd(mQ, xx, N, srcPS, destPS);
  patchData3d yy;
  init_pd(mQ, yy, N, srcPS, destPS);
 
  // Random data generation for the input arrays makin sure
  // both are identical (we test the results of xx against those of yy)
  std::uniform_real_distribution<double> unif(0.1, 0.11);
  std::default_random_engine re(time(NULL));
  for (int i=0;i<xx.nsrc;i++)
  {
    xx.reco[i] = unif(re);
    yy.reco[i] = xx.reco[i];
  }

  exahype2::fv::Fusanov_3D_with_SYCL_offloading<n,5,0,TestEuler,false,false,false,false>(mQ, xx);

                                     Fusanov_3D<n,5,0,TestEuler,false,false,false,false>(yy);
 

  
  for (int i=0;i<xx.ndst;i++)
  {
    xx.reco[i] = unif(re);
    yy.reco[i] = xx.reco[i];
    EXPECT_DOUBLE_EQ(xx.result[i], yy.result[i]);
  }


  for (int i=0;i<N;i++)
  {
    EXPECT_DOUBLE_EQ(xx.maxEigenvalue[i], yy.maxEigenvalue[i]);
  } 

  dealloc_pd(mQ, xx);
  dealloc_pd(mQ, yy);
}
