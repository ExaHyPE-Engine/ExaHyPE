# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Forces:
    """How a tracer reads the fluid state and moves through it.

    Integration schemes: 0 static, 1 explicit Euler, 2 semi-implicit Euler, 3 velocity Verlet,
    4 midpoint, 5 RK3, 6 RK4. Interpolation: 0 none, 1 piecewise constant, 2 piecewise linear.
    ``force_method`` is a bit mask: 1 central gravity, 2 buoyancy, 4 Stokes drag, 8 constant
    gravity. ``fields_to_interpolate`` lists the particle data entries that receive the first
    unknowns of the solver in this order. ``mu`` is the gravitational parameter of the central
    gravity, ``gravity_magnitude`` the acceleration of the constant gravity and the buoyancy
    and ``dynamic_viscosity`` the viscosity of the Stokes drag, each a C++ expression.
    """

    INTERPOLATION_SCHEMES = {0: "None", 1: "PiecewiseConstant", 2: "PiecewiseLinear"}
    INTEGRATION_SCHEMES = {
        0: "Static",
        1: "ExplicitEuler",
        2: "SemiImplicitEuler",
        3: "VelocityVerlet",
        4: "Midpoint",
        5: "RungeKutta3",
        6: "RungeKutta4",
    }

    def __init__(
        self,
        interpolation_scheme=0,
        integration_scheme=0,
        force_method=14,
        fields_to_interpolate="0,3,4",
        mu="10.0",
        gravity_magnitude="1",
        dynamic_viscosity="1",
    ):
        self.interpolation_scheme = int(interpolation_scheme)
        self.integration_scheme = int(integration_scheme)
        self.force_method = int(force_method)
        if self.interpolation_scheme not in self.INTERPOLATION_SCHEMES:
            raise ValueError(
                "interpolation_scheme is one of {}.".format(
                    sorted(self.INTERPOLATION_SCHEMES)
                )
            )
        if self.integration_scheme not in self.INTEGRATION_SCHEMES:
            raise ValueError(
                "integration_scheme is one of {}.".format(
                    sorted(self.INTEGRATION_SCHEMES)
                )
            )
        if self.force_method < 0 or self.force_method > 15:
            raise ValueError("force_method is a bit mask between 0 and 15.")
        self.fields_to_interpolate = self._parse_fields(fields_to_interpolate)
        self.mu = str(mu)
        self.gravity_magnitude = str(gravity_magnitude)
        self.dynamic_viscosity = str(dynamic_viscosity)

    @staticmethod
    def _parse_fields(fields_to_interpolate):
        if isinstance(fields_to_interpolate, str):
            entries = [
                entry.strip()
                for entry in fields_to_interpolate.split(",")
                if entry.strip()
            ]
        else:
            entries = list(fields_to_interpolate)
        fields = []
        for entry in entries:
            if isinstance(entry, bool) or int(entry) < 0:
                raise ValueError(
                    "fields_to_interpolate names particle data entries by their index."
                )
            fields.append(int(entry))
        if len(set(fields)) != len(fields):
            raise ValueError("fields_to_interpolate names every data entry once.")
        return fields

    @property
    def interpolated_fields(self):
        """Returns the particle data entries the interpolation scheme writes; the scheme 0
        writes none."""
        if self.interpolation_scheme == 0:
            return []
        return list(self.fields_to_interpolate)

    def dictionary(self):
        """Returns the template entries of the forces."""
        return {
            "INTERPOLATION": self.INTERPOLATION_SCHEMES[self.interpolation_scheme],
            "INTEGRATION": self.INTEGRATION_SCHEMES[self.integration_scheme],
            "FORCE_METHOD": self.force_method,
            "GRAVITATIONAL_PARAMETER": self.mu,
            "GRAVITY_MAGNITUDE": self.gravity_magnitude,
            "DYNAMIC_VISCOSITY": self.dynamic_viscosity,
            "FIELDS": self.interpolated_fields,
        }
