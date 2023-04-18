# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re
import subprocess

class InvokeExternalTool(object):
  def __init__(self,instruction_stream,subdirectory):
    """
      call_string    How to invoke tool. The invokation will change into subdirectory
                     prior to the program launch.
    """
    self.instruction_stream = instruction_stream
    self.subdirectory       = subdirectory

  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)

    try:
       print( "invoke " + self.instruction_stream )
       returnCode = subprocess.check_call( self.instruction_stream, stdout=subprocess.PIPE, shell=True, cwd=self.subdirectory )
       if returnCode!=0:
         print( "Error: something went wrong with the program invocation (error code=" + str(returnCode) + ")" )
    except Exception as e:
      print( "Error: "  + str(e) )
