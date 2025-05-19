# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DaStGenToLegacyTool import DaStGenToLegacyTool
from .DoF                 import DoF

import dastgen2

import peano4.dastgen2.MPIAndStorageAspect

import copy



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
    """!
  
    Finally construct the output
  
    Pass in an instance of the output object. We use this output 
    object only to register the generated C++ file with the build
    environment.
    
    """
    full_qualified_name = ""
    for i in self._data.namespace:
      full_qualified_name += i
      full_qualified_name += "::"
    full_qualified_name += self._data.name
    self._data.data.set_full_qualified_name(full_qualified_name)

    self._data.data.write_header_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".h" )
    self._data.data.write_implementation_file(  self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cpp" )
    output.makefile.add_cpp_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True )



class DaStGen2GeneratorForObjectsWithSmartPointers(object):
    """! 
      
    Simple generator mapping a DaStGen2 object onto a plain C++ class
    
    The generator is very similar to DaStGen2Generator, but it has to take
    extra care as we assume that some objects host smart pointers. Therefore,
    we do two things: We dump the smart pointer version, and we create a 
    flattene version, too.
      
    """  
    def __init__(self,data):
        self._data = data
        

    def get_stack_container(self):
        return "peano4::stacks::STDVectorStackOverObjectsWithSmartPointers< " + self._data.get_full_qualified_type() + " >";

        
    def get_header_file_include(self):
        return "#include \"peano4/stacks/STDVectorStackOverObjectsWithSmartPointers.h\" \n \
                #include \"" + self._data.namespace[-1] + "/" + self._data.name + ".h\""


    def construct_output(self,output):
        """!
      
        Finally construct the output
      
        Pass in an instance of the output object. We use this output 
        object only to register the generated C++ file with the build
        environment. This part is exactly the same as for a plain 1:1
        translation.
        
        Further to the default generator, this routine does a little bit
        more:
        
        - Make a deep copy of the data object and replace each instance of
          a smart pointer with its plain counterpart. This new data object
          is identified by the extension _Flattened.
        - Make the new helper data structure include the original class with
          the smart pointers.
        - Declare this one as an embedded type via using.
        - Add a new copy constructor/conversion routine which really just maps 
          the two data types onto each other by using the setters and getters.
        
        """
        full_qualified_name = ""
        for i in self._data.namespace:
          full_qualified_name += i
          full_qualified_name += "::"
        full_qualified_name += self._data.name


        flattened_data_object = copy.copy(self._data.data)
        old_attributes        = flattened_data_object._attributes
        flattened_data_object._attributes = []
        copy_statements               = ""
        for attribute in old_attributes:
            if isinstance(attribute,peano4.dastgen2.Peano4SmartPointerDoubleArray):
                flattened_data_object._attributes.append( peano4.dastgen2.Peano4DoubleArray( name        = attribute._name,
                                                                                             cardinality = attribute._cardinality,  
                                                                                             ifdefs      = attribute.ifdefs
                                                                                             ))
            else:
                flattened_data_object._attributes.append( attribute )
            if attribute.ifdefs==[]:
               copy_statements += """
result.set{}( static_cast<decltype(result.get{}())>( copy.get{}() ) );
""".format(attribute.get_accessor_name(),
           attribute.get_accessor_name(),
           attribute.get_accessor_name()
           )
            else:
                copy_statements += """
{}
result.set{}( static_cast<decltype(result.get{}())>( copy.get{}() ) );
#endif
""".format(dastgen2.Utils.construct_ifdef_string(attribute.ifdefs),
           attribute.get_accessor_name(),
           attribute.get_accessor_name(),
           attribute.get_accessor_name(),
           )
    

        extension             = "_Flattened"
        full_qualified_name_flattened   = full_qualified_name + extension
        full_qualified_file_without_extension_flattened = self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + extension
        
        
        self._data.data.additional_includes += """
#include "{}_Flattened.h"            
""".format(self._data.name)


#        flattened_data_object.additional_includes += """
##include "{}.h"            
#""".format(self._data.name)

            
        self._data.data.public_fields = """
  using FlattenedType = {}_Flattened;

  static {} flatten(const {}& copy) {{
    {} result;
    
    {}
    
    return result;
  }}
  static {} unflatten(const {}& copy) {{
    {} result;
    
    {}
    
    return result;
  }}
""".format(self._data.name,
#flatten
           self._data.name + extension,
           self._data.name,
           self._data.name + extension,
           copy_statements,
# unflatten
           self._data.name,
           self._data.name + extension,
           self._data.name,
           copy_statements,
           )

        self._data.data.set_full_qualified_name(full_qualified_name)
        self._data.data.write_header_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".h" )
        self._data.data.write_implementation_file(  self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cpp" )
        output.makefile.add_cpp_file( self._data.subdirectory + self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True )

        flattened_data_object.set_full_qualified_name(full_qualified_name_flattened)
        flattened_data_object.write_header_file( full_qualified_file_without_extension_flattened + ".h" )
        flattened_data_object.write_implementation_file(  full_qualified_file_without_extension_flattened + ".cpp" )
        output.makefile.add_cpp_file( full_qualified_file_without_extension_flattened + ".cpp", generated=True )

        
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
    """!

    Construct a DaStGen2 object
    
    We hold the data model and a dedicated data generator. By default, the data
    model is augmented with geometric debug information. Every Peano data 
    object has an MPI aspect, i.e. can, in principle, be sent and received 
    through MPI.
    
    """  
    super(DaStGen2, self).__init__(name)

    # is set by configure() lazily
    self.generator        = None
    
    self.data             = dastgen2.DataModel(name)
    
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "debugX", "Dimensions", ifdefs=["PeanoDebug>0"] ) )
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "debugH", "Dimensions", ifdefs=["PeanoDebug>0"] ) )
    
    self.peano4_mpi_and_storage_aspect = peano4.dastgen2.MPIAndStorageAspect(peano4.datamodel.DoFAssociation.Undef)

    self.data.add_aspect( self.peano4_mpi_and_storage_aspect )


  def configure(self, namespace, association, subdirectory=""):
    """!

    Configure output
    
    Configuring an output means setting the right output directory and
    taking into account whether we are associated to a vertex, face or
    cell.
    
    I always need the MPI aspect, but I can't add the right one in the 
    constructor, as I don't know whether this DaStGen model is used for 
    vertices, faces or cells. Such context however is important to generate
    the correct merge routines with the correct signature. Therefore, I 
    hook into this routine. 
    
    """
    super(DaStGen2, self).configure(namespace,association,subdirectory)
    self.peano4_mpi_and_storage_aspect.dof_association = association
    
    if self.hosts_smart_pointer_attribute():
        self.generator = DaStGen2GeneratorForObjectsWithSmartPointers(self)
    else:
        self.generator = DaStGen2Generator(self)
    
        
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


  def hosts_smart_pointer_attribute(self):
      """!
  
      Does class host a smart pointer attribute
      
      The context is important when we pick the data generator, as objects with 
      a smart poiner have to be handled differently compared to plain objects.
  
      """
      for attribute in self.data._attributes:
          if isinstance(attribute,peano4.dastgen2.Peano4SmartPointerDoubleArray):
              return True
      return False
          