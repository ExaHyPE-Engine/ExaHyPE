#ifndef __TP_BINDDING__
#define __TP_BINDDING__

#include "libtwopunctures/TwoPunctures.h"
//The functions in this namespace works as binddings of two puncture library 

namespace TP_bindding {
	//pre-process, solve the puncture equations
	inline void prepare(TP::TwoPunctures* tp){
	    //first we set the parameter. TODO:find a way to read parameter from python script
		tp->par_b=10.0;
		tp->center_offset[0]=0.0; tp->center_offset[1]=0.0; tp->center_offset[2]=0.0;
		tp->target_M_plus=1.0;//adm mass
		tp->par_P_plus[0]=0.0; tp->par_P_plus[1]=0.0; tp->par_P_plus[2]=0.0;//linear momentum
		tp->par_S_plus[0]=0.0; tp->par_S_plus[1]=0.0; tp->par_S_plus[2]=0.0;//spin
		tp->target_M_minus=1.0;//adm mass
		tp->par_P_minus[0]=0.0; tp->par_P_minus[1]=0.0; tp->par_P_minus[2]=0.0;//linear momentum
		tp->par_S_minus[0]=0.0; tp->par_S_minus[1]=0.0; tp->par_S_minus[2]=0.0; //spin		
		tp->grid_setup_method="evaluation"; //evaluation or Taylor expansion
		tp->TP_epsilon=1e-6;
		
		tp->PrintParameters();
		
		//then solve the equation
		tp->Run();
	}
	
	//calculate the soccz4 quantites use the real physics quantities given in TP lib
	inline void SOCCZ4Cal(double* __restrict__ Q){
		//take care: the metric below are all non-conformal, i.e. without tilde
    	double g_cov[3][3] = { {Q[0], Q[1], Q[2]}, {Q[1], Q[3], Q[4]}, {Q[2], Q[4], Q[5]} };
    	double det = Q[0]*Q[3]*Q[5] - Q[0]*Q[4]*Q[4] - Q[1]*Q[1]*Q[5] + 2*Q[1]*Q[2]*Q[4] -Q[2]*Q[2]*Q[3];
    	double invdet = 1./det;

		double g_contr[3][3] = {
		    { ( Q[3]*Q[5]-Q[4]*Q[4])*invdet, -( Q[1]*Q[5]-Q[2]*Q[4])*invdet, -(-Q[1]*Q[4]+Q[2]*Q[3])*invdet},
		    {-( Q[1]*Q[5]-Q[4]*Q[2])*invdet,  ( Q[0]*Q[5]-Q[2]*Q[2])*invdet, -( Q[0]*Q[4]-Q[2]*Q[1])*invdet},
		    {-(-Q[1]*Q[4]+Q[3]*Q[2])*invdet, -( Q[0]*Q[4]-Q[1]*Q[2])*invdet,  ( Q[0]*Q[3]-Q[1]*Q[1])*invdet}
		};
		
		double phi=std::pow(det,-1./6.);
		double phisq=phi*phi;
		
		double traceK=0;
		double Kex[3][3] = { {Q[6], Q[7], Q[8]}, {Q[7], Q[9], Q[10]}, {Q[8], Q[10], Q[11]} };
		for (int i=0;i<3;i++)
    	for (int j=0;j<3;j++) traceK += g_contr[i][j]*Kex[i][j];
		
		//now adjust the solution
		for (int i=0;i<6;i++) {
			Q[i]=phisq*Q[i]; //\tilde{\gamma}_ij
			Q[i+6]=	phisq*Q[i+6]-1./3. * traceK * Q[i]; //\tilde{A}_ij
		}
		
		Q[53]=traceK; Q[54]=std::log(phi); Q[16]=std::log(Q[16]);
	}

	inline void GradientCal(const double* X, double* __restrict__ Q, double* LgradQ, int nVars, TP::TwoPunctures* tp){
		constexpr double epsilon = 1e-4;
		double Qp1[nVars],Qm1[nVars],Qp2[nVars],Qm2[nVars];
		
		for (int d=0;d<3;d++){
			double xp1[3]={X[0],X[1],X[2]}; double xp2[3]={X[0],X[1],X[2]};
			double xm1[3]={X[0],X[1],X[2]}; double xm2[3]={X[0],X[1],X[2]};
			xp1[d]+=epsilon; xp2[d]+=2*epsilon; xm1[d]-=epsilon; xm2[d]-=2*epsilon;
			
			tp->Interpolate(xp1,Qp1); SOCCZ4Cal(Qp1);
			tp->Interpolate(xp2,Qp2); SOCCZ4Cal(Qp2);
			tp->Interpolate(xm1,Qm1); SOCCZ4Cal(Qm1);
			tp->Interpolate(xm2,Qm2); SOCCZ4Cal(Qm2);
			
			for(int i=0; i<nVars; i++) {
				LgradQ[d*nVars+i] = ( 8.0*Qp1[i] - 8.0*Qm1[i]  + Qm2[i]   - Qp2[i]  )/(12.0*epsilon);
			}
		}
	}
}

#endif /* __TP_BINDDING__ */
