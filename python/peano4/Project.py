# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


import peano4.output
import peano4.datamodel
import peano4.solversteps
import peano4.runner


import subprocess
import sys
import multiprocessing
import gc



class Project (object):
  """ 
  Represents a Peano 4 project.
  
  namespace Sequence of strings representing the (nested) namespace. Pass in 
    [ "examples", "algebraicmg" ] for example if you wanna write a solver that 
    is embedded into the namespace examples::algebraicmg.
    
  """
  
  def __init__(self, namespace, project_name, directory = "."):
    """
      project_name  Simple string. 
    """
    if sys.version_info.major < 3:
      print( "Warning: should be invoked through python3, i.e. with newer Python version" )

    self.namespace    = namespace
    self.directory    = directory
    self.project_name = project_name

    #
    # Create default output model, i.e. those parts that have to be there
    # always
    #
    self.output       = peano4.output.Output()    
    
    #
    # Empty model by default
    #
    self.datamodel    = peano4.datamodel.Model(namespace)
    
    self.solversteps  = peano4.solversteps.Steps(self)
    
    self.main         = peano4.runner.DefaultSequence(self) 
    
    self.is_generated         = False
    self.is_built             = False
    self.build_was_successful = False
    
    self.constants  = peano4.output.Constants(self)


  def cleanup(self):
    """
      This routine has to be called after you've generated your code.
    """
    self.is_generated = False
    self.is_built     = False
    
    self.datamodel.clear()       
    self.solversteps.clear()

    
  def generate(self, overwrite=peano4.output.Overwrite.Default, throw_away_data_after_generation=False):
    """
    Generate all code. If you add stuff to your project after a 
    build, you have to (re-)generate the code. If you compile only
    once, you don't have to invoke this routine explicitly. It is 
    lazily called by the other project routines - the latest before
    you run the code.
    
    It is important that I reset the output. Otherwise, two 
    subsequent generate calls enrich the output twice.
    
    throw_away_data_after_generation: Bool
      The Peano 4 memory footprint can become quite substantial 
      effectively reducing the translation capabilities (as compilers
      tend to require a lot of memory, too). So pass in True if you 
      want the script to throw away the data structures (and run a 
      garbage collection) after all files have been generated. Please
      note that it invalidates both this object (and maybe another 
      object that you've used to generate the present one - such as 
      ExaHyPE). It really strips everything down to the stuff you 
      absolutely need to translate and run the code.
      
    """
    print( "generate all code ..." )
    self.is_generated = True
    self.is_built = False
    if len(self.output.artefacts)>0:
      print( "some artefacts have already been added to repository ... assume this is intentional (by higher abstraction layer, e.g.)")
      #self.output.clear_artefacts()    

    self.datamodel.construct_output(self.output)
    self.solversteps.construct_output(self.output)
    self.main.construct_output(self.output,self.project_name + "-main")
    self.output.generate(overwrite, self.directory)
    self.constants.generate(overwrite, self.directory)

    print( "generation complete" )
    
    if throw_away_data_after_generation:
      self.datamodel = None
      self.solversteps = None
      self.output      = None
      
      gc.collect()
      
      print( "threw away all data and ran garbage collection" )

      
  def build(self, make_clean_first=True, number_of_parallel_builds = -1, additional_libraries = []):
    """
      Invokes the underlying make/C build mechanism on the project. 
      We invoke the make command via a subprocess. That's it.
      
      
      number_of_parallel_builds: int
        This is mapped onto make -jnumber_of_parallel_builds, i.e. it 
        determines how many parallel make instances the code spawns.
        Usually, a lot of the generated code is quite lengthy. Therefore
        compile time can be high. If you pass something smaller or equal
        to zero, it will use the core count as guideline how to compile.
        
    """
    if number_of_parallel_builds<=0:
        number_of_parallel_builds = multiprocessing.cpu_count()
        
    if not self.is_generated:
      self.generate();

    if make_clean_first:
      print( "clean up project ..." )
      try:
        subprocess.check_call(["make", "clean"])
        self.is_built = False
        print( "clean complete" )
      except Exception as e:
        print( "clean failed (" + str(e) + ") - continue anyway" )

    if not self.is_built:
      print( "start to compile with concurrency level of " + str(number_of_parallel_builds) + " ..." )
      try:
        subprocess.check_call(["make", "-j"+str(number_of_parallel_builds)])
        print( "compile complete" )
        self.is_built = True
        self.build_was_successful = True
      except Exception as e:
        print( "compile was not successful: " + str(e) )
        self.is_built = True
        self.build_was_successful = False
    else:
      print( "can not build as code generation has not been successful" )
  
  
  def run(self, args, prefix=None, pipefile=None, rebuild_if_required=True):
    """
    Runs the code. args should be a list of strings or the empty list.
    prefix is an array, too. A typical invocation looks alike

    project.run( ["16.0", "32.0"], ["/opt/mpi/mpirun", "-n", "1"] )
    
    The operation returns True if the run had been successful

    pipefile: string or None

    """
    result = False
    if rebuild_if_required and not self.is_built and not self.build_was_successful:
      self.build()
      
    if not rebuild_if_required or (self.is_built and self.build_was_successful):
      print( "run application ..." )

      invocation  = []
      if prefix!=None:
        invocation += prefix
      invocation += [ "./peano4" ]
      invocation += args

      try:
        if pipefile==None:
          subprocess.check_call( invocation )
        else:
          subprocess.check_call( invocation, stdout=open( pipefile, "w" ) )
          
        print( "run complete" )
        result = True
      except Exception as e:
        print( "run of application was not successful: " + str(e) )
        print( "invocation: " + str(invocation) )
    else:
      print( "can not run as code compilation has not been successful" )
  
    return result
      
      
      
