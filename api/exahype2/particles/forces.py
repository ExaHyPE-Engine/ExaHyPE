# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Forces:
    """How a tracer reads the fluid state and moves through it.

    Integration schemes: 0 static, 1 explicit Euler, 2 semi-implicit Euler, 3 velocity Verlet,
    4 midpoint, 5 RK3, 6 RK4. Interpolation: 0 none, 1 piecewise constant, 2 piecewise linear.
    ``force_method`` is a bit mask: 1 central gravity, 2 buoyancy, 4 Stokes drag, 8 constant
    gravity. ``fields_to_interpolate`` lists the particle attributes that receive consecutive
    solver values.
    """

    def __init__(
        self,
        interpolation_scheme=0,
        integration_scheme=0,
        force_method=14,
        fields_to_interpolate="0,3,4",
        mu="10.0",
        gravity_magnitude="1",
        dynamic_viscosity="1",
        update_kernel_arguments=None,
    ):
        self.interpolation_scheme = interpolation_scheme
        self.integration_scheme = integration_scheme
        self.force_method = force_method
        self.fields_to_interpolate = fields_to_interpolate
        self.mu = mu
        self.gravity_magnitude = gravity_magnitude
        self.dynamic_viscosity = dynamic_viscosity
        self.update_kernel_arguments = update_kernel_arguments

    def arguments(self):
        """Returns the keyword arguments of the tracing action set."""
        result = {
            "interpolation_scheme": self.interpolation_scheme,
            "integration_scheme": self.integration_scheme,
            "force_method": self.force_method,
            "fields_to_interpolate": self.fields_to_interpolate,
            "mu": self.mu,
            "gravity_magnitude": self.gravity_magnitude,
            "dynamic_viscosity": self.dynamic_viscosity,
        }
        if self.update_kernel_arguments is not None:
            result["update_kernel_arguments"] = self.update_kernel_arguments
        return result
