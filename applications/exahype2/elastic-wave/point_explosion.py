import sys

from exahype2.solvers.PDETerms import PDETerms


class PointExplosion:
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        self.use_symbolic_description = use_symbolic_description
        if use_symbolic_description:
          sys.path.append('../../../tutorials/exahype2/elastic-wave')
          import sympy
          from pde import PDE
          self.pde = PDE(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

          self.pde.initial_values[0] = 0
          self.pde.initial_values[1] = 0
          self.pde.initial_values[2] = 0
          self.pde.initial_values[3] = 0
          self.pde.initial_values[4] = 0

          self.pde.boundary_values[0] = 0
          self.pde.boundary_values[1] = 0
          self.pde.boundary_values[2] = 0
          self.pde.boundary_values[3] = 0
          self.pde.boundary_values[4] = 0

          t0 = sympy.symbols("t0")
          M0 = sympy.symbols("M0")
          t = sympy.symbols("t")
          force = M0 * t / (t0 * t0) * (sympy.exp(-t / t0))

          max_h = sympy.symbols("MaxAdmissibleCellH")
          point_source = sympy.sqrt((10 - self.pde.x[0])**2 + (10 - self.pde.x[1])**2)

          self.pde.sources[0] = 0
          self.pde.sources[1] = 0
          self.pde.sources[2] = 0
          self.pde.sources[3] = 0
          self.pde.sources[4] = sympy.Piecewise((force, point_source <= max_h), (0, True))

          self.pde.substitute_expression(self.pde.rho, 2.7)
          self.pde.substitute_expression(self.pde.cp, 6.0)
          self.pde.substitute_expression(self.pde.cs, 3.464)
          self.pde.substitute_expression(t0, 0.1)
          self.pde.substitute_expression(M0, 1000.0)


    def implementation_of_initial_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_initial_conditions()
        else:
            return """
            Q[0] = 0.0;
            Q[1] = 0.0;
            Q[2] = 0.0;
            Q[3] = 0.0;
            Q[4] = 0.0;
            """


    def implementation_of_boundary_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_boundary_conditions()
        else:
            return """
            Qoutside[0] = 0.0;
            Qoutside[1] = 0.0;
            Qoutside[2] = 0.0;
            Qoutside[3] = 0.0;
            Qoutside[4] = 0.0;
            """


    def implementation_of_flux(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_flux()
        else:
            return """
            ::exahype2::elasticwave::flux(Q, faceCentre, t, dt, normal, 2.7, 6.0, 3.464, F);
            """


    def implementation_of_max_eigenvalue(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_max_eigenvalue()
        else:
            return """
            return ::exahype2::elasticwave::maxEigenvalue(Q, faceCentre, t, dt, normal, 6.0);
            """


    def implementation_of_sources(self):
        if self.use_symbolic_description:
          return self.pde.implementation_of_sources()
        else:
            return """
            S[0] = 0.0;
            S[1] = 0.0;
            S[2] = 0.0;
            S[3] = 0.0;
            S[4] = 0.0;

            const bool pointSource = tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>({10, 10})) <= MaxAdmissibleCellH;
            if (pointSource) {
              constexpr double t0 = 0.1;
              constexpr double M0 = 1000.0;
              const double force = M0*t/(t0*t0)*std::exp(-t/t0);
              S[4] = force;
            }
"""


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;
        if (tarch::la::equals(t, 0.0)) {
          const bool pointSource = tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>({10, 10})) <= MaxAdmissibleCellH;
          if (pointSource) {
            result = ::exahype2::RefinementCommand::Refine;
          }
        }
        return result;
        """
