#include "ADERDGLOH1.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::loh1::ADERDGLOH1::_log( "examples::exahype2::loh1::ADERDGLOH1" );

void examples::exahype2::loh1::ADERDGLOH1::prescribeGaussianWave(
  const tarch::la::Vector<Dimensions,double>&  x,
  double Q[]
) {
  double center_curve[3];

  center_curve[0] = 0.0;
  center_curve[1] = 0.0;
  center_curve[2] = 2.0;

  // 1.0 is the parameter from the setup, but we need at least one voxel
  bool layerWidth = std::max( getMinMeshSize() / (2*Order+1), 1.0 ); // TODO guess

  bool upperLayer = x(2) <= layerWidth;
  
  //double rho = upperLayer ? 2.67   : 2.6;
  //double cp  = upperLayer ? 6.0    : 4.0;
  //double cs  = upperLayer ? 3.343  : 2.0;
  double rho = upperLayer ? 4.0 : 6.0;
  double cp  = upperLayer ? 2.0 : 3.464;
  double cs  = upperLayer ? 2.6 : 2.7;

  Q[s.rho  ] = rho;
  Q[s.cp   ] = cp;
  Q[s.cs   ] = cs;
  Q[s.alpha] = 1.0;

  double radius = 2.0*layerWidth;
  //double height = 3.0;
  
  Q[ s.v + 0 ] = std::exp(-((x[0]-center_curve[0])*(x[0]-center_curve[0])+
                            (x[1]-center_curve[1])*(x[1]-center_curve[1])+
                            (x[2]-center_curve[2])*(x[2]-center_curve[2]))/radius) * radius;
  Q[ s.v + 1 ] = Q[ s.v + 0 ];
  Q[ s.v + 2 ] = Q[ s.v + 1 ];
                            

  for(int i= 0 ; i < 6 ; i++){
    Q[ s.sigma + i ] = 0.0;
  }
}

void examples::exahype2::loh1::ADERDGLOH1::adjustSolution(
  double * __restrict__                       Q,
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t
) {
  logTraceInWith2Arguments( "adjustSolution(...)", x, t );
  if (tarch::la::equals(t,0.0) ) {
    prescribeGaussianWave(x,Q);
  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}




void examples::exahype2::loh1::ADERDGLOH1::boundaryConditions(
  const double * __restrict__                 Qinside, // Qinside[9+4]
  double * __restrict__                       Qoutside, // Qoutside[9+4]
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );
  #ifdef Asserts
  if(!std::isfinite(1.0/Qinside[s.rho])){
    std::cout << "FV: rho_inv not finite in boundary values" << std::endl;
    for(int i=0;i< Unknowns+NumberOfParameters; i++){
      std::cout << Qinside[i] << std::endl;
    }
  }
  #endif                                         

  assertion2(std::isfinite(1.0/Qinside[s.rho]),
         "rho_i is not finite",Qinside[s.rho]);

  std::fill_n(Qoutside, 13, 0);

  Qoutside[ s.alpha ] = Qinside[ s.alpha ];
  Qoutside[ s.rho ]   = Qinside[ s.rho ];
  Qoutside[ s.cp  ]   = Qinside[ s.cp  ];
  Qoutside[ s.cs  ]   = Qinside[ s.cs  ];

  assertion2(std::isfinite(1.0/Qoutside[s.rho]), "rho_i is not finite",Qoutside[s.rho]);
  
  logTraceOut( "boundaryConditions(...)" );
}

double examples::exahype2::loh1::ADERDGLOH1::maxEigenvalue(
  const double * __restrict__ Q, // Q[9+4],
  const tarch::la::Vector<Dimensions,double>& x,
  double                                      t,
  int                                         normal
)  {
  logTraceInWith2Arguments( "maxEigenvalue(...)", x, t );
  double cp = Q[s.cp];
  double cs = Q[s.cs];

  double result = 0.0;
  result = std::max( result, std::abs(cp) );
  result = std::max( result, std::abs(cs) );
  logTraceOut( "maxEigenvalue(...)" );
  return result;
}


void examples::exahype2::loh1::nonconservativeProduct(
  const double * __restrict__                  Q, // [9+4],
  const double * __restrict__                  deltaQ, // [9+4],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[13]
 ) {
  double rho    = Q[s.rho];  
  double cp     = Q[s.cp];
  double cs     = Q[s.cs];
  double alpha  = Q[s.alpha];

  double mu     = rho*cs*cs;
  double lambda = rho*cp*cp-2*mu;
  double rho_i  = 1.0/rho;

  assertion2(std::isfinite(rho_i),"BGradQ: rho inverse not finite",rho);

  double alpha_i = alpha/(alpha*alpha + 1.0e-13 * (1.0-alpha));

  double u= Q[s.v + 0] * alpha_i;
  double v= Q[s.v + 1] * alpha_i;
  double w= Q[s.v + 2] * alpha_i;

  assertion(normal>=0);
  assertion(normal<3);
  switch ( normal ) { 
    //dx
    case 0:
      {
      double alpha_x  = deltaQ[s.alpha];
      BgradQ[s.v + 0] = -rho_i*(deltaQ[s.sigma + 0] + 2*Q[s.sigma + 0]*alpha_x);   //   AQx(7) = - irho * Qx(1) - 2*Q(1)*irho*Qx(13)        
      BgradQ[s.v + 1] = -rho_i*(deltaQ[s.sigma + 3] + 2*Q[s.sigma + 3]*alpha_x);   //   AQx(8) = - irho * Qx(4) - 2*Q(4)*irho*Qx(13)        
      BgradQ[s.v + 2] = -rho_i*(deltaQ[s.sigma + 4] + 2*Q[s.sigma + 4]*alpha_x);   //   AQx(9) = - irho * Qx(6) - 2*Q(6)*irho*Qx(13)        
      BgradQ[s.sigma + 0] =-(lambda+2*mu) * ( deltaQ[s.v + 0]  - u * alpha_x);   //   AQx(1) = - (lam+2*mu)*Qx(7) + (lam+2*mu)*u(1)*Qx(13) 
      BgradQ[s.sigma + 1] =       -lambda * ( deltaQ[s.v + 0]  - u * alpha_x);   //   AQx(2) = - lam*Qx(7)        + lam*u(1)*Qx(13)               
      BgradQ[s.sigma + 2] =       -lambda * ( deltaQ[s.v + 0]  - u * alpha_x);   //   AQx(3) = - lam*Qx(7)        + lam*u(1)*Qx(13)               
      BgradQ[s.sigma + 3] =           -mu * ( deltaQ[s.v + 1]  - v * alpha_x);   //   AQx(4) = - mu *Qx(8)        + mu *u(2)*Qx(13)               
      BgradQ[s.sigma + 4] =           -mu * ( deltaQ[s.v + 2]  - w * alpha_x);   //   AQx(6) = - mu *Qx(9)        + mu *u(3)*Qx(13)               
      BgradQ[s.sigma + 5] =                                                     0.0;   //   AQx(5) =   0.0
      }
      break; 
  
    //dy                                                                                                                                              
    case 1:
      {
      double alpha_y  = deltaQ[s.alpha];
      BgradQ[s.v + 0] = -rho_i*(deltaQ[s.sigma + 3] + 2*Q[s.sigma + 3]*alpha_y);   //   BQy(7) = - irho * Qy(4) - 2*Q(4)*irho*Qy(13)        
      BgradQ[s.v + 1] = -rho_i*(deltaQ[s.sigma + 1] + 2*Q[s.sigma + 1]*alpha_y);   //   BQy(8) = - irho * Qy(2) - 2*Q(2)*irho*Qy(13)        
      BgradQ[s.v + 2] = -rho_i*(deltaQ[s.sigma + 5] + 2*Q[s.sigma + 5]*alpha_y);   //   BQy(9) = - irho * Qy(5) - 2*Q(5)*irho*Qy(13)        
      BgradQ[s.sigma + 0] =        -lambda * ( deltaQ[s.v + 1] - v * alpha_y);   //   BQy(1) = - lam*Qy(8)        + lam*u(2)*Qy(13)           
      BgradQ[s.sigma + 1] = -(lambda+2*mu) * ( deltaQ[s.v + 1] - v * alpha_y);   //   BQy(2) = - (lam+2*mu)*Qy(8) + (lam+2*mu)*u(2)*Qy(13) 
      BgradQ[s.sigma + 2] =        -lambda * ( deltaQ[s.v + 1] - v * alpha_y);   //   BQy(3) = - lam*Qy(8)        + lam*u(2)*Qy(13)           
      BgradQ[s.sigma + 3] =            -mu * ( deltaQ[s.v + 0] - u * alpha_y);   //   BQy(4) = - mu *Qy(7)        + mu *u(1)*Qy(13)           
      BgradQ[s.sigma + 4] =                                                     0.0;   //   BQy(6) =   0.0                                      
      BgradQ[s.sigma + 5] =            -mu * ( deltaQ[s.v + 2] - w * alpha_y);   //   BQy(5) = - mu *Qy(9)        + mu *u(3)*Qy(13)
      }
      break; 
                                                                                           //       !                                               
    //dz                                                                                                                                              
    case 2:
      {
      double alpha_z  = deltaQ[s.alpha];
      BgradQ[s.v + 0] = -rho_i*(deltaQ[s.sigma + 4] + 2*Q[s.sigma + 4]*alpha_z);   //   CQz(7) = - irho * Qz(6) - 2*Q(6)*irho*Qz(13)        
      BgradQ[s.v + 1] = -rho_i*(deltaQ[s.sigma + 5] + 2*Q[s.sigma + 5]*alpha_z);   //   CQz(8) = - irho * Qz(5) - 2*Q(5)*irho*Qz(13)        
      BgradQ[s.v + 2] = -rho_i*(deltaQ[s.sigma + 2] + 2*Q[s.sigma + 2]*alpha_z);   //   CQz(9) = - irho * Qz(3) - 2*Q(3)*irho*Qz(13)        
      BgradQ[s.sigma + 0] =        -lambda * ( deltaQ[s.v + 2] - w * alpha_z);   //   CQz(1) = - lam*Qz(9)        + lam*u(3)*Qz(13)           
      BgradQ[s.sigma + 1] =        -lambda * ( deltaQ[s.v + 2] - w * alpha_z);   //   CQz(2) = - lam*Qz(9)        + lam*u(3)*Qz(13)           
      BgradQ[s.sigma + 2] = -(lambda+2*mu) * ( deltaQ[s.v + 2] - w * alpha_z);   //   CQz(3) = - (lam+2*mu)*Qz(9) + (lam+2*mu)*u(3)*Qz(13) 
      BgradQ[s.sigma + 3] =                                                     0.0;   //   CQz(4) =  0.0                                       
      BgradQ[s.sigma + 4] =            -mu * ( deltaQ[s.v + 0] - u * alpha_z);   //   CQz(6) = - mu *Qz(7)        + mu *u(1)*Qz(13)           
      BgradQ[s.sigma + 5] =            -mu * ( deltaQ[s.v + 1] - v * alpha_z);   //   CQz(5) = - mu *Qz(8)        + mu *u(2)*Qz(13)
      }
      break;

    default:
#ifdef Asserts
      assertionMsg(false,"Normal direction/gradQ component must be in {0,1,2}.");
#endif  
      break; 
  }
  
#ifdef Asserts
  assertion2(std::isfinite(BgradQ[s.v + 0]),"BGradQ: v not finite",BgradQ[s.v + 0]);
  assertion2(std::isfinite(BgradQ[s.v + 1]),"BGradQ: u not finite",BgradQ[s.v + 1]);
  assertion2(std::isfinite(BgradQ[s.v + 2]),"BGradQ: w not finite",BgradQ[s.v + 2]);
  assertion10(std::isfinite(BgradQ[s.sigma + 0]),"BGradQ: sigma_xx not finite",BgradQ[s.sigma + 0],lambda,alpha_i,gradQ[idx(1,s.v + 1)],gradQ[idx(2,s.v + 2)],v,w,alpha_y,alpha_z);
  assertion2(std::isfinite(BgradQ[s.sigma + 1]),"BGradQ: sigma_yy not finite",BgradQ[s.sigma + 1]);
  assertion2(std::isfinite(BgradQ[s.sigma + 2]),"BGradQ: sigma_zz not finite",BgradQ[s.sigma + 2]);
  assertion2(std::isfinite(BgradQ[s.sigma + 3]),"BGradQ: sigma_xy not finite",BgradQ[s.sigma + 3]);
  assertion2(std::isfinite(BgradQ[s.sigma + 4]),"BGradQ: sigma_xz not finite",BgradQ[s.sigma + 4]);
  assertion2(std::isfinite(BgradQ[s.sigma + 5]),"BGradQ: sigma_yz not finite",BgradQ[s.sigma + 5]);
#endif  
  logTraceOut( "nonconservativeProduct(...)" );
}
