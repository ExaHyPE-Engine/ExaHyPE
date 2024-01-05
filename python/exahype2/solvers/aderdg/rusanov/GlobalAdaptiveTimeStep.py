# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.ADERDG import PrecisionType
from exahype2.solvers.aderdg.ADERDG import Polynomials
from exahype2.solvers.aderdg.SingleSweep import SingleSweep

from exahype2.solvers.aderdg.kernels import create_abstract_solver_declarations
from exahype2.solvers.aderdg.kernels import create_abstract_solver_definitions
from exahype2.solvers.aderdg.kernels import create_solver_declarations
from exahype2.solvers.aderdg.kernels import create_solver_definitions

from exahype2.solvers.aderdg.rusanov.kernels import (
    create_abstract_solver_user_declarations_for_adaptive_time_stepping,
)
from exahype2.solvers.aderdg.rusanov.kernels import (
    create_abstract_solver_user_definitions_for_adaptive_time_stepping,
)

from exahype2.solvers.aderdg.rusanov.kernels import (
    create_compute_time_step_size_for_adaptive_time_stepping,
)
from exahype2.solvers.aderdg.rusanov.kernels import (
    create_compute_new_time_step_size_for_adaptive_time_stepping,
)

from exahype2.solvers.aderdg.rusanov.kernels import (
    create_start_time_step_implementation_for_adaptive_time_stepping,
)
from exahype2.solvers.aderdg.rusanov.kernels import (
    create_finish_time_step_implementation_for_adaptive_time_stepping,
)

from exahype2.solvers.aderdg.rusanov.kernels import (
    create_constructor_implementation_for_adaptive_time_stepping,
)


class GlobalAdaptiveTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        time_step_relaxation,
        plot_grid_properties=False,
        pde_terms_without_state=False,
        # Kernel optimisations
        is_linear=False,
        polynomials=Polynomials.Gauss_Legendre,
        use_kernel_generator=True,
        use_libxsmm=False,
        use_BLIS=False,
        use_Eigen=False,
        use_libxsmm_JIT=False,
        predictor_computation_precisions=["double"],
        corrector_computation_precision="double",
        solution_persistent_storage_precision="double",
        precision_criterion_implementation=PDETerms.None_Implementation,
        riemann_solver_implementation=PDETerms.None_Implementation,
        hold_cell_data_on_heap_with_smart_pointer = False,
        hold_face_data_on_heap_with_smart_pointer = False,
        use_vectorized_PDE=False,
        architecture="noarch",
        use_CERK_guess=False,
        use_split_CK=False,
        predictor_recompute=False,
        initialize_patches=False,
    ):
        super(GlobalAdaptiveTimeStep, self).__init__(
            name,
            order,
            unknowns,
            auxiliary_variables,
            min_cell_h,
            max_cell_h,
            plot_grid_properties,
            pde_terms_without_state,
            is_linear,
            polynomials,
            use_kernel_generator,
            use_libxsmm,
            use_BLIS,
            use_Eigen,
            use_libxsmm_JIT,
            predictor_computation_precisions,
            corrector_computation_precision,
            solution_persistent_storage_precision,
            precision_criterion_implementation,
            riemann_solver_implementation,
            hold_cell_data_on_heap_with_smart_pointer,
            hold_face_data_on_heap_with_smart_pointer,
            use_vectorized_PDE,
            architecture,
            use_CERK_guess,
            use_split_CK,
            predictor_recompute,
            initialize_patches,
        )

        self._cflSafetyFactor = time_step_relaxation


    def set_implementation(
        self,
        flux=None,
        ncp=None,
        eigenvalues=None,
        boundary_conditions=None,
        refinement_criterion=None,
        initial_conditions=None,
        source_term=None,
        material_parameters=None,
        point_source=0,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        super(GlobalAdaptiveTimeStep, self).set_implementation(
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            additional_action_set_includes="",
            additional_user_includes="",
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            source_term=source_term,
            material_parameters=material_parameters,
            point_source=point_source,
        )

        computation_precisions = self._predictor_computation_precisions[:]
        if self._corrector_computation_precision not in computation_precisions:
            computation_precisions.append(self._corrector_computation_precision)
        self._abstract_solver_user_declarations = create_abstract_solver_declarations(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )
        self._abstract_solver_user_declarations += (
            create_abstract_solver_user_declarations_for_adaptive_time_stepping()
        )
        self._abstract_solver_user_definitions = create_abstract_solver_definitions(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )
        self._abstract_solver_user_definitions += (
            create_abstract_solver_user_definitions_for_adaptive_time_stepping()
        )

        self._solver_user_declarations = create_solver_declarations(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )
        self._solver_user_definitions = create_solver_definitions(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )

        self._constructor_implementation = (
            create_constructor_implementation_for_adaptive_time_stepping()
        )

        self._compute_time_step_size = (
            create_compute_time_step_size_for_adaptive_time_stepping()
        )
        self._compute_new_time_step_size = (
            create_compute_new_time_step_size_for_adaptive_time_stepping()
        )

        self._start_time_step_implementation = (
            create_start_time_step_implementation_for_adaptive_time_stepping()
        )
        self._finish_time_step_implementation = (
            create_finish_time_step_implementation_for_adaptive_time_stepping()
        )
