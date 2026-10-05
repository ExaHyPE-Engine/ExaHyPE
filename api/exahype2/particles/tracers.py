# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math

from .forces import Forces


class Tracers:
    """The tracer particles one solver moves through its solution.

    ``particle_attributes`` is the number of data entries per particle, or a dictionary whose
    ``unknowns`` entry maps names to entry counts. The grid initialisation inserts every
    particle once, on the rank that owns the cell at its position.
    """

    def __init__(self, name, particle_attributes):
        self._name = name
        if isinstance(particle_attributes, dict):
            self._number_of_data = sum(particle_attributes["unknowns"].values())
        elif isinstance(particle_attributes, int) and not isinstance(
            particle_attributes, bool
        ):
            self._number_of_data = particle_attributes
        else:
            raise TypeError(
                "A tracer set expects an integer or a dictionary as particle_attributes."
            )
        if self._number_of_data < 0:
            raise ValueError(
                "A tracer set expects a non-negative number of data entries."
            )
        self._coordinates = []
        self._coordinate_transform = None
        self._data = []
        self._files = []
        self._database = None
        self._error = None
        self._face_traces = None
        self._solver = None
        self._forces = Forces()
        self._initialisation = True
        self._time_steps = True

    @property
    def name(self):
        return self._name

    @property
    def number_of_data(self):
        return self._number_of_data

    @property
    def solver(self):
        return self._solver

    @property
    def forces(self):
        return self._forces

    @property
    def error_measurement(self):
        return self._error

    @property
    def reads_a_file(self):
        return bool(self._files)

    @property
    def records_a_database(self):
        return self._database is not None

    def insert_by_coordinates(self, coordinates, transform=None):
        """Adds one particle with zero data per coordinate. A coordinate has at least one
        component per axis of the build. ``transform`` is a C++ callable of the solver that maps
        every coordinate to the particle position."""
        for coordinate in coordinates:
            entries = [float(component) for component in coordinate]
            if len(entries) < 2:
                raise ValueError("A tracer coordinate needs at least two components.")
            self._coordinates.append(entries)
        if transform is not None:
            if self._coordinate_transform not in (None, str(transform)):
                raise ValueError(
                    "The tracer set {} maps its coordinates through one transform only.".format(
                        self._name
                    )
                )
            self._coordinate_transform = str(transform)
        return self

    def sample_face_traces(self, normal, position, difference_fields=()):
        """Writes the mean of the two solution traces on its face into every particle on the
        plane at ``position`` along the axis ``normal``. For the unknowns in
        ``difference_fields``, a particle receives the trace of the upper cell minus the trace
        of the lower cell. ``normal`` and ``position`` are C++ expressions."""
        self._face_traces = {
            "NORMAL": str(normal),
            "POSITION": str(position),
            "DIFFERENCE_FIELDS": list(difference_fields),
        }
        return self

    @property
    def samples_face_traces(self):
        return self._face_traces is not None

    def insert_with_data(self, values, unknowns_per_particle=None):
        """Adds particles with data. ``values`` is a C++ initialiser list or a list of lists,
        with the coordinates of each particle followed by its data entries."""
        if (
            unknowns_per_particle is not None
            and int(unknowns_per_particle) != self._number_of_data
        ):
            raise ValueError(
                "The tracer set {} stores {} data entries per particle (got {}).".format(
                    self._name, self._number_of_data, unknowns_per_particle
                )
            )
        if isinstance(values, str):
            self._data.append(values.strip())
        else:
            for particle in values:
                self._data.append([float(value) for value in particle])
        return self

    def insert_from_file(
        self,
        filename,
        scale_factor=1.0,
        save_initial_position=False,
        initial_position_index=(1, 2, 3),
    ):
        """Adds one particle per line of a coordinate file. ``save_initial_position`` copies
        the coordinates into the data entries at ``initial_position_index``."""
        indices = [int(index) for index in initial_position_index]
        if len(indices) not in (2, 3):
            raise ValueError("The initial position needs one data index per axis.")
        if save_initial_position and max(indices) >= self._number_of_data:
            raise ValueError(
                "The tracer set {} needs a data entry {} for the initial position.".format(
                    self._name, max(indices)
                )
            )
        self._files.append(
            {
                "FILENAME": str(filename),
                "SCALE": repr(float(scale_factor)),
                "SAVE_INITIAL_POSITION": bool(save_initial_position),
                "INITIAL_POSITION_INDEX": indices,
            }
        )
        return self

    def dump_into_database(
        self,
        filename,
        position_delta_between_two_snapshots=1e-8,
        time_delta_between_two_snapshots=0,
        output_precision=8,
        use_relative_deltas=False,
        initial_record_time=-1,
        last_record_time=1e8,
    ):
        """Appends a snapshot of a particle to a CSV file after a trace once the particle moved
        further than the position delta or the time delta passed since its last snapshot. The
        database records only between ``initial_record_time`` and ``last_record_time``, both
        included."""
        self._database = {
            "FILENAME": str(filename),
            "POSITION_DELTA": repr(float(position_delta_between_two_snapshots)),
            "TIME_DELTA": repr(float(time_delta_between_two_snapshots)),
            "OUTPUT_PRECISION": int(output_precision),
            "USE_RELATIVE_DELTAS": "true" if use_relative_deltas else "false",
            "INITIAL_RECORD_TIME": repr(float(initial_record_time)),
            "LAST_RECORD_TIME": repr(float(last_record_time)),
        }
        return self

    def attach(self, solver, forces, initialisation, time_steps):
        """Binds the tracers to the solver that moves them."""
        if self._solver is not None and self._solver is not solver:
            raise ValueError(
                "The tracer set {} follows one solver only.".format(self._name)
            )
        if forces is not None and not isinstance(forces, Forces):
            raise TypeError(
                "A tracer set expects forces of the type exahype2.particles.Forces."
            )
        if self._face_traces is not None and forces is not None:
            raise ValueError(
                "The tracer set {} samples face traces and cannot take forces.".format(
                    self._name
                )
            )
        self._solver = solver
        self._forces = forces if forces is not None else Forces()
        self._initialisation = bool(initialisation)
        self._time_steps = bool(time_steps)
        for field in self._forces.interpolated_fields:
            if field >= self._number_of_data:
                raise ValueError(
                    "The tracer set {} needs a data entry {} to interpolate into.".format(
                        self._name, field
                    )
                )

    def measure_error(self, file, every):
        if (
            isinstance(every, bool)
            or not isinstance(every, (int, float))
            or not math.isfinite(every)
            or every < 0
        ):
            raise ValueError(
                "The tracer set {} expects a finite non-negative time for every (got {!r}).".format(
                    self._name, every
                )
            )
        self._error = {"FILE": str(file), "EVERY": repr(float(every))}

    def _rendered_data(self, dimensions):
        """Returns the C++ initialiser of the particles with data, or None without them."""
        if not self._data:
            return None
        rows = []
        for particle in self._data:
            if isinstance(particle, str):
                rows.append(particle.strip().strip("{}").strip())
                continue
            expected = dimensions + self._number_of_data
            if len(particle) != expected:
                raise ValueError(
                    "The tracer set {} expects {} values per particle (got {}).".format(
                        self._name, expected, len(particle)
                    )
                )
            rows.append(", ".join(repr(value) for value in particle))
        return "{" + ", ".join(row for row in rows if row) + "}"

    def dictionary(self, dimensions):
        """Returns the template entries of the tracers for a build of ``dimensions``."""
        if self._solver is None:
            raise ValueError(
                "The tracer set {} needs a solver that traces it.".format(self._name)
            )
        for coordinate in self._coordinates:
            if len(coordinate) < dimensions:
                raise ValueError(
                    "The tracer set {} needs {} components per coordinate.".format(
                        self._name, dimensions
                    )
                )
        for file in self._files:
            if len(file["INITIAL_POSITION_INDEX"]) < dimensions:
                raise ValueError(
                    "The tracer set {} needs {} initial position indices.".format(
                        self._name, dimensions
                    )
                )
        return {
            "NAME": self._name,
            "NUMBER_OF_DATA": self._number_of_data,
            "FORCES": self._forces.dictionary(),
            "FIELDS": self._forces.dictionary()["FIELDS"],
            "INITIALISATION": self._initialisation,
            "TIME_STEPS": self._time_steps,
            "COORDINATES": [
                [repr(component) for component in coordinate[:dimensions]]
                for coordinate in self._coordinates
            ],
            "COORDINATE_TRANSFORM": self._coordinate_transform,
            "FACE_TRACES": (
                None
                if self._face_traces is None
                else dict(
                    self._face_traces,
                    DIFFERENCE_FIELDS=self._solver.state.indices_of(
                        self._face_traces["DIFFERENCE_FIELDS"]
                    ),
                )
            ),
            "DATA": self._rendered_data(dimensions),
            "FILES": [
                dict(
                    file,
                    INITIAL_POSITION_INDEX=file["INITIAL_POSITION_INDEX"][:dimensions],
                )
                for file in self._files
            ],
            "DATABASE": self._database,
            "ERROR": self._error,
        }
