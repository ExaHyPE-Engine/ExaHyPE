# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from functools import lru_cache

import jinja2

from peano4.Logger import log

def write_file(directory, protected_files, full_qualified_filename):
  """
    Decide whether the code generator may (over)write full_qualified_filename.

    The policy is fixed and not user-configurable: every generated file is
    overwritten, except files the user registered as non-generated source files
    via makefile.add_h_file()/add_cpp_file()/add_Fortran_file(). Those belong to
    the user and are never touched.

    directory: project root directory the relative paths are resolved against.
    protected_files: set of normalised relative paths of the user's
      non-generated files.
  """
  relative_filename = os.path.normpath(os.path.relpath(full_qualified_filename, directory))
  if relative_filename in protected_files:
    log.debug("Preserve user-provided (non-generated) file %s", relative_filename)
    return False
  return True


@lru_cache(maxsize=None)
def get_jinja2_template(template_file, undefined=None):
  environment_arguments = {}
  if undefined is not None:
    environment_arguments["undefined"] = undefined

  environment = jinja2.Environment(
    loader=jinja2.FileSystemLoader(searchpath=os.path.dirname(template_file)),
    **environment_arguments,
  )
  return environment.get_template(os.path.basename(template_file))


@lru_cache(maxsize=None)
def _get_jinja2_string_template(template, undefined=None):
  environment_arguments = {}
  if undefined is not None:
    environment_arguments["undefined"] = undefined

  environment = jinja2.Environment(**environment_arguments)
  return environment.from_string(template)


def render_jinja2_template(template, context, undefined=None):
  return _get_jinja2_string_template(template, undefined).render(**context)
