# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output


class Observer(object):
    def __init__(
        self,
        classname,
        namespace,
        subdirectory,
        included_action_sets,
        vertices,
        faces,
        cells,
    ):
        """
        Included actions is a list of qualified actions which are used
        """
        self.classname = classname
        self.namespace = namespace
        self.subdirectory = subdirectory

        self.included_action_sets = included_action_sets
        self.vertices = vertices
        self.faces = faces
        self.cells = cells

        self.d = {}
        self.d["OPEN_NAMESPACE"] = ""
        self.d["CLOSE_NAMESPACE"] = ""
        self.d["FULL_QUALIFIED_CLASSNAME"] = ""
        for i in namespace:
            self.d["OPEN_NAMESPACE"] += "namespace " + i + "{\n"
            self.d["CLOSE_NAMESPACE"] += "}\n"
            self.d["FULL_QUALIFIED_CLASSNAME"] += i + "::"
        self.d["CLASSNAME"] = classname
        self.d["FULL_QUALIFIED_CLASSNAME"] += classname

        self.d["INCLUDES"] = ""
        self.d["ATTRIBUTES"] = ""
        for action in self.included_action_sets:
            self.d["INCLUDES"] += '#include "'
            self.d["INCLUDES"] += action.replace("::", "/")
            self.d["INCLUDES"] += '.h"\n'
            self.d["ATTRIBUTES"] += "    "
            self.d["ATTRIBUTES"] += action
            self.d["ATTRIBUTES"] += "   _actionSet"
            self.d["ATTRIBUTES"] += str(self.included_action_sets.index(action))
            self.d["ATTRIBUTES"] += "; \n"

        self.d["ACTIVE_ACTION_SETS"] = []
        for index, action in enumerate(self.included_action_sets):
            self.d["ACTIVE_ACTION_SETS"].append(
                {"name": f"_actionSet{index}", "full_qualified_name": action}
            )

        self.d["CELLS"] = []
        for cell in self.cells:
            self.d["CELLS"].append(
                {
                    "name": cell.name,
                    "logical_type_name": cell.get_logical_type_name(),
                    "full_qualified_type": cell.get_full_qualified_type(),
                }
            )

        self.d["FACES"] = []
        for face in self.faces:
            self.d["FACES"].append(
                {
                    "name": face.name,
                    "enumeration_type": face.get_enumeration_type(),
                    "logical_type_name": face.get_logical_type_name(),
                    "full_qualified_type": face.get_full_qualified_type(),
                }
            )

        self.d["VERTICES"] = []
        for vertex in self.vertices:
            self.d["VERTICES"].append(
                {
                    "name": vertex.name,
                    "enumeration_type": vertex.get_enumeration_type(),
                    "logical_type_name": vertex.get_logical_type_name(),
                    "full_qualified_type": vertex.get_full_qualified_type(),
                }
            )

        self.d[
            "DATAREPOSITORY_INCLUDES"
        ] = '#include "../repositories/DataRepository.h"\n'

    def generate(self, directory, protected_files):
        if not os.path.exists(directory + "/" + self.subdirectory):
            os.mkdir(directory + "/" + self.subdirectory)

        headerfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Observer.template.h"
        )
        cppfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Observer.template.cpp"
        )

        generated_implementation_file_pair = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                headfile_template=headerfile_template,
                cppfile_template=cppfile_template,
                classname=self.classname,
                namespace=self.namespace,
                subdirectory=self.subdirectory,
                dictionary=self.d,
                apply_iteratively=True,
            )
        )
        generated_implementation_file_pair.generate(directory, protected_files)
