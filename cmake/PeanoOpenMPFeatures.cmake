# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

# Reports an OpenMP construct as supported only when the compiler accepts it in
# silence. A compiler that accepts a construct and then names it in a warning
# ignores the construct, so Peano takes the fallback. The probe runs on every
# configuration and caches no verdict, so a change of compiler leaves no stale
# answer behind.
function(_peano_check_openmp_construct construct_name construct_keyword probe_source out_var)
  string(MAKE_C_IDENTIFIER "${construct_name}" _peano_probe_identifier)
  set(_peano_probe_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/PeanoOpenMPProbe_${_peano_probe_identifier}.cpp")
  file(WRITE "${_peano_probe_file}" "${probe_source}")

  try_compile(
    _peano_probe_compiles
    SOURCES "${_peano_probe_file}"
    LINK_LIBRARIES OpenMP::OpenMP_CXX
    OUTPUT_VARIABLE _peano_probe_output
  )

  string(TOLOWER "${_peano_probe_output}" _peano_probe_diagnostics)
  set(_peano_probe_supported FALSE)
  if(_peano_probe_compiles AND NOT _peano_probe_diagnostics MATCHES "(warning|remark)[^\n]*${construct_keyword}")
    set(_peano_probe_supported TRUE)
  endif()

  if(_peano_probe_supported)
    message(STATUS "OpenMP ${construct_name} is supported")
  elseif(_peano_probe_compiles)
    message(STATUS "OpenMP ${construct_name} is accepted but ignored by this compiler; Peano uses the fallback")
  else()
    message(STATUS "OpenMP ${construct_name} is not supported; Peano uses the fallback")
  endif()

  set(${out_var} ${_peano_probe_supported} PARENT_SCOPE)
endfunction()

# Probes the OpenMP tasking constructs the task graph uses. Each one has a
# fallback, so a compiler that lacks it builds a correct but slower Peano.
function(peano_check_openmp_features)
  _peano_check_openmp_construct(
    "task depend(iterator)"
    "iterator"
    "
    #include <omp.h>

    int main() {
      char inputTokenStorage[2]{};
      char outputTokenStorage[3]{};
      char* inputTokens[2] = {&inputTokenStorage[0], &inputTokenStorage[1]};
      char* outputTokens[3] = {&outputTokenStorage[0], &outputTokenStorage[1], &outputTokenStorage[2]};
      const int numberOfInputTokens = 2;
      const int numberOfOutputTokens = 3;

      #pragma omp parallel
      #pragma omp single
      #pragma omp task depend(iterator(i = 0 : numberOfInputTokens), in: inputTokens[i][0]) depend(iterator(j = 0 : numberOfOutputTokens), out: outputTokens[j][0])
      {}

      return 0;
    }
    "
    PEANO_OPENMP_DEPEND_ITERATOR_SUPPORTED
  )

  if(PEANO_OPENMP_DEPEND_ITERATOR_SUPPORTED)
    target_compile_definitions(PeanoOptions INTERFACE PEANO_OPENMP_DEPEND_ITERATOR_SUPPORTED)
  endif()

  _peano_check_openmp_construct(
    "taskwait depend"
    "taskwait"
    "
    #include <omp.h>

    int main() {
      char token{};
      char* const tokenPointer = &token;

      #pragma omp parallel
      #pragma omp single
      {
        #pragma omp task depend(out: tokenPointer[0])
        {}
        #pragma omp taskwait depend(in: tokenPointer[0])
      }

      return 0;
    }
    "
    PEANO_OPENMP_TASKWAIT_DEPEND_SUPPORTED
  )

  if(PEANO_OPENMP_TASKWAIT_DEPEND_SUPPORTED)
    target_compile_definitions(PeanoOptions INTERFACE PEANO_OPENMP_TASKWAIT_DEPEND_SUPPORTED)
  endif()

  _peano_check_openmp_construct(
    "taskloop"
    "taskloop"
    "
    #include <omp.h>

    int main() {
      int total = 0;

      #pragma omp parallel
      #pragma omp single
      #pragma omp taskloop num_tasks(2) reduction(+: total)
      for (int i = 0; i < 8; ++i) {
        total += i;
      }

      return total == 28 ? 0 : 1;
    }
    "
    PEANO_OPENMP_TASKLOOP_SUPPORTED
  )

  if(PEANO_OPENMP_TASKLOOP_SUPPORTED)
    target_compile_definitions(PeanoOptions INTERFACE PEANO_OPENMP_TASKLOOP_SUPPORTED)
  endif()
endfunction()
