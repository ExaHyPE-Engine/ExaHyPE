# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from functools import lru_cache

import jinja2

from peano4.Logger import log


def write_file(directory, protected_files, fully_qualified_filename):
    """Return whether generation may write the requested file.

    Generated files are overwritten unless their normalised path, relative to
    ``directory``, is registered as protected user source.
    ``Makefile.add_h_file()``, ``add_cpp_file()``, and ``add_Fortran_file()``
    register non-generated files for this protection.
    """
    relative_filename = os.path.normpath(
        os.path.relpath(fully_qualified_filename, directory)
    )
    if relative_filename in protected_files:
        log.debug("Preserve user-provided file %s", relative_filename)
        return False
    return True


def _environment_arguments(undefined):
    result = {"keep_trailing_newline": True}
    if undefined is not None:
        result["undefined"] = undefined
    return result


@lru_cache(maxsize=None)
def get_jinja2_template(template_file, undefined=None):
    environment = jinja2.Environment(
        loader=jinja2.FileSystemLoader(searchpath=os.path.dirname(template_file)),
        **_environment_arguments(undefined),
    )
    return environment.get_template(os.path.basename(template_file))


@lru_cache(maxsize=None)
def _get_jinja2_string_template(template, undefined=None):
    environment = jinja2.Environment(**_environment_arguments(undefined))
    return environment.from_string(template)


def render_jinja2_template(template, context, undefined=None):
    return _get_jinja2_string_template(template, undefined).render(**context)
