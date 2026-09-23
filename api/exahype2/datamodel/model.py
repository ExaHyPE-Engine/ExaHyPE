# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen

from .dof import DoFAssociation


class Model:
    """The data of a project on cells, faces, vertices and global objects, and the
    DataRepository that holds their containers."""

    def __init__(self, namespace):
        self.cell_data = []
        self.face_data = []
        self.vertex_data = []
        self.global_data = []
        self.namespace = namespace
        self.namespaces = [namespace] if namespace else []

    def __str__(self):
        return (
            "(#Cells="
            + str(len(self.cell_data))
            + ", #Faces="
            + str(len(self.face_data))
            + ", #Vertices="
            + str(len(self.vertex_data))
            + ", #GlobalData="
            + str(len(self.global_data))
            + ")"
        )

    def add_cell(self, submodel):
        if any(existing is submodel for existing in self.cell_data):
            return
        submodel.configure(self.namespace, DoFAssociation.Cell)
        self.cell_data.append(submodel)

    def add_face(self, submodel):
        if any(existing is submodel for existing in self.face_data):
            return
        submodel.configure(self.namespace, DoFAssociation.Face)
        self.face_data.append(submodel)

    def add_vertex(self, submodel):
        if any(existing is submodel for existing in self.vertex_data):
            return
        submodel.configure(self.namespace, DoFAssociation.Vertex)
        self.vertex_data.append(submodel)

    def add_global_object(self, submodel):
        if any(existing is submodel for existing in self.global_data):
            return
        submodel.configure(self.namespace, DoFAssociation.Global)
        self.global_data.append(submodel)

    def _repository_dictionary(self, namespace):
        """Every rank generates the repository from the same model, so a container gets the
        same id everywhere and both sides of a rank pair read the same message tag."""
        dictionary = {
            "FULL_QUALIFIED_CLASSNAME": "::".join(namespace) + "::DataRepository"
        }
        container_id = 1
        for prefix, data in (
            ("CELL", self.cell_data),
            ("FACE", self.face_data),
            ("VERTEX", self.vertex_data),
        ):
            dictionary[prefix + "_DATA_CONTAINERS"] = []
            for data_set in data:
                dictionary[prefix + "_DATA_CONTAINERS"].append(
                    {
                        "stack": data_set.generator.get_stack_container(),
                        "name": data_set.get_logical_type_name(),
                        "fq": data_set.get_full_qualified_type(),
                        "header": data_set.namespace[-1] + "/" + data_set.name + ".h",
                        "container_id": container_id,
                    }
                )
                container_id += 1
        return dictionary

    def construct_output(self, output):
        for data in (
            self.cell_data,
            self.face_data,
            self.vertex_data,
            self.global_data,
        ):
            for data_set in data:
                data_set.generator.construct_output(output)

        template_directory = os.path.dirname(os.path.realpath(__file__))
        for namespace in self.namespaces:
            repository_namespace = namespace + ["repositories"]
            output.makefile.add_cpp_file(
                "repositories/DataRepository.cpp", generated=True
            )
            output.add(
                exahype2.codegen.TemplatedFile(
                    os.path.join(template_directory, "DataRepository.h.jinja2"),
                    os.path.join(template_directory, "DataRepository.cpp.jinja2"),
                    "DataRepository",
                    repository_namespace,
                    "repositories",
                    self._repository_dictionary(repository_namespace),
                    apply_iteratively=True,
                )
            )
