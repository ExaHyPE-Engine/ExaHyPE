# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile
from .CompileMode import CompileMode


import os
import re


try:
  import jinja2
except:
  print( "Warning: jinja2 not available, i.e. you will have very limited API support (such as visualisation only)")


class Makefile(object):
  """ Represents the created makefile of a Peano4 project """
  default_overwrite = True


  def __init__(self):
    self.clear()


  def clear(self):
    self.configure_call = ""
    self.d = {}
    self.d["CXX"]              = ""
    self.d["CXXFLAGS"]         = ""
    self.d["FC"]               = ""
    self.d["FCFLAGS"]          = ""
    self.d["LDFLAGS"]          = ""
    self.d["GPUOBJS"]          = ""
    self.d["LIBS"]             = ""
    self.d["DIM"]              = "2"
    self.d["CONFIGUREPATH"]    = "."
    self.d["EXECUTABLENAME"]   = "peano4"
    self.d["FORTRAN_MODULES"]  = []
    self.set_mode( CompileMode.Debug )
    self.clear_files()


  def set_executable_name(self, fname):
      self.d["EXECUTABLENAME"] = fname


  def clear_files(self):
    self.cppfiles     = []
    self.fortranfiles = []


  def set_dimension(self,dimension):
    self.d["DIM"] = str(dimension)


  def get_configure_path(self):
    """
    Returns the directory where the ./configure script is located
    """
    return self.d["CONFIGUREPATH"]


  def get_source_path(self):
    """
    Returns the directory where the ./configure script is located
    """
    return self.get_configure_path() + "/src"


  def add_header_search_path(self, path ):
    """
     Add the header search path to both the C++ and the Fortran
     call command.
    
    """
    self.d["CXXFLAGS"] += " -I" + path
    self.d["FCFLAGS"]  += " -I" + path


  def add_gpu_object(self, object_path):
    """
    This is a temporary fix for GPU offloading with OpenMP and LLVM and will
    be required ntil linking to static libraries is supported. We need to
    collect all object files that contain offload instructions and later
    add them to the linker command. NOTE: the order matters i.e. the
    objects must be left of the static libs later.
    """
    self.d["GPUOBJS"] += " {}".format(object_path)


  def add_library(self, library_name, library_path="" ):
    """
    If you want to link against a library from Peano, feel free to use
    get_Peano4_source_directory() and hand in a concatenation of this
    path plus a subpath. Otherwise, specify the absolute path where to
    search for. By default, Peano's src directory is in the search
    path.

    A popular invocation including one of Peano's toolboxes is

    project.output.makefile.add_library( "ToolboxFiniteElements2d_trace", project.output.makefile.get_source_path() + "/toolbox/finiteelements" )
    
    """
    if library_path!="":
      self.d["LIBS"] = "-L" + library_path + " " + self.d["LIBS"]
    self.d["LIBS"] += " -l" + library_name + " "


  def set_mode( self, mode ):
    """
      mode should be of type CompileMode. Pass in

      peano4.output.CompileMode.Debug

      for example. Debug is the default.
    """
    if mode==CompileMode.Debug:
      self.d["MODE"]  = "DEBUG"
    elif mode==CompileMode.Asserts:
      self.d["MODE"]  = "ASSERTS"
    elif mode==CompileMode.Stats:
      self.d["MODE"]  = "STATS"
    elif mode==CompileMode.Trace:
      self.d["MODE"]  = "TRACE"
    elif mode==CompileMode.Release:
      self.d["MODE"]  = "RELEASE"
    else:
      assert(False)


  def set_CXX_compiler(self,value):
    self.d["CXX"]           = value


  def set_CXX_flags(self,value):
    self.d["CXXFLAGS"]      = value


  def add_CXX_flag(self,value,force=False):
    if value in self.d["CXXFLAGS"] and not force:
      print( "CXXFLAG " + value + " is already in list of flags. Ignored as force attribute is not set" )
    else:
      self.d["CXXFLAGS"]     += " " + value


  def set_Fortran_compiler(self,value):
    self.d["FC"]           = value


  def set_Fortran_flags(self,value):
    self.d["FCFLAGS"]       = value


  def add_Fortran_flag(self,value,force=False):
    if value in self.d["FCFLAGS"] and not force:
      print( "FCFLAGS " + value + " is already in list of flags. Ignored as force attribute is not set" )
    else:
      self.d["FCFLAGS"]      += " " + value


  def add_linker_flag(self,value):
    self.d["LDFLAGS"]      += " " + value


  def set_linker_flags(self,value):
    self.d["LDFLAGS"]       = value + " "


  def parse_configure_script_outcome(self,directory):
    """

    directory should point to the directory which holds the ./configure script.
    It furthermore has to be invoked after configure has passed successfully.
    This script does not accept relative paths. I then search for the subdirector
    src and parse the Makefile there.
    
    """
    self.d[ "CONFIGUREPATH" ] = directory

    input_file = directory + "/config.log"
    try:
      input = open( input_file, "r" )
      self.configure_call = ""
      print( "parse configure outcome " + input_file + " to extract configure settings" )
      for line in input:
        if "./configure" in line and self.configure_call == "":
          print( "found the configure call info " + line )
          self.configure_call = line.strip()
    except IOError:
      print( """
Error: if you call parse_configure_script_outcome(), please hand over directory where
./configure had been called. You passed """ + directory + """ and the script therefore
did search for a file """ + input_file )



    input_file = directory + "/src/Makefile"
    try:
      input = open( input_file, "r" )
      print( "parse configure outcome " + input_file + " to extract compile settings" )
      
      MakefileConstants = [
        "CXXFLAGS_PEANO_2D_RELEASE",
        "CXXFLAGS_PEANO_2D_STATS",
        "CXXFLAGS_PEANO_2D_ASSERTS",
        "CXXFLAGS_PEANO_2D_TRACE",
        "CXXFLAGS_PEANO_2D_DEBUG",
        "CXXFLAGS_PEANO_3D_RELEASE",
        "CXXFLAGS_PEANO_3D_STATS",
        "CXXFLAGS_PEANO_3D_ASSERTS",
        "CXXFLAGS_PEANO_3D_TRACE",
        "CXXFLAGS_PEANO_3D_DEBUG",
        "LDFLAGS_PEANO_RELEASE",
        "LDFLAGS_PEANO_STATS",
        "LDFLAGS_PEANO_ASSERTS",
        "LDFLAGS_PEANO_TRACE",
        "LDFLAGS_PEANO_DEBUG",
        "LDADD_PEANO_2D_RELEASE",
        "LDADD_PEANO_2D_STATS",
        "LDADD_PEANO_2D_ASSERTS", 
        "LDADD_PEANO_2D_TRACE",
        "LDADD_PEANO_2D_DEBUG",
        "LDADD_PEANO_3D_RELEASE",
        "LDADD_PEANO_3D_STATS",
        "LDADD_PEANO_3D_ASSERTS", 
        "LDADD_PEANO_3D_TRACE",
        "LDADD_PEANO_3D_DEBUG",
        "CXX",
        "FC",
        "CXXFLAGS",
        "LDFLAGS"
      ]
      
      for line in input:
        for constant in MakefileConstants: 
          if re.search( constant + " *=", line) and line.startswith( constant ): 
            try:
              flags = line.split("=",1)[1].strip()
              print( "add " + constant + "=" + flags )
              self.d[constant] = flags
            except:
              print( "Error in " + line + " for token " + constant )

      # A posteriori fix for openmp flag propagation
      if "-fopenmp-targets" in self.d["CXXFLAGS"]:
          val = self.d["CXXFLAGS"].split("-fopenmp-targets=")[-1].split()[0]
          new_entry = " -fopenmp -fopenmp-targets={} ".format(val)
          print( "used OpenMP GPU offloading. Augment linker with " + new_entry )
          self.d["LDFLAGS"] += new_entry
      elif "-fopenmp" in self.d["CXXFLAGS"]:
          #val = self.d["CXXFLAGS"].split("-fopenmp")[-1].split()[0]
          new_entry = " -fopenmp "
          print( "used OpenMP tasking backend. Augment linker with " + new_entry )
          self.d["LDFLAGS"] += " -fopenmp "
          #self.d["LDFLAGS"] += " -fopenmp ".format(val)

    except IOError:
      print( """
Error: if you call parse_configure_script_outcome(), please hand over directory where
./configure had been called. You passed """ + directory + """ and the script therefore
did search for a file """ + input_file )



  def add_cpp_file(self,filename):
    """

     Add a new filename. This is basically a set implementation, i.e. you can
     add files multiple times, but they are not inserted multiple times. This
     is important, as the steps add the cpp files. Multiple steps can hold the
     same action, so this action would be created multiple times.

     All the standard Peano 4 routines rely on this function to add their
     generated files to the build environment. Nothing stops you however to
     add more files yourself.

    """
    if self.cppfiles.count(filename)==0:
      self.cppfiles.append( filename )


  def add_Fortran_file(self,filename):
    """

     Add a new Fortran file.

     All the standard Peano 4 routines rely on this function to add their
     generated files to the build environment. Nothing stops you however to
     add more files yourself. Don't add a file multiple times. This might
     break the compiler.

     Fortran is really picky about the translation order. So you have to add
     the stuff in the right order. Otherwise, Fortran might complain. This is
     your responsibility.

     If your file defines a module, please do not use this routine, but use
     add_Fortran_module() instead.

     filename: String
       Filename of a Fortran file. They usually should have the .f90 extension.

    """
    if self.fortranfiles.count(filename)==0:
      self.fortranfiles.append( filename )


  def add_Fortran_files(self,filenames):
    """

     Calls add_Fortran_file() for each file.

     filenames: [String]
       List of files.

    """
    for i in filenames:
      self.add_Fortran_file(i)


  def add_Fortran_module(self,module_file,force=False):
    """

     Add a Fortran module

     module_file: String
       Filename of a Fortran source code file which hosts a module. It should
       have the extension .f90 or similar.
       
     force: Boolean
       Enforce that Fortran module is added even though it might already be in
       the list.

    """
    if not module_file.endswith( ".f90" ):
      print( "Warning: Fortran module file does not have extension .f90 (" + module_file + ") and translation thus might fail" )
    if module_file in self.d["FORTRAN_MODULES"] and not force:
      print( """Fortran module file 
""" + module_file + """
is already in module file list. Did not add it once more. You can overwrite 
this default behaviour via the force attribute in add_Fortran_module(). If 
you create multiple Peano 4 makefiles in a row (as you change parameters, e.g.)
then this message can typically be ignored.
""" )
    elif module_file in self.d["FORTRAN_MODULES"] and force:
      print( "Fortran module file " + module_file + " is already in module file list but force flag is set. Add it" )
      self.d["FORTRAN_MODULES"].append( module_file )
    else:
      self.d["FORTRAN_MODULES"].append( module_file )


  def add_Fortran_modules(self,module_files):
    for i in module_files:
      self.add_Fortran_module( i )


  def generate(self,overwrite,directory):
    filename = directory + "/Makefile";
    if writeFile(overwrite,self.default_overwrite,filename):
      print( "write " + filename )

      # files
      self.d[ 'CXX_SOURCES' ]     = self.cppfiles

      self.d[ 'FORTRAN_SOURCES' ] = self.fortranfiles


      template_file   = os.path.realpath(__file__).replace( ".pyc", ".template" ).replace( ".py", ".template" )
      template_loader = jinja2.FileSystemLoader(searchpath=os.path.split(template_file)[0])
      templateEnv     = jinja2.Environment(loader=template_loader)
      template        = templateEnv.get_template( os.path.split(template_file)[1] )


      # We first eliminate the precompiled variant, and then we get rid of the
      # postfix in the case where it is a source file
      with open( filename, "w" ) as output:
        output.write( template.render(self.d) )

