# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from functools import lru_cache

import jinja2

from .Overwrite import Overwrite

from peano4.Logger import log

def write_file(overwrite,overwrite_is_default,full_qualified_filename):
  """
    overwrite is of type Overwrite and is what the user passes in
    overwrite_is_default is a boolean
  """
  if overwrite==Overwrite.Always:
    return True
  elif overwrite==Overwrite.Never and os.path.exists(full_qualified_filename):
    return False
  elif overwrite==Overwrite.Never and not os.path.exists(full_qualified_filename):
    return True
  elif overwrite==Overwrite.Default:
    if not overwrite_is_default and os.path.exists(full_qualified_filename):
      return False
    else:
      return True
  else:
    log.error("Undefined overwrite strategy")
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
