# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from .Overwrite import Overwrite

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
    print("Error: undefined overwrite strategy")
    return True
