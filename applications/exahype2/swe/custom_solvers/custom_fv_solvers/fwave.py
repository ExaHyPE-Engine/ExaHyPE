from custom_solvers import solver

class FWave(solver.Solver):

    def __init__(self, epsilon=1e-2):
        self.epsilon = epsilon
        pass

    def implementation_of_riemann_solver(self):
        return """    
    const double g = 9.81;
    // const bool leftWet = QL[3] < 0.0; 
    const bool leftWet = tarch::la::greater(QL[0], %f);
    // const bool rightWet = QR[3] < 0.0; 
    const bool rightWet = tarch::la::greater(QR[0], %f);
    const bool leftDry = !leftWet;
    const bool rightDry = !rightWet;

    // dry land
    if(leftDry && rightDry)
    {
        for(int i = 0; i < 3; i++)
        {
            AMDQ[i] = 0.0;
            APDQ[i] = 0.0;
        }
        return 0.0;
    }

    const double m_h_l = leftWet * QL[0] + leftDry * QR[0];
    const double m_h_r = rightWet * QR[0] + rightDry * QL[0];
    const double m_hu_l = leftWet * QL[normal+1] - leftDry * QR[normal+1];
    const double m_hu_r = rightWet * QR[normal+1] - rightDry * QL[normal+1];
    const double bat_l = leftWet * QL[3] + leftDry * QR[3];
    const double bat_r = rightWet * QR[3] + rightDry * QL[3];

    const double u[2]{m_hu_l / m_h_l, m_hu_r / m_h_r};
    const double h_roe = (m_h_l + m_h_r) / 2;
    const double u_roe = (u[0] * std::sqrt(m_h_l) + u[1] * std::sqrt(m_h_r)) / (std::sqrt(m_h_l) + std::sqrt(m_h_r));
    const double lambda_roe[2]{u_roe - std::sqrt(g * h_roe), u_roe + std::sqrt(g * h_roe)};

    const double f_q_l[2]{leftWet * FL[0] - leftDry * FR[0], leftWet * FL[normal+1] + leftDry * FR[normal+1]};
    const double f_q_r[2]{rightWet * FR[0] - rightDry * FL[0], rightWet * FR[normal+1] + rightDry * FL[normal+1]};

    const double delta_x_psi = -g * (bat_r - bat_l) * h_roe;
    const double delta_f[2]{f_q_r[0] - f_q_l[0], f_q_r[1] - f_q_l[1] - delta_x_psi};

    const double determinante{1 / (lambda_roe[1] - lambda_roe[0])};
    const double deter_mul_del_f_1 = determinante * delta_f[1];
    const double deter_mul_del_f_0 = determinante * delta_f[0];
    const double alpha[]{lambda_roe[1] * deter_mul_del_f_0 - deter_mul_del_f_1,
                -(lambda_roe[0]) * deter_mul_del_f_0 + deter_mul_del_f_1};

    const double z_1[2]{alpha[0], alpha[0] * lambda_roe[0]};
    const double z_2[2]{alpha[1], alpha[1] * lambda_roe[1]};

    APDQ[0] = -((tarch::la::greater(lambda_roe[0], 0.0) ? z_1[0] : 0) + (tarch::la::greater(lambda_roe[1], 0) ? z_2[0] : 0));
    APDQ[1] = 0.0;
    APDQ[2] = 0.0;
    APDQ[normal+1] = -((tarch::la::greater(lambda_roe[0], 0) ? z_1[1] : 0) + (tarch::la::greater(lambda_roe[1], 0) ? z_2[1] : 0));

    AMDQ[0] = (tarch::la::smaller(lambda_roe[0], 0) ? z_1[0] : 0) + (tarch::la::smaller(lambda_roe[1], 0) ? z_2[0] : 0);
    AMDQ[1] = 0.0;
    AMDQ[2] = 0.0;
    AMDQ[normal+1] = (tarch::la::smaller(lambda_roe[0], 0) ? z_1[1] : 0) + (tarch::la::smaller(lambda_roe[1], 0) ? z_2[1] : 0);

    return std::max(std::abs(lambda_roe[0]), std::abs(lambda_roe[1]));
    """  % (self.epsilon, self.epsilon)