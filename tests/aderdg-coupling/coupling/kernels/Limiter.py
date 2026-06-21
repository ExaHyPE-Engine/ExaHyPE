# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from copy import deepcopy

import peano4.output

import mpmath as mp

def compute_projector_to_point_set(quad_points_from, quad_points_to):
  """
  Transforms the degrees of freedom located at Lagrange quadrature_points to degrees of freedom
  located at a set of points as defined by the parameter points.
  
  Let us denote by P the  projection operator. The DoF are computed according to:
  
  u@x =  sum_{m} P_m u^DG_m
  
  Returns:
    Projector:
      The corresponding degrees of freedom located at the set of points defined by the parameter x.
  """
  proj = [[mp.mpf(0) for _ in range(len(quad_points_from))] for _ in range(len(quad_points_to))]
  for idx, point in enumerate(quad_points_to):
      # For each point to which we project, calculate lagrange weights for that point 
      n = len(quad_points_from)
      # weights = []
      for i in range(n):
        numerator = 1.0
        denominator = 1.0
        result = 1.0
        for k in range(n):
            if i != k:
                numerator *= (point - quad_points_from[k])
                denominator *= (quad_points_from[i] - quad_points_from[k])
        result*= numerator / denominator
        proj[idx][i] = result
  return proj

def flatten(xss):
  return ', '.join([mp.nstr(x, n=12) for xs in xss for x in xs])


class Limiter:
    def __init__(self, dictionary):
        self.d = deepcopy(dictionary)

    def generate_kernels(
        self, namespace, output, template_prefix, output_path,
        regular_solver_quadpoints, limiter_solver_quadpoints
    ):

        self.d["PROJ_REGULAR_TO_LIMITER"] = flatten(
          compute_projector_to_point_set(
            regular_solver_quadpoints, limiter_solver_quadpoints
        ))

        self.d["PROJ_LIMITER_TO_REGULAR"] = flatten(
          compute_projector_to_point_set(
            limiter_solver_quadpoints, regular_solver_quadpoints
        ))

        self.d["DIFF_ORDERS"] = regular_solver_quadpoints!=limiter_solver_quadpoints

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=template_prefix + "Limiter.template.h",
            cppfile_template=template_prefix + "Limiter.template.cpp",
            classname=output_path + "/Limiter",
            namespace=namespace,
            subdirectory=".",
            dictionary=self.d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(output_path + "/Limiter.h", generated=True)
        output.makefile.add_cpp_file(output_path + "/Limiter.cpp", generated=True)
