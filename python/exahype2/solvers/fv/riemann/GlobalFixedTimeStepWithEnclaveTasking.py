# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .kernels import *

from peano4.toolbox.blockstructured import ReconstructedArrayMemoryLocation

from exahype2.solvers import PDETerms
from exahype2.solvers.fv.EnclaveTasking import EnclaveTasking

from exahype2.solvers.fv.FixedTimeSteppingCodeSnippets import (
    FixedTimeSteppingCodeSnippets,
)


class GlobalFixedTimeStepWithEnclaveTasking(EnclaveTasking):
    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        min_volume_h,
        max_volume_h,
        normalised_time_step_size,
        initial_conditions=PDETerms.User_Defined_Implementation,
        boundary_conditions=PDETerms.User_Defined_Implementation,
        refinement_criterion=PDETerms.Empty_Implementation,
        flux=PDETerms.User_Defined_Implementation,
        ncp=PDETerms.None_Implementation,
        eigenvalues=PDETerms.User_Defined_Implementation,
        riemann_solver=PDETerms.User_Defined_Implementation,
        source_term=PDETerms.None_Implementation,
        plot_grid_properties=False,
        pde_terms_without_state=False,
        overlap=1,
    ):
        """
        time_step_size: Float
          This is the normalised time step size w.r.t. the coarsest admissible h value. If
          the code employs AMR on top of it and refines further, it will automatically
          downscale the time step size accordingly. So hand in a valid time step size w.r.t.
          to max_volume_h.
        """
        super(GlobalFixedTimeStepWithEnclaveTasking, self).__init__(
            name,
            patch_size,
            overlap,
            unknowns,
            auxiliary_variables,
            min_volume_h,
            max_volume_h,
            plot_grid_properties,
            pde_terms_without_state,
            kernel_namespace="riemann",
        )
        self._normalised_time_step_size = normalised_time_step_size

        self._flux_implementation = PDETerms.None_Implementation
        self._ncp_implementation = PDETerms.None_Implementation
        self._eigenvalues_implementation = PDETerms.None_Implementation
        self._riemann_solver_implementation = PDETerms.None_Implementation
        self._source_term_implementation = PDETerms.None_Implementation

        self._compute_eigenvalue = False

        self.set_implementation(
            initial_conditions=initial_conditions,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            riemann_solver=riemann_solver,
            source_term=source_term,
        )

    def set_implementation(
        self,
        initial_conditions=None,
        boundary_conditions=None,
        refinement_criterion=None,
        flux=None,
        ncp=None,
        eigenvalues=None,
        riemann_solver=None,
        source_term=None,
        memory_location=None,
        use_split_loop=False,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        """
        If you pass in User_Defined, then the generator will create C++ stubs
        that you have to befill manually. If you pass in None_Implementation, it
        will create nop, i.e., no implementation or defaults. Any other string
        is copied 1:1 into the implementation. If you pass in None, then the
        set value so far won't be overwritten.

        Please note that not all options are supported by all solvers. You
        cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.

        This routine should be the very last invoked by the constructor.
        """
        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions
        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
        if flux is not None:
            self._flux_implementation = flux
        if ncp is not None:
            self._ncp_implementation = ncp
        if eigenvalues is not None:
            self._eigenvalues_implementation = eigenvalues
        if riemann_solver is not None:
            self._riemann_solver_implementation = riemann_solver
        if source_term is not None:
            self._source_term_implementation = source_term
        if memory_location is not None:
            self._reconstructed_array_memory_location = memory_location
        if use_split_loop:
            self._use_split_loop = use_split_loop

        self._compute_kernel_call = create_compute_Riemann_kernel(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            compute_max_eigenvalue_of_next_time_step=self._compute_eigenvalue,
            solver_variant=SolverVariant.WithVirtualFunctions,
            kernel_variant=KernelVariant.PatchWiseAoS,
        )

        self._compute_kernel_call_stateless = create_compute_Riemann_kernel(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            compute_max_eigenvalue_of_next_time_step=self._compute_eigenvalue,
            solver_variant=SolverVariant.Stateless,
            kernel_variant=KernelVariant.PatchWiseAoS,
        )

        self._fused_compute_kernel_call_stateless_cpu = create_compute_Riemann_kernel(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            compute_max_eigenvalue_of_next_time_step=self._compute_eigenvalue,
            solver_variant=SolverVariant.Stateless,
            kernel_variant=KernelVariant.PatchWiseAoS,
        )

        self._fused_compute_kernel_call_stateless_gpu = create_compute_Riemann_kernel(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            compute_max_eigenvalue_of_next_time_step=self._compute_eigenvalue,
            solver_variant=SolverVariant.Accelerator,
            kernel_variant=KernelVariant.PatchWiseAoS,
        )

        if (
            self._reconstructed_array_memory_location
            == ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete
            or self._reconstructed_array_memory_location
            == ReconstructedArrayMemoryLocation.HeapWithoutDelete
            or self._reconstructed_array_memory_location
            == ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarchWithoutDelete
        ):
            raise Exception(
                "Memory mode without appropriate delete chosen, i.e. this will lead to a memory leak"
            )

        solver_code_snippets = FixedTimeSteppingCodeSnippets(
            self._normalised_time_step_size, False
        )

        self._abstract_solver_user_declarations = create_abstract_solver_declarations(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
        )
        self._abstract_solver_user_declarations += (
            solver_code_snippets.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_definitions = create_abstract_solver_definitions(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
        )
        self._abstract_solver_user_definitions += (
            solver_code_snippets.create_abstract_solver_user_definitions()
        )

        self._solver_user_declarations = create_solver_declarations(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
        )
        self._solver_user_definitions = create_solver_definitions(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
        )

        self._compute_time_step_size = (
            solver_code_snippets.create_compute_time_step_size()
        )
        self._compute_new_time_step_size = (
            solver_code_snippets.create_compute_new_time_step_size()
        )

        self._start_time_step_implementation = (
            solver_code_snippets.create_start_time_step_implementation()
        )
        self._finish_time_step_implementation = (
            solver_code_snippets.create_finish_time_step_implementation()
        )
        self._constructor_implementation = (
            solver_code_snippets.create_abstract_solver_constructor_statements()
        )

        super(GlobalFixedTimeStepWithEnclaveTasking, self).set_implementation(
            initial_conditions=initial_conditions,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            memory_location=memory_location,
            use_split_loop=use_split_loop,
            additional_action_set_includes=additional_action_set_includes,
            additional_user_includes=additional_user_includes,
        )

    @property
    def user_action_set_includes(self):
        return (
            super(GlobalFixedTimeStepWithEnclaveTasking, self).user_action_set_includes
            + """
#include "exahype2/fv/riemann/Riemann.h"
"""
        )

    def __str__(self):
        result = (
            super(GlobalFixedTimeStepWithEnclaveTasking, self).__str__().rstrip("\n")
        )
        result += (
            """
Riemann solver:         """
            + str(self._riemann_solver_implementation)
            + """
"""
        )
        return result

    __repr__ = __str__
