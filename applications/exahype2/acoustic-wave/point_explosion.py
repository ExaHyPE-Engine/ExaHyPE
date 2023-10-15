import sys

from exahype2.solvers.PDETerms import PDETerms


class PointExplosion:
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        self.use_symbolic_description = use_symbolic_description
        if use_symbolic_description:
          sys.path.append('../../../tutorials/exahype2/acoustic-wave')
          import sympy
          from pde import PDE
          self.pde = PDE(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

          self.pde.initial_values[0] = 0 # p
          self.pde.initial_values[1] = 0 # u
          self.pde.initial_values[2] = 0 # v
          if dimensions == 3:
            self.pde.initial_values[3] = 0 # w

          self.pde.boundary_values[0] = 0
          self.pde.boundary_values[1] = 0
          self.pde.boundary_values[2] = 0
          if dimensions == 3:
            self.pde.boundary_values[3] = 0

          sigma = sympy.symbols("sigma")
          t0 = sympy.symbols("t0")
          M0 = sympy.symbols("M0")
          t = sympy.symbols("t")

          max_h = sympy.symbols("MaxAdmissibleCellH")
          force = M0 * sympy.exp(-((t - t0) * (t - t0)) / (2.0 * sigma * sigma))

          if dimensions == 2:
              point_source = sympy.sqrt((5 - self.pde.x[0])**2 + (5 - self.pde.x[1])**2)
              self.pde.sources[0] = sympy.Piecewise((force, point_source <= max_h), (0, True)) # p
          else:
              point_source = sympy.sqrt((5 - self.pde.x[0])**2 + (5 - self.pde.x[1])**2 + (5 - self.pde.x[2])**2)
              self.pde.sources[0] = sympy.Piecewise((force, point_source <= max_h), (0, True)) # p

          self.pde.sources[1] = 0
          self.pde.sources[2] = 0
          if dimensions == 3:
            self.pde.sources[3] = 0

          self.pde.substitute_expression(self.pde.rho, 2.7)
          self.pde.substitute_expression(self.pde.c, 6.0)
          self.pde.substitute_expression(sigma, 0.1149)
          self.pde.substitute_expression(t0, 0.7)
          self.pde.substitute_expression(M0, 1000.0)


    def implementation_of_initial_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_initial_conditions()
        else:
            return """
            Q[0] = 0.0;
            Q[1] = 0.0;
            Q[2] = 0.0;
            #if Dimensions == 3
            Q[3] = 0.0;
            #endif
            """


    def implementation_of_boundary_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_boundary_conditions()
        else:
            return """
            Qoutside[0] = 0.0;
            Qoutside[1] = 0.0;
            Qoutside[2] = 0.0;
            #if Dimensions == 3
            Qoutside[3] = 0.0;
            #endif
            """


    def implementation_of_flux(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_flux()
        else:
            return """
            ::exahype2::acousticwave::flux(Q, faceCentre, t, dt, normal, 2.7, 6.0, F);
            """


    def implementation_of_max_eigenvalue(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_max_eigenvalue()
        else:
            return """
            return ::exahype2::acousticwave::maxEigenvalue(Q, faceCentre, t, dt, normal, 6.0);
            """


    def implementation_of_sources(self):
        if self.use_symbolic_description:
          return self.pde.implementation_of_sources()
        else:
            return """
            S[0] = 0.0;
            S[1] = 0.0;
            S[2] = 0.0;
            #if Dimensions == 3
            S[3] = 0.0;
            #endif

            #if Dimensions == 2
            const bool pointSource = tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>({5, 5})) <= MaxAdmissibleCellH;
            #else
            const bool pointSource = tarch::la::norm2(volumeCentre - tarch::la::Vector<Dimensions, double>({5, 5, 5})) <= MaxAdmissibleCellH;
            #endif

            if (pointSource) {
              constexpr double sigma = 0.1149;
              constexpr double t0 = 0.7;
              constexpr double M0 = 1000.0;
              const double force = M0*std::exp(-((t-t0)*(t-t0))/(2.0*sigma*sigma));
              S[0] = force;
            }
            """


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        #if Dimensions==3
        tarch::la::Vector<Dimensions, double> circleCentre = {5, 5, 5};
        #else
        tarch::la::Vector<Dimensions, double> circleCentre = {5, 5};
        #endif

        // This is an example how to implement a priori refinement.
        // If you only have this thing, then you work with static AMR,
        // as you never invoke erase.
        if (tarch::la::equals(t, 0.0)) {
          bool isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < MaxAdmissibleCellH);
          if (isInTheCentre) {
            result = ::exahype2::RefinementCommand::Refine;
          }
        }

        return result;
        """
