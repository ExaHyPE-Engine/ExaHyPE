// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

namespace exahype2::fv::rusanov::cpp::internal {
  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedStateless(
    int       targetDevice,
    CellData& patchData,
    double*   tempFluxX,
    double*   tempFluxY,
    double*   tempFluxZ,
    double*   tempNonconservativeProductX,
    double*   tempNonconservativeProductY,
    double*   tempNonconservativeProductZ,
    double*   tempEigenvalueX,
    double*   tempEigenvalueY,
    double*   tempEigenvalueZ
  ) InlineMethod {
    const enumerator::AoSLexicographicEnumerator QInEnumerator(1, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, NumberOfAuxiliaryVariables);
    const enumerator::AoSLexicographicEnumerator QOutEnumerator(1, NumberOfVolumesPerAxisInPatch, 0, NumberOfUnknowns, NumberOfAuxiliaryVariables);
    const TempDataEnumeratorType                  fluxEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, 0);
    const TempDataEnumeratorType                  ncpEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch + 1, HaloSize, NumberOfUnknowns, 0);
    const TempDataEnumeratorType                  eigenvalueEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, 1, 0);

    // The cartesian products have to be located on the heap so that they are accessible from the GPU.
    // Here we define types to able to allocated in heap.
    using cart_prod_2d = decltype(tl::views::cartesian_product(std::views::iota(0, 1), std::views::iota(0, 1)));
    using cart_prod_3d = decltype(tl::views::cartesian_product(std::views::iota(0, 1), std::views::iota(0, 1), std::views::iota(0, 1)));
    using cart_prod_4d = decltype(tl::views::cartesian_product(std::views::iota(0, 1), std::views::iota(0, 1), std::views::iota(0, 1), std::views::iota(0, 1)));
    using cart_prod_5d = decltype(tl::views::cartesian_product(
      std::views::iota(0, 1), std::views::iota(0, 1), std::views::iota(0, 1), std::views::iota(0, 1), std::views::iota(0, 1)
    ));

    // Definition of the cartesian products.
    auto* range2d = new cart_prod_2d;
    *range2d      = tl::views::cartesian_product(std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, NumberOfVolumesPerAxisInPatch));

    auto* range3d = new cart_prod_3d;
    *range3d      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, NumberOfVolumesPerAxisInPatch)
    );

    auto* range2d_cell = new cart_prod_3d;
    *range2d_cell      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, patchData.numberOfCells)
    );

    auto* range3d_cell = new cart_prod_4d;
    *range3d_cell      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells)
    );

    auto* range2d_cell_unknown_aux = new cart_prod_4d;
    *range2d_cell_unknown_aux      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells),
      std::views::iota(0, NumberOfUnknowns + NumberOfAuxiliaryVariables)
    );

    auto* range3d_cell_unknown_aux = new cart_prod_5d;
    *range3d_cell_unknown_aux      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells),
      std::views::iota(0, NumberOfUnknowns + NumberOfAuxiliaryVariables)
    );

    auto* range1d_cell_solvhalosize = new cart_prod_3d;
    *range1d_cell_solvhalosize      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, patchData.numberOfCells), std::views::iota(0, NumberOfVolumesPerAxisInPatch + 2 * HaloSize)
    );

    auto* range2d_cell_solvhalosize = new cart_prod_4d;
    *range2d_cell_solvhalosize      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch + 2 * HaloSize)
    );

    auto* range2d_cell_unknown = new cart_prod_4d;
    *range2d_cell_unknown      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells),
      std::views::iota(0, NumberOfUnknowns)
    );

    auto* range3d_cell_unknown = new cart_prod_5d;
    *range3d_cell_unknown      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells),
      std::views::iota(0, NumberOfUnknowns)
    );

    auto* range1d_cell_solvhalosize_m = new cart_prod_3d;
    *range1d_cell_solvhalosize_m      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch), std::views::iota(0, patchData.numberOfCells), std::views::iota(0, NumberOfVolumesPerAxisInPatch + 2 * HaloSize - 1)
    );

    auto* range2d_cell_solvhalosize_m = new cart_prod_4d;
    *range2d_cell_solvhalosize_m      = tl::views::cartesian_product(
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch),
      std::views::iota(0, patchData.numberOfCells),
      std::views::iota(0, NumberOfVolumesPerAxisInPatch + 2 * HaloSize - 1)
    );

    // ====================================================
    // Copy solution over and evaluate source (if required)
    // ====================================================
    if constexpr (EvaluateSource) {
#if Dimensions == 2
      std::for_each(
        std::execution::par_unseq,
        range2d_cell->begin(),
        range2d_cell->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
          auto [x, y, patchIndex] = ids;
          loopbodies::copySolutionAndAddSourceTerm<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y),
            t[patchIndex],
            dt[patchIndex],
            QOut[patchIndex],
            QOutEnumerator
          );
        }
      );
#else
      std::for_each(
        std::execution::par_unseq,
        range3d_cell->begin(),
        range3d_cell->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
          auto [x, y, z, patchIndex] = ids;
          loopbodies::copySolutionAndAddSourceTerm<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y, z),
            t[patchIndex],
            dt[patchIndex],
            QOut[patchIndex],
            QOutEnumerator
          );
        }
      );
#endif
    } else {
#if Dimensions == 2
      std::for_each(std::execution::par_unseq, range2d_cell_unknown_aux->begin(), range2d_cell_unknown_aux->end(), [=, QIn = patchData.QIn, QOut = patchData.QOut](auto ids) {
        auto [x, y, patchIndex, unknown] = ids;
        loopbodies::copySolution(QIn[patchIndex], QInEnumerator, patchIndex, volumeIndex(x, y), unknown, QOut[patchIndex], QOutEnumerator);
      });
#else
      std::for_each(std::execution::par_unseq, range3d_cell_unknown_aux->begin(), range3d_cell_unknown_aux->end(), [=, QIn = patchData.QIn, QOut = patchData.QOut](auto ids) {
        auto [x, y, z, patchIndex, unknown] = ids;
        loopbodies::copySolution(QIn[patchIndex], QInEnumerator, patchIndex, volumeIndex(x, y, z), unknown, QOut[patchIndex], QOutEnumerator);
      });
#endif
    }

    // ====================================================
    // Compute damping due to max eigenvalue
    // ====================================================
#if Dimensions == 2
    std::for_each(
      std::execution::par_unseq,
      range1d_cell_solvhalosize->begin(),
      range1d_cell_solvhalosize->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
        auto [y, patchIndex, x] = ids;
        loopbodies::computeMaxEigenvalue<SolverType>(
          QIn[patchIndex],
          QInEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x - HaloSize, y),
          t[patchIndex],
          dt[patchIndex],
          0,
          tempEigenvalueX,
          eigenvalueEnumerator
        );
      }
    );
    std::for_each(
      std::execution::par_unseq,
      range1d_cell_solvhalosize->begin(),
      range1d_cell_solvhalosize->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
        auto [x, patchIndex, y] = ids;
        loopbodies::computeMaxEigenvalue<SolverType>(
          QIn[patchIndex],
          QInEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x, y - HaloSize),
          t[patchIndex],
          dt[patchIndex],
          1,
          tempEigenvalueY,
          eigenvalueEnumerator
        );
      }
    );

    std::for_each(
      std::execution::par_unseq,
      range2d_cell_unknown->begin(),
      range2d_cell_unknown->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
        auto [x, y, patchIndex, unknown] = ids;
        loopbodies::updateSolutionWithEigenvalueDamping(
          QIn[patchIndex],
          QInEnumerator,
          tempEigenvalueX,
          tempEigenvalueY,
          tempEigenvalueZ,
          eigenvalueEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x, y),
          unknown,
          dt[patchIndex],
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    );
#else
    std::for_each(
      std::execution::par_unseq,
      range2d_cell_solvhalosize->begin(),
      range2d_cell_solvhalosize->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
        auto [z, y, patchIndex, x] = ids;
        loopbodies::computeMaxEigenvalue<SolverType>(
          QIn[patchIndex],
          QInEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x - HaloSize, y, z),
          t[patchIndex],
          dt[patchIndex],
          0,
          tempEigenvalueX,
          eigenvalueEnumerator
        );
      }
    );
    std::for_each(
      std::execution::par_unseq,
      range2d_cell_solvhalosize->begin(),
      range2d_cell_solvhalosize->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
        auto [z, x, patchIndex, y] = ids;
        loopbodies::computeMaxEigenvalue<SolverType>(
          QIn[patchIndex],
          QInEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x, y - HaloSize, z),
          t[patchIndex],
          dt[patchIndex],
          1,
          tempEigenvalueY,
          eigenvalueEnumerator
        );
      }
    );
    std::for_each(
      std::execution::par_unseq,
      range2d_cell_solvhalosize->begin(),
      range2d_cell_solvhalosize->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
        auto [y, x, patchIndex, z] = ids;
        loopbodies::computeMaxEigenvalue<SolverType>(
          QIn[patchIndex],
          QInEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x, y, z - HaloSize),
          t[patchIndex],
          dt[patchIndex],
          2,
          tempEigenvalueZ,
          eigenvalueEnumerator
        );
      }
    );

    std::for_each(
      std::execution::par_unseq,
      range3d_cell_unknown->begin(),
      range3d_cell_unknown->end(),
      [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
        auto [x, y, z, patchIndex, unknown] = ids;
        loopbodies::updateSolutionWithEigenvalueDamping(
          QIn[patchIndex],
          QInEnumerator,
          tempEigenvalueX,
          tempEigenvalueY,
          tempEigenvalueZ,
          eigenvalueEnumerator,
          cellCentre[patchIndex],
          cellSize[patchIndex],
          patchIndex,
          volumeIndex(x, y, z),
          unknown,
          dt[patchIndex],
          QOut[patchIndex],
          QOutEnumerator
        );
      }
    );
#endif

    // ====================================================
    // Normal (conservative) flux
    // ====================================================
    if constexpr (EvaluateFlux) {
#if Dimensions == 2
      std::for_each(
        std::execution::par_unseq,
        range1d_cell_solvhalosize->begin(),
        range1d_cell_solvhalosize->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [y, patchIndex, x] = ids;
          loopbodies::computeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x - HaloSize, y),
            t[patchIndex],
            dt[patchIndex],
            0, // normal
            tempFluxX,
            fluxEnumerator
          );
        }
      );
      std::for_each(
        std::execution::par_unseq,
        range1d_cell_solvhalosize->begin(),
        range1d_cell_solvhalosize->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [x, patchIndex, y] = ids;
          loopbodies::computeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y - HaloSize),
            t[patchIndex],
            dt[patchIndex],
            1, // normal
            tempFluxY,
            fluxEnumerator
          );
        }
      );

      std::for_each(
        std::execution::par_unseq,
        range2d_cell_unknown->begin(),
        range2d_cell_unknown->end(),
        [=, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
          auto [x, y, patchIndex, unknown] = ids;
          loopbodies::updateSolutionWithFlux(
            tempFluxX,
            tempFluxY,
            tempFluxZ,
            fluxEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y),
            unknown,
            dt[patchIndex],
            QOut[patchIndex],
            QOutEnumerator
          );
        }
      );
#else
      std::for_each(
        std::execution::par_unseq,
        range2d_cell_solvhalosize->begin(),
        range2d_cell_solvhalosize->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [z, y, patchIndex, x] = ids;
          loopbodies::computeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x - HaloSize, y, z),
            t[patchIndex],
            dt[patchIndex],
            0, // normal
            tempFluxX,
            fluxEnumerator
          );
        }
      );
      std::for_each(
        std::execution::par_unseq,
        range2d_cell_solvhalosize->begin(),
        range2d_cell_solvhalosize->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [z, x, patchIndex, y] = ids;
          loopbodies::computeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y - HaloSize, z),
            t[patchIndex],
            dt[patchIndex],
            1, // normal
            tempFluxY,
            fluxEnumerator
          );
        }
      );
      std::for_each(
        std::execution::par_unseq,
        range2d_cell_solvhalosize->begin(),
        range2d_cell_solvhalosize->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [x, y, patchIndex, z] = ids;
          loopbodies::computeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y, z - HaloSize),
            t[patchIndex],
            dt[patchIndex],
            2, // normal
            tempFluxZ,
            fluxEnumerator
          );
        }
      );

      std::for_each(
        std::execution::par_unseq,
        range3d_cell_unknown->begin(),
        range3d_cell_unknown->end(),
        [=, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
          auto [x, y, z, patchIndex, unknown] = ids;
          loopbodies::updateSolutionWithFlux(
            tempFluxX,
            tempFluxY,
            tempFluxZ,
            fluxEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y, z),
            unknown,
            dt[patchIndex],
            QOut[patchIndex],
            QOutEnumerator
          );
        }
      );
#endif
    }

    if constexpr (EvaluateNonconservativeProduct) {
#if Dimensions == 2
      std::for_each(
        std::execution::par_unseq,
        range1d_cell_solvhalosize_m->begin(),
        range1d_cell_solvhalosize_m->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [y, patchIndex, x] = ids;
          loopbodies::computeNonconservativeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x - HaloSize, y),
            t[patchIndex],
            dt[patchIndex],
            0, // normal
            tempNonconservativeProductX,
            ncpEnumerator
          );
        }
      );
      std::for_each(
        std::execution::par_unseq,
        range1d_cell_solvhalosize_m->begin(),
        range1d_cell_solvhalosize_m->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [x, patchIndex, y] = ids;
          loopbodies::computeNonconservativeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y - HaloSize),
            t[patchIndex],
            dt[patchIndex],
            1, // normal
            tempNonconservativeProductY,
            ncpEnumerator
          );
        }
      );

      std::for_each(
        std::execution::par_unseq,
        range2d_cell_unknown->begin(),
        range2d_cell_unknown->end(),
        [=, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
          auto [x, y, patchIndex, unknown] = ids;
          loopbodies::updateSolutionWithNonconservativeFlux(
            tempNonconservativeProductX,
            tempNonconservativeProductY,
            tempNonconservativeProductZ,
            ncpEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y),
            unknown,
            dt[patchIndex],
            QOut[patchIndex],
            QOutEnumerator
          );
        }
      );
#else
      std::for_each(
        std::execution::par_unseq,
        range2d_cell_solvhalosize_m->begin(),
        range2d_cell_solvhalosize_m->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [z, y, patchIndex, x] = ids;
          loopbodies::computeNonconservativeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x - HaloSize, y, z),
            t[patchIndex],
            dt[patchIndex],
            0, // normal
            tempNonconservativeProductX,
            ncpEnumerator
          );
        }
      );
      std::for_each(
        std::execution::par_unseq,
        range2d_cell_solvhalosize_m->begin(),
        range2d_cell_solvhalosize_m->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [z, x, patchIndex, y] = ids;
          loopbodies::computeNonconservativeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y - HaloSize, z),
            t[patchIndex],
            dt[patchIndex],
            1, // normal
            tempNonconservativeProductY,
            ncpEnumerator
          );
        }
      );
      std::for_each(
        std::execution::par_unseq,
        range2d_cell_solvhalosize_m->begin(),
        range2d_cell_solvhalosize_m->end(),
        [=, QIn = patchData.QIn, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
          auto [y, x, patchIndex, z] = ids;
          loopbodies::computeNonconservativeFlux<SolverType>(
            QIn[patchIndex],
            QInEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y, z - HaloSize),
            t[patchIndex],
            dt[patchIndex],
            2, // normal
            tempNonconservativeProductZ,
            ncpEnumerator
          );
        }
      );

      std::for_each(
        std::execution::par_unseq,
        range3d_cell_unknown->begin(),
        range3d_cell_unknown->end(),
        [=, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, dt = patchData.dt, QOut = patchData.QOut](auto ids) {
          auto [x, y, z, patchIndex, unknown] = ids;
          loopbodies::updateSolutionWithNonconservativeFlux(
            tempNonconservativeProductX,
            tempNonconservativeProductY,
            tempNonconservativeProductZ,
            ncpEnumerator,
            cellCentre[patchIndex],
            cellSize[patchIndex],
            patchIndex,
            volumeIndex(x, y, z),
            unknown,
            dt[patchIndex],
            QOut[patchIndex],
            QOutEnumerator
          );
        }
      );
#endif
    }

    if constexpr (EvaluateMaximumEigenvalueAfterTimeStep) {
#if Dimensions == 2
      for (int patchIndex = 0; patchIndex < patchData.numberOfCells; patchIndex++) {
        double newMaxEigenvalue = 0.0;
        std::for_each(
          std::execution::seq,
          range2d->begin(),
          range2d->end(),
          [=, &newMaxEigenvalue, QOut = patchData.QOut, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
            auto [x, y]      = ids;
            newMaxEigenvalue = std::max(
              newMaxEigenvalue,
              loopbodies::reduceMaxEigenvalue<SolverType>(
                QOut[patchIndex], QOutEnumerator, cellCentre[patchIndex], cellSize[patchIndex], patchIndex, volumeIndex(x, y), t[patchIndex], dt[patchIndex]
              )
            );
          }
        );
        patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
      }
#else
      for (int patchIndex = 0; patchIndex < patchData.numberOfCells; patchIndex++) {
        double newMaxEigenvalue = 0.0;
        std::for_each(
          std::execution::seq,
          range3d->begin(),
          range3d->end(),
          [=, &newMaxEigenvalue, QOut = patchData.QOut, cellCentre = patchData.cellCentre, cellSize = patchData.cellSize, t = patchData.t, dt = patchData.dt](auto ids) {
            auto [x, y, z]   = ids;
            newMaxEigenvalue = std::max(
              newMaxEigenvalue,
              loopbodies::reduceMaxEigenvalue<SolverType>(
                QOut[patchIndex], QOutEnumerator, cellCentre[patchIndex], cellSize[patchIndex], patchIndex, volumeIndex(x, y, z), t[patchIndex], dt[patchIndex]
              )
            );
          }
        );
        patchData.maxEigenvalue[patchIndex] = newMaxEigenvalue;
      }
#endif
    }

    delete[] range2d;
    delete[] range3d;
    delete[] range2d_cell;
    delete[] range3d_cell;
    delete[] range2d_cell_unknown_aux;
    delete[] range3d_cell_unknown_aux;
    delete[] range1d_cell_solvhalosize;
    delete[] range2d_cell_solvhalosize;
    delete[] range2d_cell_unknown;
    delete[] range3d_cell_unknown;
    delete[] range1d_cell_solvhalosize_m;
    delete[] range2d_cell_solvhalosize_m;
  }
} // namespace exahype2::fv::rusanov::cpp::internal


template <
  class SolverType,
  int  NumberOfVolumesPerAxisInPatch,
  int  HaloSize,
  int  NumberOfUnknowns,
  int  NumberOfAuxiliaryVariables,
  bool EvaluateFlux,
  bool EvaluateNonconservativeProduct,
  bool EvaluateSource,
  bool EvaluateMaximumEigenvalueAfterTimeStep,
  class TempDataEnumeratorType>
void exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless(int targetDevice, CellData& patchData, tarch::timing::Measurement& measurement) {
  static_assert(HaloSize == 1);

  static tarch::logging::Log _log("exahype2::fv::rusanov::cpp");
  logTraceIn("timeStepWithRusanovBatchedUSMStateless()");

  const enumerator::AoSLexicographicEnumerator QInEnumerator(1, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const enumerator::AoSLexicographicEnumerator QOutEnumerator(1, NumberOfVolumesPerAxisInPatch, 0, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const TempDataEnumeratorType                  fluxEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  ncpEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch + 1, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  eigenvalueEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, 1, 0);

  double* tempFluxX                   = new double[fluxEnumerator.size()];
  double* tempFluxY                   = new double[fluxEnumerator.size()];
  double* tempFluxZ                   = new double[fluxEnumerator.size()];
  double* tempNonconservativeProductX = new double[ncpEnumerator.size()];
  double* tempNonconservativeProductY = new double[ncpEnumerator.size()];
  double* tempNonconservativeProductZ = new double[ncpEnumerator.size()];
  double* tempEigenvalueX             = new double[eigenvalueEnumerator.size()];
  double* tempEigenvalueY             = new double[eigenvalueEnumerator.size()];
  double* tempEigenvalueZ             = new double[eigenvalueEnumerator.size()];

  tarch::timing::Watch watch("exahype2::fv::rusanov::cpp", "timeStepWithRusanovBatchedUSMStateless", false, true);
  internal::timeStepWithRusanovBatchedStateless<
    SolverType,
    NumberOfVolumesPerAxisInPatch,
    HaloSize,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumeratorType>(
    targetDevice,
    patchData,
    tempFluxX,
    tempFluxY,
    tempFluxZ,
    tempNonconservativeProductX,
    tempNonconservativeProductY,
    tempNonconservativeProductZ,
    tempEigenvalueX,
    tempEigenvalueY,
    tempEigenvalueZ
  );
  watch.stop();
  measurement.setValue(watch.getCalendarTime());

  if (tempFluxX != nullptr) {
    delete[] tempFluxX;
  }
  if (tempFluxY != nullptr) {
    delete[] tempFluxY;
  }
  if (tempFluxZ != nullptr) {
    delete[] tempFluxZ;
  }
  if (tempNonconservativeProductX != nullptr) {
    delete[] tempNonconservativeProductX;
  }
  if (tempNonconservativeProductY != nullptr) {
    delete[] tempNonconservativeProductY;
  }
  if (tempNonconservativeProductZ != nullptr) {
    delete[] tempNonconservativeProductZ;
  }
  if (tempEigenvalueX != nullptr) {
    delete[] tempEigenvalueX;
  }
  if (tempEigenvalueY != nullptr) {
    delete[] tempEigenvalueY;
  }
  if (tempEigenvalueZ != nullptr) {
    delete[] tempEigenvalueZ;
  }

  logTraceOut("timeStepWithRusanovBatchedUSMStateless()");
}


template <
  class SolverType,
  int  NumberOfVolumesPerAxisInPatch,
  int  HaloSize,
  int  NumberOfUnknowns,
  int  NumberOfAuxiliaryVariables,
  bool EvaluateFlux,
  bool EvaluateNonconservativeProduct,
  bool EvaluateSource,
  bool EvaluateMaximumEigenvalueAfterTimeStep,
  class TempDataEnumeratorType>
void exahype2::fv::rusanov::cpp::timeStepWithRusanovBatchedUSMStateless(int targetDevice, CellData& patchData) {
  static_assert(HaloSize == 1);

  static tarch::logging::Log _log("exahype2::fv::rusanov::cpp");
  logTraceIn("timeStepWithRusanovBatchedUSMStateless()");

  const enumerator::AoSLexicographicEnumerator QInEnumerator(1, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const enumerator::AoSLexicographicEnumerator QOutEnumerator(1, NumberOfVolumesPerAxisInPatch, 0, NumberOfUnknowns, NumberOfAuxiliaryVariables);
  const TempDataEnumeratorType                  fluxEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  ncpEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch + 1, HaloSize, NumberOfUnknowns, 0);
  const TempDataEnumeratorType                  eigenvalueEnumerator(patchData.numberOfCells, NumberOfVolumesPerAxisInPatch, HaloSize, 1, 0);

  double* tempFluxX                   = new double[fluxEnumerator.size()];
  double* tempFluxY                   = new double[fluxEnumerator.size()];
  double* tempFluxZ                   = new double[fluxEnumerator.size()];
  double* tempNonconservativeProductX = new double[ncpEnumerator.size()];
  double* tempNonconservativeProductY = new double[ncpEnumerator.size()];
  double* tempNonconservativeProductZ = new double[ncpEnumerator.size()];
  double* tempEigenvalueX             = new double[eigenvalueEnumerator.size()];
  double* tempEigenvalueY             = new double[eigenvalueEnumerator.size()];
  double* tempEigenvalueZ             = new double[eigenvalueEnumerator.size()];

  internal::timeStepWithRusanovBatchedStateless<
    SolverType,
    NumberOfVolumesPerAxisInPatch,
    HaloSize,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    EvaluateFlux,
    EvaluateNonconservativeProduct,
    EvaluateSource,
    EvaluateMaximumEigenvalueAfterTimeStep,
    TempDataEnumeratorType>(
    targetDevice,
    patchData,
    tempFluxX,
    tempFluxY,
    tempFluxZ,
    tempNonconservativeProductX,
    tempNonconservativeProductY,
    tempNonconservativeProductZ,
    tempEigenvalueX,
    tempEigenvalueY,
    tempEigenvalueZ
  );

  if (tempFluxX != nullptr) {
    delete[] tempFluxX;
  }
  if (tempFluxY != nullptr) {
    delete[] tempFluxY;
  }
  if (tempFluxZ != nullptr) {
    delete[] tempFluxZ;
  }
  if (tempNonconservativeProductX != nullptr) {
    delete[] tempNonconservativeProductX;
  }
  if (tempNonconservativeProductY != nullptr) {
    delete[] tempNonconservativeProductY;
  }
  if (tempNonconservativeProductZ != nullptr) {
    delete[] tempNonconservativeProductZ;
  }
  if (tempEigenvalueX != nullptr) {
    delete[] tempEigenvalueX;
  }
  if (tempEigenvalueY != nullptr) {
    delete[] tempEigenvalueY;
  }
  if (tempEigenvalueZ != nullptr) {
    delete[] tempEigenvalueZ;
  }

  logTraceOut("timeStepWithRusanovBatchedUSMStateless()");
}
