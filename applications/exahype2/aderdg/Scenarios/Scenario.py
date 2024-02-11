from exahype2.solvers.PDETerms import PDETerms

class Scenario:

    _equation    = None
    _dimensions  = 2
    _end_time    = 1.0
    _plot_dt     = 0.1
    _offset      = 0.0
    _domain_size = 1.0
    _periodic_bc = False

    def initial_conditions(self):
        return PDETerms.User_Defined_Implementation

    def boundary_conditions(self):
        return (
          "assert(false);" if self._periodic_bc
          else PDETerms.User_Defined_Implementation
        )

    def refinement_criterion(self): 
        return PDETerms.None_Implementation

    def analytical_solution(self):
        return PDETerms.None_Implementation

    def set_global_simulation_parameters(self, project):
        project.set_global_simulation_parameters(
          dimensions = self._dimensions,
          offset  = [self._offset, self._offset, self._offset][0:self._dimensions],
          size    = [self._domain_size, self._domain_size, self._domain_size][0:self._dimensions],
          min_end_time = self._end_time,
          max_end_time = self._end_time,
          first_plot_time_stamp = 0.0,
          time_in_between_plots = self._plot_dt,
          periodic_BC = [self._periodic_bc, self._periodic_bc, self._periodic_bc][0:self._dimensions],
        )