# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DoF import DoF
from .MetaDataToData import MetaDataToData


class MetaData(DoF):
    readme_descriptor = """
"""

    readme_package_descriptor = """
"""

    def __init__(self, name):
        super(MetaData, self).__init__(name)
        self.attributes = []
        self.merge_method_definition = ""
        self.send_condition = "true"
        self.receive_and_merge_condition = "true"
        self.includes = ""
        self.generator = MetaDataToData(self)

    def add_attribute(self, attribute):
        self.attributes.append(attribute)
