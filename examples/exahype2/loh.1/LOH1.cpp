#include "LOH1.h"

#include <algorithm>
#include <iomanip>

tarch::logging::Log   examples::exahype2::loh1::LOH1::_log( "examples::exahype2::loh1::LOH1" );



::exahype2::RefinementCommand examples::exahype2::loh1::LOH1::refinementCriterion(
  double                                       Q[13],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(h)) ) {{
    result = ::exahype2::RefinementCommand::Refine;
  }}

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}

// TODO: No point sources supported
//void examples::exahype2::loh1::LOH1::prescribeLOH1InitialData(
//    const tarch::la::Vector<Dimensions,double>&  x,
//		double Q[]
//) {
//    // initial conditions
//   constexpr idx_rho = 9; 
//   if( x[1] < 1.0) {
//      Q[s.rho+0] = 2.6;
//      Q[s.cp] = 4.0;
//      Q[s.cs] = 2.0;
//    }else{
//      Q[s.rho+0] = 2.7;
//      Q[s.cp] = 6.0;
//      Q[s.cs] = 3.464;
//    }
//}

void examples::exahype2::loh1::LOH1::prescribeGaussianWave(
    const tarch::la::Vector<Dimensions,double>&  x,
		double Q[]
) {
  double center_curve[3];

  center_curve[0] = 15.0;
  center_curve[1] = 15.0;
  center_curve[2] = 15.0;    

  double rho = 2.67;
  double cp  = 6.0;
  double cs  = 3.343;
  
  Q[s.rho  ] = rho;
  Q[s.cp   ] = cp;
  Q[s.cs   ] = cs;
  Q[s.alpha] = 1.0;

  double radius = 3.0 ;
  double height = 3.0;
  
  Q[ s.v + 0 ] = std::exp(-((x[0]-center_curve[0])*(x[0]-center_curve[0])+
                            (x[1]-center_curve[1])*(x[1]-center_curve[1])+
                            (x[2]-center_curve[2])*(x[2]-center_curve[2]))/radius) * radius;
  Q[ s.v + 1 ] = Q[ s.v + 0 ];
  Q[ s.v + 2 ] = Q[ s.v + 1 ];
                            

  for(int i= 0 ; i < 6 ; i++){
    Q[ s.sigma + i ] = 0.0;
  }
}



void examples::exahype2::loh1::LOH1::adjustSolution(
  double Q[13],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", x, h, t );
  if (tarch::la::equals(t,0.0) ) {
    prescribeGaussianWave(x,Q);
  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}


double examples::exahype2::loh1::LOH1::maxEigenvalue(
  double                                       Q[13],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  double cp = Q[s.cp];
  double cs = Q[s.cs];
  
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


void examples::exahype2::loh1::LOH1::boundaryConditions(
  double                                       Qinside[13],
  double                                       Qoutside[13],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  std::fill_n(Qoutside, 13, 0);

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

  Qoutside[ s.alpha ] = Qinside[ s.alpha ];
  Qoutside[ s.rho ]   = Qinside[ s.rho ];
  Qoutside[ s.cp  ]   = Qinside[ s.cp  ];
  Qoutside[ s.cs  ]   = Qinside[ s.cs  ];

  assertion2(std::isfinite(1.0/Qoutside[s.rho]),
             "rho_i is not finite",Qoutside[s.rho]);
  
  logTraceOut( "boundaryConditions(...)" );
}


void examples::exahype2::loh1::LOH1::nonconservativeProduct(
  double                                       Q[13],
  double                                       gradQ[13][Dimensions],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       BgradQ[13]
) {
  logTraceIn( "nonconservativeProduct(...)" );
 
  // modificatons to ExaSeis original
 
  BgradQ[s.rho]= 0.0;  
  BgradQ[s.cp]= 0.0;
  BgradQ[s.cs]= 0.0;
  BgradQ[s.alpha]= 0.0;
  #define idx(dim,var) var][dim

  // below as in original code

  double rho    = Q[s.rho];  
  double cp     = Q[s.cp];
  double cs     = Q[s.cs];
  double alpha  = Q[s.alpha];

  double mu     = rho*cs*cs;
  double lambda = rho*cp*cp-2*mu;
  double rho_i  = 1.0/rho;

  assertion2(std::isfinite(rho_i),"BGradQ: rho inverse not finite",rho);

  double alpha_i = alpha/(alpha*alpha + 1.0e-13 * (1-alpha));

  double u= Q[s.v + 0] * alpha_i;
  double v= Q[s.v + 1] * alpha_i;
  double w= Q[s.v + 2] * alpha_i;

  double alpha_x  = gradQ[idx(0,s.alpha)];
  double alpha_y  = gradQ[idx(1,s.alpha)];
  double alpha_z  = gradQ[idx(2,s.alpha)];


  //  BgradQ[s.alpha] = 0;
  //  std::cout << rho_i << std::endl; 
  //dx
  BgradQ[s.v + 0] = -rho_i*(gradQ[idx(0,s.sigma + 0)] + 2*Q[s.sigma + 0]*alpha_x);   //   AQx(7) = - irho * Qx(1) - 2*Q(1)*irho*Qx(13)        
  BgradQ[s.v + 1] = -rho_i*(gradQ[idx(0,s.sigma + 3)] + 2*Q[s.sigma + 3]*alpha_x);   //   AQx(8) = - irho * Qx(4) - 2*Q(4)*irho*Qx(13)        
  BgradQ[s.v + 2] = -rho_i*(gradQ[idx(0,s.sigma + 4)] + 2*Q[s.sigma + 4]*alpha_x);   //   AQx(9) = - irho * Qx(6) - 2*Q(6)*irho*Qx(13)        
  BgradQ[s.sigma + 0] =-(lambda+2*mu) * ( gradQ[idx(0,s.v + 0)]  - u * alpha_x);   //   AQx(1) = - (lam+2*mu)*Qx(7) + (lam+2*mu)*u(1)*Qx(13) 
  BgradQ[s.sigma + 1] =       -lambda * ( gradQ[idx(0,s.v + 0)]  - u * alpha_x);   //   AQx(2) = - lam*Qx(7)        + lam*u(1)*Qx(13)               
  BgradQ[s.sigma + 2] =       -lambda * ( gradQ[idx(0,s.v + 0)]  - u * alpha_x);   //   AQx(3) = - lam*Qx(7)        + lam*u(1)*Qx(13)               
  BgradQ[s.sigma + 3] =           -mu * ( gradQ[idx(0,s.v + 1)]  - v * alpha_x);   //   AQx(4) = - mu *Qx(8)        + mu *u(2)*Qx(13)               
  BgradQ[s.sigma + 4] =           -mu * ( gradQ[idx(0,s.v + 2)]  - w * alpha_x);   //   AQx(6) = - mu *Qx(9)        + mu *u(3)*Qx(13)               
  BgradQ[s.sigma + 5] =                                                     0.0;   //   AQx(5) =   0.0
  
  //dy                                                                                                                                              
  BgradQ[s.v + 0]+= -rho_i*(gradQ[idx(1,s.sigma + 3)] + 2*Q[s.sigma + 3]*alpha_y);   //   BQy(7) = - irho * Qy(4) - 2*Q(4)*irho*Qy(13)        
  BgradQ[s.v + 1]+= -rho_i*(gradQ[idx(1,s.sigma + 1)] + 2*Q[s.sigma + 1]*alpha_y);   //   BQy(8) = - irho * Qy(2) - 2*Q(2)*irho*Qy(13)        
  BgradQ[s.v + 2]+= -rho_i*(gradQ[idx(1,s.sigma + 5)] + 2*Q[s.sigma + 5]*alpha_y);   //   BQy(9) = - irho * Qy(5) - 2*Q(5)*irho*Qy(13)        
  BgradQ[s.sigma + 0]+=        -lambda * ( gradQ[idx(1,s.v + 1)] - v * alpha_y);   //   BQy(1) = - lam*Qy(8)        + lam*u(2)*Qy(13)           
  BgradQ[s.sigma + 1]+= -(lambda+2*mu) * ( gradQ[idx(1,s.v + 1)] - v * alpha_y);   //   BQy(2) = - (lam+2*mu)*Qy(8) + (lam+2*mu)*u(2)*Qy(13) 
  BgradQ[s.sigma + 2]+=        -lambda * ( gradQ[idx(1,s.v + 1)] - v * alpha_y);   //   BQy(3) = - lam*Qy(8)        + lam*u(2)*Qy(13)           
  BgradQ[s.sigma + 3]+=            -mu * ( gradQ[idx(1,s.v + 0)] - u * alpha_y);   //   BQy(4) = - mu *Qy(7)        + mu *u(1)*Qy(13)           
  BgradQ[s.sigma + 4]+=                                                     0.0;   //   BQy(6) =   0.0                                      
  BgradQ[s.sigma + 5]+=            -mu * ( gradQ[idx(1,s.v + 2)] - w * alpha_y);   //   BQy(5) = - mu *Qy(9)        + mu *u(3)*Qy(13)
                                                                                           //       !                                               
  //dz                                                                                                                                              
  BgradQ[s.v + 0]+= -rho_i*(gradQ[idx(2,s.sigma + 4)] + 2*Q[s.sigma + 4]*alpha_z);   //   CQz(7) = - irho * Qz(6) - 2*Q(6)*irho*Qz(13)        
  BgradQ[s.v + 1]+= -rho_i*(gradQ[idx(2,s.sigma + 5)] + 2*Q[s.sigma + 5]*alpha_z);   //   CQz(8) = - irho * Qz(5) - 2*Q(5)*irho*Qz(13)        
  BgradQ[s.v + 2]+= -rho_i*(gradQ[idx(2,s.sigma + 2)] + 2*Q[s.sigma + 2]*alpha_z);   //   CQz(9) = - irho * Qz(3) - 2*Q(3)*irho*Qz(13)        
  BgradQ[s.sigma + 0]+=        -lambda * ( gradQ[idx(2,s.v + 2)] - w * alpha_z);   //   CQz(1) = - lam*Qz(9)        + lam*u(3)*Qz(13)           
  BgradQ[s.sigma + 1]+=        -lambda * ( gradQ[idx(2,s.v + 2)] - w * alpha_z);   //   CQz(2) = - lam*Qz(9)        + lam*u(3)*Qz(13)           
  BgradQ[s.sigma + 2]+= -(lambda+2*mu) * ( gradQ[idx(2,s.v + 2)] - w * alpha_z);   //   CQz(3) = - (lam+2*mu)*Qz(9) + (lam+2*mu)*u(3)*Qz(13) 
  BgradQ[s.sigma + 3]+=                                                     0.0;   //   CQz(4) =  0.0                                       
  BgradQ[s.sigma + 4]+=            -mu * ( gradQ[idx(2,s.v + 0)] - u * alpha_z);   //   CQz(6) = - mu *Qz(7)        + mu *u(1)*Qz(13)           
  BgradQ[s.sigma + 5]+=            -mu * ( gradQ[idx(2,s.v + 1)] - v * alpha_z);   //   CQz(5) = - mu *Qz(8)        + mu *u(2)*Qz(13)
  
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
