#include "Euler.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"


/*

    double minValues[] =
        {0.01,-std::numeric_limits<double>::max(),-1e-8,-1e-8,-std::numeric_limits<double>::max()};

    double maxValues[] =
        {1.0, std::numeric_limits<double>::max(),  1e-8, 1e-8, std::numeric_limits<double>::max()};

    ::exahype2::fv::validatePatch(
        reconstructedPatch,
        5,
        0,
        4,
        1, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString(),
        false, minValues, maxValues
    ); // previous time step has to be valid






parameter location: observers/TimeStep2peano4_toolbox_blockstructured_ReconstructPatchAndApplyFunctor5.cpp(446):
(x=[0.611111,0.685185],h=[0.037037,0.037037],has-been-refined=0,will-be-refined=0,is-local=1,all-vertices-refined=0,one-vertex-hanging=0,all-vertices-inside-domain=1,no-lb=1,rel-pos=[1,0])
reconstructed-patch=
(d[0]=x,d[1]=x,d[2]=x,d[3]=x,d[4]=x)(d[5]=0.400144,d[6]=0.00239283,d[7]=-6.77626e-21,d[8]=0,d[9]=0.0121948)(d[10]=0.400128,d[11]=0.001724,d[12]=-6.77626e-21,d[13]=0,d[14]=0.00845163)(d[15]=0.400111,d[16]=0.000971439,d[17]=-2.25875e-21,d[18]=0,d[19]=0.00424978)(d[20]=0.400079,d[21]=0.000262441,d[22]=0,d[23]=0,d[24]=0.000516593)(d[25]=x,d[26]=x,d[27]=x,d[28]=x,d[29]=x)(d[30]=0.400175,d[31]=0.00329896,d[32]=1.12938e-20,d[33]=0,d[34]=0.0172435)(d[35]=0.300108,d[36]=0.00179462,d[37]=6.77626e-21,d[38]=0,d[39]=0.00914608)(d[40]=0.300096,d[41]=0.001293,d[42]=6.77626e-21,d[43]=0,d[44]=0.00633872)(d[45]=0.300083,d[46]=0.00072858,d[47]=2.25875e-21,d[48]=0,d[49]=0.00318734)(d[50]=0.300059,d[51]=0.00019683,d[52]=0,d[53]=0,d[54]=0.000387445)(d[55]=0.10001,d[56]=2.15397e-05,d[57]=0,d[58]=0,d[59]=7.37361e-06)(d[60]=0.400175,d[61]=0.00329896,d[62]=0,d[63]=0,d[64]=0.0172435)(d[65]=0.300108,d[66]=0.00179462,d[67]=0,d[68]=0,d[69]=0.00914608)(d[70]=0.300096,d[71]=0.001293,d[72]=-3.76158e-37,d[73]=0,d[74]=0.00633872)(d[75]=0.300083,d[76]=0.00072858,d[77]=0,d[78]=0,d[79]=0.00318734)(d[80]=0.300059,d[81]=0.00019683,d[82]=0,d[83]=0,d[84]=0.000387445)(d[85]=0.10001,d[86]=2.15397e-05,d[87]=0,d[88]=0,d[89]=7.37361e-06)(d[90]=0.400175,d[91]=0.00329896,d[92]=0,d[93]=0,d[94]=0.0172435)(d[95]=0.300108,d[96]=0.00179462,d[97]=0,d[98]=0,d[99]=0.00914608)(d[100]=0.300096,d[101]=0.001293,d[102]=-3.76158e-37,d[103]=0,d[104]=0.00633872)(d[105]=0.300083,d[106]=0.00072858,d[107]=0,d[108]=0,d[109]=0.00318734)(d[110]=0.300059,d[111]=0.00019683,d[112]=0,d[113]=0,d[114]=0.000387445)(d[115]=0.10001,d[116]=2.15397e-05,d[117]=0,d[118]=0,d[119]=7.37361e-06)(d[120]=0.400175,d[121]=0.00329896,d[122]=-2.03288e-20,d[123]=0,d[124]=0.0172435)(d[125]=0.300108,d[126]=0.00179462,d[127]=0,d[128]=0,d[129]=0.00914608)(d[130]=0.300096,d[131]=0.001293,d[132]=1.12938e-21,d[133]=0,d[134]=0.00633872)(d[135]=0.300083,d[136]=0.00072858,d[137]=-2.25875e-21,d[138]=0,d[139]=0.00318734)(d[140]=0.300059,d[141]=0.00019683,d[142]=0,d[143]=0,d[144]=0.000387445)(d[145]=0.10001,d[146]=2.15397e-05,d[147]=0,d[148]=0,d[149]=7.37361e-06)(d[150]=x,d[151]=x,d[152]=x,d[153]=x,d[154]=x)(d[155]=0.400144,d[156]=0.00239283,d[157]=-6.77626e-21,d[158]=0,d[159]=0.0121948)(d[160]=0.400128,d[161]=0.001724,d[162]=-2.71051e-20,d[163]=0,d[164]=0.00845163)(d[165]=0.400111,d[166]=0.000971439,d[167]=-4.51751e-21,d[168]=0,d[169]=0.00424978)(d[170]=0.400079,d[171]=0.000262441,d[172]=0,d[173]=0,d[174]=0.000516593)(d[175]=x,d[176]=x,d[177]=x,d[178]=x,d[179]=x)


parameter plotPatch(Q,unknowns,auxiliaryVariables,numberOfVolumesPerAxisInPatch,haloSize):
(d[0]=0.30015,d[1]=0.0018008,d[2]=1.62502e-06,d[3]=0,d[4]=0.00914808)(d[5]=0.300112,d[6]=0.00129626,d[7]=1.12639e-06,d[8]=0,d[9]=0.00633915)(d[10]=0.300095,d[11]=0.000731813,d[12]=5.66481e-07,d[13]=0,d[14]=0.00318768)(d[15]=0.300056,d[16]=0.000198581,d[17]=6.88676e-08,d[18]=0,d[19]=0.000387747)(d[20]=0.300132,d[21]=0.00180069,d[22]=0,d[23]=0,d[24]=0.00914755)(d[25]=0.300098,d[26]=0.00129619,d[27]=0,d[28]=0,d[29]=0.00633884)(d[30]=0.300085,d[31]=0.000731788,d[32]=0,d[33]=0,d[34]=0.00318757)(d[35]=0.300053,d[36]=0.000198578,d[37]=0,d[38]=0,d[39]=0.000387742)(d[40]=0.300132,d[41]=0.00180069,d[42]=0,d[43]=0,d[44]=0.00914755)(d[45]=0.300098,d[46]=0.00129619,d[47]=0,d[48]=0,d[49]=0.00633884)(d[50]=0.300085,d[51]=0.000731788,d[52]=0,d[53]=0,d[54]=0.00318757)(d[55]=0.300053,d[56]=0.000198578,d[57]=0,d[58]=0,d[59]=0.000387742)(d[60]=0.30015,d[61]=0.0018008,d[62]=-1.62502e-06,d[63]=0,d[64]=0.00914808)(d[65]=0.300112,d[66]=0.00129626,d[67]=-1.12639e-06,d[68]=0,d[69]=0.00633915)(d[70]=0.300095,d[71]=0.000731813,d[72]=-5.66481e-07,d[73]=0,d[74]=0.00318768)(d[75]=0.300056,d[76]=0.000198581,d[77]=-6.88676e-08,d[78]=0,d[79]=0.000387747)



*/


tarch::logging::Log   examples::exahype2::euler::Euler::_log( "examples::exahype2::euler::Euler" );



enum class Scenario {
  PointExplosion,
  PointExplosionWithDynamicAMR,
  BreakingDam,
  BreakingDamWithDynamicAMR,
  BreakingDamResolutionStudies
};


//Scenario scenario = Scenario::BreakingDamResolutionStudies;
Scenario scenario = Scenario::BreakingDamWithDynamicAMR;

//Scenario scenario = Scenario::BreakingDam;



::exahype2::RefinementCommand examples::exahype2::euler::Euler::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  switch (scenario) {
    case Scenario::PointExplosion:
      {
        // This is an example how to implement a priori refinement. If
        // you only have this thing, then you work with static AMR, as
        // you never invoke erase.
        if ( tarch::la::equals(t,0.0) ) {
          tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
          bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::PointExplosionWithDynamicAMR:
      {
        // Same as for static AMR. This is the initial pattern
        if ( tarch::la::equals(t,0.0) ) {
          tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
          bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }

        // This is an example how to implement dynamic AMR, as the
        // AMR instruction depends on the actual solution (which is
        // not directly available throughout the grid construction).
        // If you remove this part, you get static AMR.
        if ( tarch::la::greater(t,0.0) and t<0.0001 ) {
          if ( Q[4]>0.4 ) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if ( Q[4]<0.2 ) {
            result = ::exahype2::RefinementCommand::Erase;
          }
        }
      }
      break;
    case Scenario::BreakingDam:
      {
        if ( volumeCentre(0)<0.5 ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::BreakingDamWithDynamicAMR:
      {
        if ( tarch::la::equals(t,0.0) and tarch::la::equals(volumeCentre(0),1.0/3.0) ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
        else if (t>0.0 and Q[4]>0.4) {
          result = ::exahype2::RefinementCommand::Refine;
        }
        else if (t>0.0 and Q[4]<0.2) {
          result = ::exahype2::RefinementCommand::Erase;
        }
      }
      break;
    case Scenario::BreakingDamResolutionStudies:
      {
        if (
          volumeCentre(1)>0.5-0.5*volumeH(1)*NumberOfFiniteVolumesPerAxisPerPatch
          and
          volumeCentre(1)<0.5+0.5*volumeH(1)*NumberOfFiniteVolumesPerAxisPerPatch
        ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
  }

  return result;
}

void examples::exahype2::euler::Euler::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
  switch (scenario) {
    case Scenario::PointExplosion:
    case Scenario::PointExplosionWithDynamicAMR:
      {
        logDebug( "initialCondition(...)", "set point explosion initial condition" );
        // Manual offset to make the wave originate slightly to the left of the center --- helps
        // to detect if wave is moving to the left or right
        #if Dimensions==2
        tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
        #else
        tarch::la::Vector<Dimensions,double> circleCentre = {0.18,0.3,0.6};
        #endif

        // initial conditions
        bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = isInTheCentre ? 1.0 : 0.0; // inner energy
      }
      break;
    case Scenario::BreakingDam:
    case Scenario::BreakingDamWithDynamicAMR:
    case Scenario::BreakingDamResolutionStudies:
      {
        logDebug( "initialCondition(...)", "set breaking dam initial condition" );
        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = volumeCentre(0)<0.5 ? 1.0 : 0.0; // inner energy
      }
      break;
  }
}


double examples::exahype2::euler::Euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  assertion(normal>=0);
  assertion(normal<Dimensions);
  //assertion( Q[0]>0.0 );

  if (Q[0]<=0.0 or Q[0]!=Q[0]) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density negative" );
    assertion(false);
  }

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion9( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
  nonCriticalAssertion14( result>=0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  return result;
}


void examples::exahype2::euler::Euler::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[5]
)
{
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  assertion4( normal>=0, faceCentre, volumeH, t, normal );
  assertion4( normal<Dimensions, faceCentre, volumeH, t, normal);
  nonCriticalAssertion9( Q[0]==Q[0], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[1]==Q[1], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[2]==Q[2], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[3]==Q[3], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[4]==Q[4], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );

  if (Q[0]<=1e-12  or Q[0]!=Q[0]) {
    std::ostringstream msg;
    msg << "density is negative"
        << ".faceCentre=" << faceCentre
        << ",volumeH=" << volumeH
        << ",normal=" << normal
        << ",Q[0]=" << Q[0];
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", msg.str() );
    assertion(false);
  }

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion( F[0]==F[0] );
  nonCriticalAssertion( F[1]==F[1] );
  nonCriticalAssertion( F[2]==F[2] );
  nonCriticalAssertion( F[3]==F[3] );
  nonCriticalAssertion( F[4]==F[4] );

  const double coeff = irho*Q[normal+1];
  F[0] = coeff*Q[0];
  F[1] = coeff*Q[1];
  F[2] = coeff*Q[2];
  F[3] = coeff*Q[3];
  F[4] = coeff*Q[4];
  F[normal+1] += p;
  F[4]        += coeff*p;

  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
}




void examples::exahype2::euler::Euler::boundaryConditions(
  const double * __restrict__                  Qinside,    // Qinside[5+0]
  double * __restrict__                        Qoutside,   // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[0]==Qinside[0], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[1]==Qinside[1], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[2]==Qinside[2], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[3]==Qinside[3], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[4]==Qinside[4], faceCentre, volumeH, t, normal );

  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );
  assertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

  logTraceOut( "boundaryConditions(...)" );
}




#if defined(OpenMPGPUOffloading)
/**
 * Please ignore everything below if you don't have GPUs
 *
 *
 * This is a lot of code duplication here. Once we enable GPUs, we need a
 * version of the flux and eigenvalue which is a classic member function
 * of the class (as we have it without the GPU support) and we need a
 * version of these routines that does not have a state, i.e. is static.
 *
 * Some explanations for this are given in the autogenerated abstract
 * solver once you switch on GPUs.
 *
 * Now, our static flux function and eigenvalue function does not differ
 * from the non-static one. It thus would be convenient to write the
 * static flavour once and then to make the standard, non-static version
 * call the static one. Indeed, I'd do so in a real application.
 *
 * This however is our standard introductory example where we try to make
 * concepts to users as clear as possible, and it is not clear immediately
 * why you need different versions of one routine. So I prefer to replicate
 * code here. As pointed out: You likely would not do this in a proper app.
 */
#pragma omp declare target
double examples::exahype2::euler::Euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
) {
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double u_n = Q[normal + 1] * irho;
  const double c   = std::sqrt(gamma * p * irho);

  double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
  return result;
}
#pragma omp end declare target


#pragma omp declare target
void examples::exahype2::euler::Euler::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F, // F[5],
 Offloadable
) {
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion( F[0]==F[0] );
  nonCriticalAssertion( F[1]==F[1] );
  nonCriticalAssertion( F[2]==F[2] );
  nonCriticalAssertion( F[3]==F[3] );
  nonCriticalAssertion( F[4]==F[4] );

  const double coeff = irho*Q[normal+1];
  F[0] = coeff*Q[0];
  F[1] = coeff*Q[1];
  F[2] = coeff*Q[2];
  F[3] = coeff*Q[3];
  F[4] = coeff*Q[4];
  F[normal+1] += p;
  F[4]        += coeff*p;
}
#pragma omp end declare target

#endif
