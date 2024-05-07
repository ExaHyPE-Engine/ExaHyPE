# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DaStGenToLegacyTool import DaStGenToLegacyTool
from .DoF                 import DoF

import dastgen2

import peano4.dastgen2.MPIAndStorageAspect


from peano4.output.Helper import using_cuda_backend

class DaStGen2Generator(object):
  """! 
  
  Simple generator mapping a DaStGen2 object onto a plain C++ class
  
  """  
  def __init__(self,data):
    self._data = data
    

  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self._data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    return "#include \"peano4/stacks/STDVectorStack.h\" \n \
            #include \"" + self._data.namespace[-1] + "/" + self._data.name + ".h\""


  def construct_output(self,output):
    """
      Pass in a version of output. It is important that external tools are used
      first before we start any compile or so. So I add them first. 
    """
    full_qualified_name = ""
    for i in self._data.namespace:
      full_qualified_name += i
      full_qualified_name += "::"
    full_qualified_name += self._data.name
    self._data.data.set_full_qualified_name(full_qualified_name)

    self._data.data.write_header_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".h" )
    if using_cuda_backend():
      self._data.data.write_implementation_file(  self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cu" )
      output.makefile.add_cpp_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cu", generated=True )
    else:
      self._data.data.write_implementation_file(  self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cpp" )
      output.makefile.add_cpp_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True )



class DaStGen2(DoF):
  """!
  
    Default superclass for any data model in Peano which is stored within the grid
  
    A DaStGen2 data type generator. To add fields to this object, just
    use the DaStGen2 instance data of this field, i.e. data.add_attribute().

    
    ## Attributes
    
    data: dastgen2.DataModel
      Add elements to this guy to enrich your data model.
    
    peano4_mpi_and_storage_aspect: peano4.dastgen2.MPIAndStorageAspect
      This aspect adds the Peano-specific MPI routines to the data type, 
      i.e. routines used for boundary and re-balancing exchange. Modify 
      this one if you want to control certain data exchange or merge
      patterns.


    ## Data storage
    
    I can control when to store data through the peano4_mpi_and_storage_aspect.
   
   
    ## MPI
    
    If you want to add your own MPI merge implementation, you have to 
    alter the attribute peano4_mpi_and_storage_aspect.
    
    ## Arguments
    
    name: String
      Name (unqualified)
      
    
      
  """  
  
  
  readme_descriptor = """
"""

  
  readme_package_descriptor = """
### Data structure modelling

Peano models its data structures via a tool called DaStGen. The actual DaStGen 
version in use is the second generation of DaStGen which is integrated into 
LLVM, e.g. The first generation of DaStGen has been a stand-alone Java tool
which serves as a precompiler. As DaStGen 2 is not published yet, we appreciate 
a citation of the two original DaStGen papers when you discuss the memory needs: 


        @InProceedings{Bungartz:2008:DaStGen,
          author={Bungartz, Hans--Joachim and Eckhardt, Wolfgang and Mehl, Miriam and Weinzierl, Tobias}, "
          editor={Bubak, Marian and van Albada, Geert Dick and Dongarra, Jack and Sloot, Peter M. A.}, 
          title={DaStGen---A Data Structure Generator for Parallel C++ HPC Software}, 
          booktitle={Computational Science -- ICCS 2008}, 
          year=2008,
          publisher={Springer Berlin Heidelberg}, 
          address={Berlin, Heidelberg}, "
          pages={213--222}, 
          isbn={978-3-540-69389-5} 
        }


        @article{Bungartz:2010:DaStGen, 
          title = {A precompiler to reduce the memory footprint of multiscale PDE solvers in C++}, 
          journal = {Future Generation Computer Systems},"
          volume = {26}, 
          number = {1}, 
          pages = {175-182}, 
          year = {2010}, 
          issn = {0167-739X}, 
          doi = {https://doi.org/10.1016/j.future.2009.05.011},
          url = {https://www.sciencedirect.com/science/article/pii/S0167739X09000673},
          author = {H.-J. Bungartz and W. Eckhardt and T. Weinzierl and C. Zenger}, 
          keywords = {Data compaction and compression, Code generation, Multigrid and multilevel methods}, 
          abstract = {A PDE solver's value is increasingly co-determined by its memory footprint, as the increase of computational multicore power overtakes the memory access speed, and as memory restricts the maximum experiment size. Tailoring a code to require less memory is technical challenging, error-prone, and hardware-dependent. Object-oriented code typically consumes much memory, though developers favour such high-level languages offering meaningful models and good maintainability. We augment the language C++ with new keywords branding records to be memory-critical. Our precompiler DaStGen then transforms this augmented specification into plain C++ optimised for low memory requirements. Hereby, it encodes multiple attributes with fixed range within one variable, and it reduces the number of bits per floating point value. The tool also generates one user-defined MPI data type per class and, thus, facilitates the construction of parallel codes with small messages.} 
        } 
"""  
  
  
  def __init__(self, name):
    super(DaStGen2, self).__init__(name)
    self.generator        = DaStGen2Generator(self)
    
    self.data             = dastgen2.DataModel(name)
    
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "debugX", "Dimensions", ifdefs=["PeanoDebug>0"] ) )
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "debugH", "Dimensions", ifdefs=["PeanoDebug>0"] ) )
    
    self.peano4_mpi_and_storage_aspect = peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Undef)

    self.data.add_aspect( self.peano4_mpi_and_storage_aspect )


  def configure(self, namespace, association, subdirectory=""):
    """
    
      I always need the MPI aspect, but I can't add the right one before I don't 
      know whether this DaStGen model is used for vertices, faces or cells. To I 
      hook into this routine. In theory, I could add the DaStGen2 MPI aspect 
      straightaway (in the constructor), but I decided to do so in one place. 
      
      
      Implementation detail
      
      It is important that we add the aspect once when we initialise the code and 
      then reconfigure it. I had the commented out version before, but that one
      adds an aspect everytime configure() is called and thus causes issues as 
      aspects might be added multiple times.
      
    """
    super(DaStGen2, self).configure(namespace,association,subdirectory)
    self.peano4_mpi_and_storage_aspect.dof_association = association
    
        
  @property
  def additional_load_and_store_arguments(self):
    return self._additional_load_and_store_arguments
    

  @additional_load_and_store_arguments.setter
  def additional_load_and_store_arguments(self,new_arguments):
    self.peano4_mpi_and_storage_aspect.additional_load_and_store_arguments = [ (x[1],x[2]) for x in new_arguments]
    for new_argument in new_arguments:
      self.peano4_mpi_and_storage_aspect.includes.append( """
#include \"""" + new_argument[1].replace("::","/") + """.h"
""")
    self._additional_load_and_store_arguments = new_arguments

