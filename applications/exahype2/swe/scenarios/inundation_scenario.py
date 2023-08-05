import sympy

import scenarios.template_scenario

from abc import abstractmethod

from exahype2.solvers.fv.PDETerms import PDETerms

class Inundation_Scenario(scenarios.template_scenario.Template_Scenario):

    @abstractmethod
    def __init__(self, refinement_tolerance:float, unknowns=3, auxiliary_variables=1, epsilon = 1e-4):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, refinement_tolerance=refinement_tolerance)
        self.epsilon = epsilon

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

    const double bm = std::max(QL[3], QR[3]);
    const double hRoe = 0.5 * (QL[0] + QR[0]);
    const double DEta = std::max(QR[0]+QR[3] - bm, 0.0) - std::max(QL[0]+QL[3] - bm, 0.0);

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
    
    def implementation_of_ncp(self):
        return PDETerms.None_Implementation