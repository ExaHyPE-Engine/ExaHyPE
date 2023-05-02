import pde, sympy


class Radial_Dam_Break(pde.PDE):
    def __init__(self, dimensions, auxiliary_variables, width):
        super().__init__(dimensions=dimensions, auxiliary_variables=auxiliary_variables)

        dam = sympy.sqrt(
            ((width[0] / 2) - self.x[0]) ** 2 + ((width[1] / 2) - self.x[1]) ** 2
        )

        # (sqrt((x - 500.f) * (x - 500.f) + (y - 500.f) * (y - 500.f)) < 100.f) ? 15.f : 10.0f;
        self.initial_values[0] = sympy.Piecewise(
            (1.1, dam < width[0] / 10), (1.0, True)
        )
        
        self.initial_values[1] = 0.0
        self.initial_values[2] = 0.0
        self.initial_values[3] = -1.0 
        
        pass
