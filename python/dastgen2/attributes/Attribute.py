# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import abc


class Attribute(object):
  """
  
   Represents one attribute 
   
  """
  def __init__(self, name):
    self._name          = name
    self.use_data_store = False


  def get_public_fields(self):
    """
    
      Return string that is to be embedded into the public part of the class
      definition. Most attributes don't add anything here, but some (alike
      enumerations) need to.
    
    """
    return ""

  @abc.abstractmethod
  def get_methods(self,_full_qualified_class_name):
    """
      
      Return sequence of methods that are defined for this attribute. Each
      entry is a tuple. Its first entry is the signature of the function 
      (not including the semicolon), the second entry is the return type.
      
    """
    return 

  
  def use_setter_to_initialise_arguments(self):
    """
    
    You can use this flag to control if the generated data object shall 
    use initialisation lists or plain old setters inside the constructor.
    
    """
    return False

  
  def get_constructor_arguments(self):
    """
    
    Return list of tuple of arguments for the constructor. The first 
    tuple entry is the name, the second one the type. By default,
    I take them from the plain C attributes, but you can alter
    this behaviour.
    
    """
    return self.get_plain_C_attributes()
           
  
  @abc.abstractmethod
  def get_plain_C_attributes(self):
    """
    
      Return list of triples. The first triple entry always is the name, 
      the second one the type. The third one can be C++ attributes.
      Type has to be plain C.
      
      ## Arrays
      
      Arrays can be modelled by adding a cardinality ("[15]" for example)
      to the first triple entry, i.e. the name. 
      
    """
    assert False, "abstract function should not be called"
    return


  def get_first_plain_C_attribute(self):
    """
    
     For MPI for example, I need to know the first attribute. If you map
     your attribute onto multiple data types, it is one type that represents the whole thing.
     
    """
    return self.get_plain_C_attributes()


  @abc.abstractmethod
  def get_method_body(self,signature):
    """
     
      I hand in the method signature (see get_methods()) and wanna get 
      the whole implementation.
      
    """
    return


  @abc.abstractmethod
  def get_native_MPI_type(self):
    """
     
      Return native (built-in) MPI datatype. Return None if there's no 
      direct mapping. The actual result is not a string only, but a list
      of tuples from native type to cardinality.
      
    """
    return 


  @abc.abstractmethod
  def get_to_string(self):
    """
    
      Return string representation of attribute. Should be something
      that can be streamed via << into a sstream. So the result has 
      to be of type string.

    """
    return
