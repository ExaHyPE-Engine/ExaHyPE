# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen


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
        self.classname = classname
        self.namespace = namespace
        self.subdirectory = subdirectory

        self.included_action_sets = included_action_sets
        self.vertices = vertices
        self.faces = faces
        self.cells = cells

        self.d = {}
        self.d["CLASSNAME"] = classname
        self.d["FULL_QUALIFIED_CLASSNAME"] = "::".join(namespace + [classname])

        self.d["INCLUDES"] = sorted(
            action.replace("::", "/") + ".h" for action in self.included_action_sets
        )

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

        fine_vertices = [
            "fineGrid" + vertex["logical_type_name"] for vertex in self.d["VERTICES"]
        ]
        fine_faces = [
            "fineGrid" + face["logical_type_name"] for face in self.d["FACES"]
        ]
        fine_cells = [
            "fineGrid" + cell["logical_type_name"] for cell in self.d["CELLS"]
        ]
        coarse_data = [
            "coarseGrid" + entry["logical_type_name"]
            for entry in self.d["VERTICES"] + self.d["FACES"] + self.d["CELLS"]
        ]
        self.d["VERTEX_EVENT_ARGUMENTS"] = (
            ["vertexMarker"]
            + [vertex + "(pick)" for vertex in fine_vertices]
            + coarse_data
        )
        self.d["FACE_EVENT_ARGUMENTS"] = (
            ["faceMarker"]
            + fine_vertices
            + [face + "(pick)" for face in fine_faces]
            + coarse_data
        )
        self.d["CELL_EVENT_ARGUMENTS"] = (
            ["cellMarker"] + fine_vertices + fine_faces + fine_cells + coarse_data
        )

    def generate(self, directory, protected_files):
        if not os.path.exists(directory + "/" + self.subdirectory):
            os.makedirs(directory + "/" + self.subdirectory, exist_ok=True)

        headerfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Observer.h.jinja2"
        )
        cppfile_template = (
            os.path.dirname(os.path.realpath(__file__)) + "/Observer.cpp.jinja2"
        )

        generated_implementation_file_pair = exahype2.codegen.TemplatedFile(
            headerfile_template,
            cppfile_template,
            self.classname,
            self.namespace,
            self.subdirectory,
            self.d,
            apply_iteratively=True,
        )
        generated_implementation_file_pair.generate(directory, protected_files)
