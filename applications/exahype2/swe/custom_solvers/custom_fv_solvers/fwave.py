from custom_solvers import solver

class FWave(solver.Solver):

    def __init__(self, epsilon=1e-4):
        self.epsilon = epsilon
        pass

    def implementation_of_riemann_solver(self):
        return """
    const double g = 9.81;
    double m_h_l = QL[0];
    double m_h_r = QR[0];
    double m_hu_l = QL[normal+1];
    double m_hu_r = QR[normal+1];
    double bat_l = QL[3];
    double bat_r = QR[3];

    if(bat_r >= 0 && bat_l >= 0)
    {
        for(int i = 0; i < 3; i++)
        {
            AMDQ[i] = 0.0;
            APDQ[i] = 0.0;
        }
        return 0.0;
    } 
    else if (bat_r >= 0 && bat_l < 0)
    {
        bat_r = bat_l;
        m_h_r = m_h_l;
        m_hu_r = -m_hu_l;
    }
    else if (bat_l >= 0 && bat_r < 0)
    {
        bat_l = bat_r;
        m_h_l = m_h_r;
        m_hu_l = -m_hu_r;
    }

    double u[2]{m_hu_l / m_h_l, m_hu_r / m_h_r};

    double lambda_roe[2]{0};
    double h_roe = (m_h_l + m_h_r) / 2;
    double u_roe = (u[0] * std::sqrt(m_h_l) + u[1] * std::sqrt(m_h_r)) / (std::sqrt(m_h_l) + std::sqrt(m_h_r));
    lambda_roe[0] = u_roe - std::sqrt(g * h_roe);
    lambda_roe[1] = u_roe + std::sqrt(g * h_roe);

    const double f_q_l[2]{m_hu_l, m_h_l * u[0] * u[0] + 0.5 * g * m_h_l * m_h_l};
    const double f_q_r[2]{m_hu_r, m_h_r * u[1] * u[1] + 0.5 * g * m_h_r * m_h_r};

    double delta_x_psi = -g * (bat_r - bat_l) * (m_h_l + m_h_r) / 2;
    double delta_f[2]{f_q_r[0] - f_q_l[0], f_q_r[1] - f_q_l[1] - delta_x_psi};

    double determinante{1 / (lambda_roe[1] - lambda_roe[0])};
    double deter_mul_del_f_1 = determinante * delta_f[1];
    double deter_mul_del_f_0 = determinante * delta_f[0];
    double alpha[]{lambda_roe[1] * deter_mul_del_f_0 - deter_mul_del_f_1,
                -(lambda_roe[0]) * deter_mul_del_f_0 + deter_mul_del_f_1};

    double z_1[2]{alpha[0], alpha[0] * lambda_roe[0]};
    double z_2[2]{alpha[1], alpha[1] * lambda_roe[1]};

    APDQ[0] = -((lambda_roe[0] > 0 ? z_1[0] : 0) + (lambda_roe[1] > 0 ? z_2[0] : 0));
    APDQ[1] = 0.0;
    APDQ[2] = 0.0;
    APDQ[normal+1] = -((lambda_roe[0] > 0 ? z_1[1] : 0) + (lambda_roe[1] > 0 ? z_2[1] : 0));

    AMDQ[0] = (lambda_roe[0] < 0 ? z_1[0] : 0) + (lambda_roe[1] < 0 ? z_2[0] : 0);
    AMDQ[1] = 0.0;
    AMDQ[2] = 0.0;
    AMDQ[normal+1] = (lambda_roe[0] < 0 ? z_1[1] : 0) + (lambda_roe[1] < 0 ? z_2[1] : 0);

    return std::max(std::abs(lambda_roe[0]), std::abs(lambda_roe[1]));
    """