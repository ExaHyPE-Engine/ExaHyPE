# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from .makefile import Makefile
from .readme import Readme


class Output:
    """The artefacts a project generates and the build configuration they compile with."""

    def __init__(self):
        self.artefacts = []
        self.readme = Readme()
        self.makefile = Makefile()

    def add(self, artefact, append=True):
        if append:
            self.artefacts.append(artefact)
        else:
            self.artefacts = [artefact] + self.artefacts

    def generate(self, directory, generated_subdir=None):
        """Writes every artefact and the README into ``generated_subdir`` of ``directory``, or
        into ``directory`` itself, and the build files into ``directory``. A file the makefile
        registers as user source is never overwritten."""
        protected_files = self.makefile.protected_files()
        self.makefile.generate(directory, generated_subdir)

        artefact_directory = directory
        if generated_subdir:
            artefact_directory = os.path.join(directory, generated_subdir)
            os.makedirs(artefact_directory, exist_ok=True)
            with open(os.path.join(artefact_directory, ".gitignore"), "w") as gitignore:
                gitignore.write("*\n")
        for artefact in self.artefacts:
            artefact.generate(artefact_directory, protected_files)
        self.readme.generate(artefact_directory)
