from custom_solvers import solver

class HLLEM(solver.Solver):

    def __init__(self, epsilon=1e-2):
        self.epsilon = epsilon
        pass

    def implementation_of_riemann_solver(self):
        return """
    const double g = 9.81;
    double smax = 0.0;
    {
        double maxAbsLambda = 0.0;
        for(int i = 0; i < 3; i++)
        {
            maxAbsLambda = std::max(std::abs(LL[i]), std::abs(LR[i]));
            smax = std::max(smax, maxAbsLambda);
        }
    }

    double bm = std::max(QL[3], QR[3]);
    double hRoe = 0.5 * (QL[0] + QR[0]);
    double DEta = std::max(QR[0]+QR[3] - bm, 0.0) - std::max(QL[0]+QL[3] - bm, 0.0);

    double flux[3] = {0.0};
    flux[0] = 0.5 * (FL[0] + FR[0]) - 0.5 * smax * DEta;
    flux[1] = 0.5 * (FL[1] + FR[1]) - 0.5 * smax * (QR[1] - QL[1]);
    flux[2] = 0.5 * (FL[2] + FR[2]) - 0.5 * smax * (QR[2] - QL[2]);

    APDQ[0] = flux[0];
    AMDQ[0] = flux[0];
    APDQ[1] = flux[1];
    AMDQ[1] = flux[1];
    APDQ[2] = flux[2];
    AMDQ[2] = flux[2];

    APDQ[normal + 1] -= 0.5 * g * hRoe * DEta;
    AMDQ[normal + 1] += 0.5 * g * hRoe * DEta;
    return smax;
    """

    def implementation_of_eigenvalues(self):
        return """
    const double g = 9.81;
    const double c = std::sqrt(g * Q[0]);
    const double u_n = Q[normal+1] * Q[0] * std::sqrt(2) / std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], %f), 4));

    lambda[0] = u_n + c;
    lambda[1] = u_n - c;
    lambda[2] = u_n;

    if(tarch::la::equals(u_n,0.0) && tarch::la::equals(c,0.0))
        lambda[0] = std::sqrt(g * %f);
    return;
    """ % (self.epsilon, self.epsilon)

    def implementation_of_flux(self):
        return """
    const double u_n = Q[1] * Q[0] * std::sqrt(2) / std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], %f), 4));
    const double v_n = Q[2] * Q[0] * std::sqrt(2) / std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], %f), 4));
    switch(normal)
    {
    case 0: 
        F[0] = Q[0] * u_n;
        F[1] = Q[0] * u_n * u_n;
        F[2] = Q[0] * u_n * v_n;
        break;
    case 1:
        F[0] = Q[0] * v_n;
        F[1] = Q[0] * u_n * v_n;
        F[2] = Q[0] * v_n * v_n;
        break;
    }
    return;
    """ % (self.epsilon, self.epsilon)