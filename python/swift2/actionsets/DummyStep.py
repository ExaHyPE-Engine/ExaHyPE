# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


class DummyStep(ActionSet):
    """!

    Dummy step action set

    Does almost nothing, as the name suggests. The only thing it does is to 
    get the particle-vertex association tracing right, i.e. it tells the API 
    that a new mesh sweep starts.

    """

    def __init__(
        self,
    ):
        super( DummyStep, self ).__init__()
        pass

    def user_should_modify_template(self):
        return False


    def get_includes(self):
        return """
#include "toolbox/particles/assignmentchecks/TracingAPI.h"
"""        

    def get_body_of_prepareTraversal(self):
        return """
toolbox::particles::assignmentchecks::startMeshSweep( "DummyStep" );
"""
