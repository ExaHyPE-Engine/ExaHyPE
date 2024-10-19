# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum

class CompileMode(Enum):
  """!
  
    Compile modi
    
    The compile mode is required in my get_library() routines. It is also used
    in the Makefile template. You can use CompileModes for a list (overview) of
    all available modes. string_to_mode() finally helps you to translate a 
    string representation into an actual modus.
    
  """
  Debug   = 0
  Trace   = 1
  Asserts = 2
  Release = 3
  Stats   = 4


"""!

Overview over all compile modes available

Is often used within the argument parser:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.add_argument("-m", "--mode", dest="mode", choices=peano4.output.CompileModes, default=peano4.output.CompileModes[0], help="|".join(peano4.output.CompileModes) )
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
CompileModes = [ "Release", "Stats", "Trace", "Asserts", "Debug" ]



"""!

Convert a string into a compile mode

"""
def string_to_mode(mode: CompileModes):
    if mode==CompileModes[0]:
        return CompileMode.Release
    if mode==CompileModes[1]:
        return CompileMode.Stats
    if mode==CompileModes[2]:
        return CompileMode.Trace
    if mode==CompileModes[3]:
        return CompileMode.Asserts
    if mode==CompileModes[4]:
        return CompileMode.Debug
    raise Exception( "mode {} not supported".format(mode) )
