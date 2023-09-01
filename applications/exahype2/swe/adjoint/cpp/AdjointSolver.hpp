#ifndef applications_exahype2_swe_adjoint_adjoint_solver_h_
#define applications_exahype2_swe_adjoint_adjoint_solver_h_
#include <cmath>
#include "tarch/la/ScalarOperations.h"
namespace applications::exahype2::swe::adjoint
{
    class AdjointSolver
    {
        public:
            inline static void compute_net_updates(double m_h_l, double m_h_r, double m_hu_l, double m_hu_r, double bat_l, double bat_r, double &h_netup_l,
                                                    double &h_netup_r, double &hu_netup_l, double &hu_netup_r, double &maxEdgeSpeed)
            {
                // Steady states && land mass
                if ((tarch::la::equals(m_h_l, m_h_r) && tarch::la::equals(m_hu_l, m_hu_r) && tarch::la::equals(bat_l, bat_r)) || (tarch::la::equals(m_h_l, 0.0) && tarch::la::equals(m_h_r, 0.0)))
                {
                    h_netup_l = 0.0;
                    h_netup_r = 0.0;
                    hu_netup_l = 0.0;
                    hu_netup_r = 0.0;
                    maxEdgeSpeed = 0.0;
                    return;
                }
                // Is left cell wet and right cell dry?
                else if (tarch::la::equals(m_h_r, 0.0) && tarch::la::greater(m_h_l, 0.0))
                {
                    m_h_r = m_h_l;
                    m_hu_r = -m_hu_l;
                    bat_r = bat_l;
                }
                // Is right cell wet and left cell dry?
                else if (tarch::la::equals(m_h_l, 0.0) && tarch::la::greater(m_h_r, 0.0))
                {
                    m_h_l = m_h_r;
                    m_hu_l = -m_hu_r;
                    bat_l = bat_r;
                }

                double h_l_hat = -bat_l;
                double h_r_hat = -bat_r;

                double c_l = sqrt(9.81 * h_l_hat);
                double c_r = sqrt(9.81 * h_r_hat);

                //delta[0] = -hu_r * cR**2 + hu_l * cL**2
                double delta0 = (-m_hu_r * c_r * c_r) + (m_hu_l * c_l * c_l);
                //delta[1] = -(h_r + b_r) + (h_l + b_l)
                double delta1 = -(m_h_r + bat_r) + (m_h_l + bat_l); 

                //beta[0] = (delta[0] + cR * delta[1]) 
                double beta0 = delta0 + c_r * delta1;
                //beta[1] = (-delta[0] + cL * delta[1])
                double beta1 = -delta0 + c_l * delta1;
                if(!tarch::la::equals(c_l + c_r, 0.0))
                {
                    beta0 /= (c_l + c_r);
                    beta1 /= (c_l + c_r);
                }

                maxEdgeSpeed = std::max(std::abs(c_l), std::abs(c_r));
                h_netup_l = c_l * beta0;
                hu_netup_l = beta0;
                h_netup_r = -c_r * beta1;
                hu_netup_r = beta1;
            }
    };
};
#endif