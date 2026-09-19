# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen


class Steps:
    """The mesh traversals of a project; every step holds the action sets one grid sweep runs."""

    def __init__(self, project):
        self._project = project
        self._steps = []

    def __str__(self):
        return "(#steps=" + str(len(self._steps)) + ")"

    def add_step(self, step):
        step.set_project(self._project)
        self._steps.append(step)

    def construct_output(self, output):
        """Writes every step and the StepRepository that maps a step to its program step."""
        for step in self._steps:
            step.construct_output(output)
        template_prefix = os.path.join(
            os.path.dirname(os.path.realpath(__file__)), "StepsToStepRepository"
        )
        for namespace in self._project.namespaces:
            steps = [
                {"name": step.name, "number": self._steps.index(step) + 1}
                for step in self._steps
                if step.namespace == namespace
            ]
            output.makefile.add_cpp_file(
                "repositories/StepRepository.cpp", generated=True
            )
            output.add(
                exahype2.codegen.TemplatedFile(
                    template_prefix + ".h.jinja2",
                    template_prefix + ".cpp.jinja2",
                    "StepRepository",
                    namespace + ["repositories"],
                    "repositories",
                    {"STEPS": steps},
                )
            )
