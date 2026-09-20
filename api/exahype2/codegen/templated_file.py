# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from copy import deepcopy
from pathlib import Path

import jinja2

from exahype2.log import log

from .helper import get_jinja2_template
from .helper import render_jinja2_template
from .helper import write_file


class TemplatedFile:
    """Renders ``header_template`` and, when given, ``implementation_template`` into
    ``subdirectory/class_name`` with the template's own suffix.

    The dictionary receives ``CLASSNAME``, ``NAMESPACE`` and ``FULL_QUALIFIED_NAMESPACE``;
    ``apply_iteratively`` renders the result again until no Jinja2 tag remains or a result
    repeats.
    """

    def __init__(
        self,
        header_template,
        implementation_template,
        class_name,
        namespace,
        subdirectory,
        dictionary,
        apply_iteratively=False,
    ):
        self.header_template = header_template
        self.implementation_template = implementation_template
        self.class_name = class_name
        self.namespace = list(namespace)
        self.subdirectory = subdirectory
        self.apply_iteratively = apply_iteratively
        self.d = deepcopy(dictionary)
        self.d["CLASSNAME"] = class_name
        self.d["NAMESPACE"] = list(namespace)
        self.d["FULL_QUALIFIED_NAMESPACE"] = "::".join(namespace)

    def _write(self, directory, protected_files, template_file):
        target = os.path.join(
            directory,
            self.subdirectory,
            self.class_name + Path(template_file).with_suffix("").suffix,
        )
        if not write_file(directory, protected_files, target):
            return
        log.debug("Write %s", target)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        text = get_jinja2_template(template_file, jinja2.DebugUndefined).render(self.d)
        if self.apply_iteratively:
            rendered_texts = set()
            while text not in rendered_texts and any(
                delimiter in text for delimiter in ("{{", "{%", "{#")
            ):
                rendered_texts.add(text)
                text = render_jinja2_template(text, self.d, jinja2.DebugUndefined)
        with open(target, "w") as output:
            output.write(text)

    def generate(self, directory, protected_files):
        os.makedirs(os.path.join(directory, self.subdirectory), exist_ok=True)
        self._write(directory, protected_files, self.header_template)
        if self.implementation_template is not None:
            self._write(directory, protected_files, self.implementation_template)
