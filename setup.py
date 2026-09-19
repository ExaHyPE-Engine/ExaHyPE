# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from setuptools import find_packages, setup

API_ROOT = "api"

api_packages = find_packages(where=API_ROOT)
packages = api_packages + ["postprocessing"]
package_dir = {"": API_ROOT, "postprocessing": "postprocessing"}
package_names = set(packages)


def _package_directory(package):
    if package == "postprocessing":
        return "postprocessing"
    return os.path.join(API_ROOT, *package.split("."))


def _data_files(package):
    """Every non-Python file owned by this package, relative to its directory,
    recursing through plain subdirectories but stopping at nested packages
    (which ship their own data)."""
    base = _package_directory(package)
    patterns = []
    for directory, subdirectories, filenames in os.walk(base):
        relative = os.path.relpath(directory, base)
        dotted = (
            package
            if relative == "."
            else package + "." + relative.replace(os.sep, ".")
        )
        subdirectories[:] = [
            name for name in subdirectories if dotted + "." + name not in package_names
        ]
        for filename in filenames:
            if filename.endswith((".py", ".pyc")):
                continue
            path = os.path.relpath(os.path.join(directory, filename), base)
            patterns.append(path.replace(os.sep, "/"))
    return patterns


package_data = {package: _data_files(package) for package in packages}
package_data = {package: files for package, files in package_data.items() if files}

setup(
    package_dir=package_dir,
    packages=packages,
    package_data=package_data,
)
