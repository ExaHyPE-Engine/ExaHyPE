# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import os
import subprocess
import sys
import tempfile

import postprocessing

# The directory that holds the postprocessing package, so a script of the package runs with -m.
PACKAGE_PARENT = os.path.dirname(
    os.path.dirname(os.path.abspath(postprocessing.__file__))
)


class TemporaryRepository:
    """A temporary directory with a clang-format style file, which marks a repository root."""

    def __init__(self):
        self._directory = tempfile.TemporaryDirectory()
        self.root = self._directory.name
        self.write(".clang-format", "BasedOnStyle: LLVM\n")

    def write(self, relative_path, text):
        path = os.path.join(self.root, relative_path)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as file:
            file.write(text)
        return path

    def read(self, relative_path):
        with open(os.path.join(self.root, relative_path), encoding="utf-8") as file:
            return file.read()

    def run(self, module, *arguments):
        """Runs a script of the package on the repository and returns the completed process."""
        search_path = [
            PACKAGE_PARENT,
            *filter(None, os.environ.get("PYTHONPATH", "").split(os.pathsep)),
        ]
        environment = dict(
            os.environ,
            PYTHONPATH=os.pathsep.join(search_path),
            PYTHONDONTWRITEBYTECODE="1",
        )
        return subprocess.run(
            [
                sys.executable,
                "-m",
                f"postprocessing.{module}",
                "--root",
                self.root,
                *arguments,
            ],
            capture_output=True,
            text=True,
            check=False,
            env=environment,
            cwd=self.root,
        )

    def cleanup(self):
        self._directory.cleanup()
