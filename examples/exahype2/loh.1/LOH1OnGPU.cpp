#include "LOH1OnGPU.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::loh1::LOH1OnGPU::_log( "examples::exahype2::loh1::LOH1OnGPU" );

void examples::exahype2::loh1::LOH1OnGPU::adjustSolution(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt
) 
{
    // initial conditions
  if (tarch::la::equals(t,0.0) ) 
  {
     double center_curve[3] = {0.0,0.0,2.0};
     bool upperLayer = volumeX(2) < 1.0;
     
     double rho = upperLayer ? 4.0 : 6.0;
     double cp  = upperLayer ? 2.0 : 3.464;
     double cs  = upperLayer ? 2.6 : 2.7;

     constexpr static int v_     =  0; 
     constexpr static int sigma_ =  3;
     constexpr static int rho_   =  9;
     constexpr static int cp_    = 10;
     constexpr static int cs_    = 11;
     constexpr static int alpha_ = 12;

     Q[rho_  ] = rho;
     Q[cp_   ] = cp;
     Q[cs_   ] = cs;
     Q[alpha_] = 1.0;

     double radius = 0.1;
     
     Q[ v_ + 0 ] = std::exp(-( (volumeX[0]-center_curve[0])*(volumeX[0]-center_curve[0])+
                               (volumeX[1]-center_curve[1])*(volumeX[1]-center_curve[1])+
                               (volumeX[2]-center_curve[2])*(volumeX[2]-center_curve[2]))/radius) * radius;
     Q[ v_ + 1 ] = Q[ v_ + 0 ];
     Q[ v_ + 2 ] = Q[ v_ + 1 ];
                               
     for(int i= 0 ; i < 6 ; i++)  Q[ sigma_ + i ] = 0.0;
  }
  else
  {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::loh1::LOH1OnGPU::boundaryConditions(
  const double * __restrict__                  Qinside,
  double * __restrict__                        Qoutside,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)
{
  std::fill_n(Qoutside, 13, 0);

  constexpr static int rho_   =  9;
  constexpr static int cp_    = 10;
  constexpr static int cs_    = 11;
  constexpr static int alpha_ = 12;

  Qoutside[ alpha_ ] = Qinside[ alpha_ ];
  Qoutside[ rho_ ]   = Qinside[ rho_ ];
  Qoutside[ cp_  ]   = Qinside[ cp_  ];
  Qoutside[ cs_  ]   = Qinside[ cs_  ];
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif


#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double examples::exahype2::loh1::LOH1OnGPU::maxEigenvalue(
  const double * __restrict__ Q, // Q[9+4],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) 
{
  constexpr static int cp_ = 10;
  constexpr static int cs_ = 11;
  double cp = Q[cp_];
  double cs = Q[cs_];
  
/*
  lambda[0] = -cp;
  lambda[1] =  cp;
  lambda[2] = -cs;
  lambda[3] =  cp;   I think this is a bug!
  lambda[4] = 0.0;
  lambda[5] = 0.0;
  lambda[6] = 0.0;
  lambda[7] = 0.0;
  lambda[8] = 0.0;
  lambda[9] = 0.0;
*/

  double result = 0.0;
  result = std::max( result, -cp );
  result = std::max( result,  cp );
  result = std::max( result, -cs );
  result = std::max( result,  cs );
  return result;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif



#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::loh1::LOH1OnGPU::nonconservativeProduct(
  const double * __restrict__ Q, // Q[9+4],
  const double * __restrict__             deltaQ, // [9+4]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[9]
) 
{

   constexpr static int v_     =  0; 
   constexpr static int sigma_ =  3;
   constexpr static int rho_   =  9;
   constexpr static int cp_    = 10;
   constexpr static int cs_    = 11;
   constexpr static int alpha_ = 12;

   double rho    = Q[rho_];  
   double cp     = Q[cp_];
   double cs     = Q[cs_];
   double alpha  = Q[alpha_];
   
   double mu     = rho*cs*cs;
   double lambda = rho*cp*cp-2*mu;
   double rho_i  = 1.0/rho;
   
   
   double alpha_i = alpha/(alpha*alpha + 1.0e-13 * (1.0-alpha));
   
   double u= Q[v_ + 0] * alpha_i;
   double v= Q[v_ + 1] * alpha_i;
   double w= Q[v_ + 2] * alpha_i;
   
   
   switch ( normal ) { 
     //dx
     case 0:
       {
       double alpha_x     = deltaQ[alpha_];
       BgradQ[v_ + 0]     = -rho_i*(deltaQ[sigma_ + 0] + 2*Q[sigma_ + 0]*alpha_x);
       BgradQ[v_ + 1]     = -rho_i*(deltaQ[sigma_ + 3] + 2*Q[sigma_ + 3]*alpha_x);
       BgradQ[v_ + 2]     = -rho_i*(deltaQ[sigma_ + 4] + 2*Q[sigma_ + 4]*alpha_x);
       
       BgradQ[sigma_ + 0] = -(lambda+2*mu) * ( deltaQ[v_ + 0]  - u * alpha_x);
       BgradQ[sigma_ + 1] =        -lambda * ( deltaQ[v_ + 0]  - u * alpha_x);       
       BgradQ[sigma_ + 2] =        -lambda * ( deltaQ[v_ + 0]  - u * alpha_x);       
       BgradQ[sigma_ + 3] =            -mu * ( deltaQ[v_ + 1]  - v * alpha_x);       
       BgradQ[sigma_ + 4] =            -mu * ( deltaQ[v_ + 2]  - w * alpha_x);       
       BgradQ[sigma_ + 5] =                                                     0.0;  
       }
       break; 
   
     ////dy                                                                                                                                              
     case 1:
       {
       double alpha_y  = deltaQ[alpha_];
       BgradQ[v_ + 0]     = -rho_i*(deltaQ[sigma_ + 3] + 2*Q[sigma_ + 3]*alpha_y);
       BgradQ[v_ + 1]     = -rho_i*(deltaQ[sigma_ + 1] + 2*Q[sigma_ + 1]*alpha_y);
       BgradQ[v_ + 2]     = -rho_i*(deltaQ[sigma_ + 5] + 2*Q[sigma_ + 5]*alpha_y);
       
       BgradQ[sigma_ + 0] =        -lambda * ( deltaQ[v_ + 1] - v * alpha_y);             
       BgradQ[sigma_ + 1] = -(lambda+2*mu) * ( deltaQ[v_ + 1] - v * alpha_y);   
       BgradQ[sigma_ + 2] =        -lambda * ( deltaQ[v_ + 1] - v * alpha_y);             
       BgradQ[sigma_ + 3] =            -mu * ( deltaQ[v_ + 0] - u * alpha_y);             
       BgradQ[sigma_ + 4] =                                             0.0;         
       BgradQ[sigma_ + 5] =            -mu * ( deltaQ[v_ + 2] - w * alpha_y);  
       }
       break; 
                                                                                            ////       !                                               
     ////dz                                                                                                                                              
     case 2:
       {
       double alpha_z  = deltaQ[alpha_];
       BgradQ[v_ + 0]     = -rho_i*(deltaQ[sigma_ + 4] + 2*Q[sigma_ + 4]*alpha_z);
       BgradQ[v_ + 1]     = -rho_i*(deltaQ[sigma_ + 5] + 2*Q[sigma_ + 5]*alpha_z);
       BgradQ[v_ + 2]     = -rho_i*(deltaQ[sigma_ + 2] + 2*Q[sigma_ + 2]*alpha_z);
       BgradQ[sigma_ + 0] =        -lambda * ( deltaQ[v_ + 2] - w * alpha_z);           
       BgradQ[sigma_ + 1] =        -lambda * ( deltaQ[v_ + 2] - w * alpha_z);           
       BgradQ[sigma_ + 2] = -(lambda+2*mu) * ( deltaQ[v_ + 2] - w * alpha_z); 
       BgradQ[sigma_ + 3] =                                                                
       BgradQ[sigma_ + 4] =            -mu * ( deltaQ[v_ + 0] - u * alpha_z);           
       BgradQ[sigma_ + 5] =            -mu * ( deltaQ[v_ + 1] - v * alpha_z);
       }
       break;
   
     default:
       break; 
   }
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
