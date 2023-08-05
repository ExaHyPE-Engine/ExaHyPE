from custom_solvers import solver

class HLLEM(solver.Solver):

    def __init__(self, epsilon=1e-4):
        self.epsilon = epsilon
        pass

    def implementation_of_riemann_solver(self):
        return """ 
    double fluxLeft[3]{0.0};
    double fluxRight[3]{0.0};
    double lambdaLeft[3]{0.0};
    double lambdaRight[3]{0.0};

    double h_l = QL[0];
    double hu_l = QL[1];
    double hv_l = QL[2];
    double b_l = QL[3];

    double h_r = QR[0];
    double hu_r = QR[1];
    double hv_r = QR[2];
    double b_r = QR[3];

    if(b_r >= 0 && b_l >= 0)
    {
        for(int i = 0; i < 3; i++)
        {
            AMDQ[i] = 0.0;
            APDQ[i] = 0.0;
        }
        return 0.0;
    } 
    else if (b_r >= 0 && b_l < 0)
    {
        b_r = b_l;
        h_r = h_l;
        hu_r = -hu_l;
        hv_r = -hv_l;
        for(int i = 0; i < 3; i++)
        {
            lambdaLeft[i] = LL[i];
            lambdaRight[i] = -lambdaLeft[i];
            fluxLeft[i] = FL[i];
            fluxRight[i] = fluxLeft[i];
            if(normal == 0 && i == 2 || normal == 1 && i == 1)
                fluxRight[i] *= -1.0;
        }

    }
    else if (b_l >= 0 && b_r < 0)
    {
        b_l = b_r;
        h_l = h_r;
        hu_l = -hu_r;
        hv_l = -hv_r;
        for(int i = 0; i < 3; i++)
        {
            lambdaRight[i] = LR[i];
            lambdaLeft[i] = -lambdaRight[i];
            fluxRight[i] = FR[i];
            fluxLeft[i] = fluxRight[i];
            if(normal == 0 && i == 2 || normal == 1 && i == 1)
                fluxLeft[i] *= -1.0;
        }
    }
    else
    {
        for(int i = 0; i < 3; i++)
        {
            lambdaRight[i] = LR[i];
            lambdaLeft[i] = LL[i];
            fluxRight[i] = FR[i];
            fluxLeft[i] = FL[i];
        }
    }

    const double g = 9.81;
    double smax = 0.0;
    {
        double maxAbsLambda = 0.0;
        for(int i = 0; i < 3; i++)
        {
            maxAbsLambda = std::max(std::abs(lambdaLeft[i]), std::abs(lambdaRight[i]));
            smax = std::max(smax, maxAbsLambda);
        }
    }

    double bm = std::max(b_l, b_r);
    double hRoe = 0.5 * (h_l + h_r);
    double DEta = std::max(h_r + b_r - bm, 0.0) - std::max(h_l + b_l - bm, 0.0);

    double flux[3] = {0.0};
    flux[0] = 0.5 * (fluxLeft[0] + fluxRight[0]) - 0.5 * smax * DEta;
    flux[1] = 0.5 * (fluxLeft[1] + fluxRight[1]) - 0.5 * smax * (hu_r - hu_l);
    flux[2] = 0.5 * (fluxLeft[2] + fluxRight[2]) - 0.5 * smax * (hv_r - hv_l);

    for(int i = 0; i < 3; i++)
    {
        APDQ[i] = flux[i];
        AMDQ[i] = flux[i];
    }

    APDQ[normal + 1] -= 0.5 * g * hRoe * DEta;
    AMDQ[normal + 1] += 0.5 * g * hRoe * DEta;
    return smax;
    """

    def implementation_of_eigenvalues(self):
        return """
    double g = 9.81;
    const double c = std::sqrt(g * Q[0]);
    double u_n = Q[normal+1] * Q[0] *std::sqrt(2)/std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], %f), 4));

    lambda[0] = u_n + c;
    lambda[1] = u_n - c;
    lambda[2] = u_n;
    return;
    """ % self.epsilon

    def implementation_of_flux(self):
        return """
    double u_n = Q[1] * Q[0] *std::sqrt(2)/std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], %f), 4));
    double v_n = Q[2] * Q[0] *std::sqrt(2)/std::sqrt(std::pow(Q[0], 4) + std::pow(std::max(Q[0], %f), 4));
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