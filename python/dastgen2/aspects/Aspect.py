# This file is part of the Peano 4 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from abc import ABC, abstractmethod


class Aspect(object):
    """!
    
    Superclass of each and every MPI aspect in Peano 4.
    
    """
    def __init__(self):
        self._data_model = None
        pass
    

    def set_model(self, 
                  data_model,
                  ):
        """!
        
        Inform aspect about model
        
        This routine is invoked by DataModel.add_aspect() and informs us about
        the data model in use.
        
        @param data_model: dastgen2.DataModel
          I cannot use the actual data model, as I otherwise get circular 
          inputs.
          
        """
        self._data_model = data_model


    @abstractmethod
    def get_attributes(self):
        raise NotImplementedError

    
    @abstractmethod
    def get_include(self):
        raise NotImplementedError


    @abstractmethod
    def get_method_declarations(self, full_qualified_name):
        raise NotImplementedError


    @abstractmethod
    def get_implementation(self, full_qualified_name):
        raise NotImplementedError
