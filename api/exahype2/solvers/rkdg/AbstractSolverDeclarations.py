# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class AbstractSolverDeclarations:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_abstract_solver_user_declarations(self):
        template = jinja2.Template(
            r"""
    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<none>" or MAX_EIGENVALUE_IMPLEMENTATION=="<empty>" %}
    #error Max eigenvalue implementation cannot be none or empty
    {% endif %}
    {% if POINT_SOURCES_IMPLEMENTATION!="<none>" %}
    #error Point sources are not implemented for RKDG
    {% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )
        return template.render(**self.d)
