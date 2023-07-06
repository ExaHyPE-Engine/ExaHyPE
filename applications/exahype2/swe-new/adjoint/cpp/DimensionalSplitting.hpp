#ifndef applications_exahype2_swe_adjoint_DimensionalSplitting_h_
#define applications_exahype2_swe_adjoint_DimensionalSplitting_h_
#include "AdjointSolver.hpp"
#include <omp.h>
namespace applications::exahype2::swe::adjoint
{
    class Dimensional_Splitting
    {
        private:
            size_t l_nX_ = 0;
            size_t l_nY_ = 0;

            double l_dx_ = 0.0;
            double l_dy_ = 0.0;

            double *h_ = nullptr;
            double *hu_ = nullptr;
            double *hv_ = nullptr;
            double *b_ = nullptr;
            
            double *h_net_updates_left_ = nullptr;
            double *h_net_updates_right_ = nullptr;
            double *h_net_updates_below_ = nullptr;
            double *h_net_updates_above_ = nullptr;

            double *hu_net_updates_left_ = nullptr;
            double *hu_net_updates_right_ = nullptr;
            double *hv_net_updates_below_ = nullptr;
            double *hv_net_updates_above_ = nullptr;

            /**
             * @brief updates unknowns after x-sweep. Updated values used for y-sweep.
             * 
             * @param dt width of time step
             */
            void update_unknowns_x_sweep(double dt);

            /**
             * @brief updates unknowns after y-sweep. These are the final values of the simulation time step.
             * 
             * @param dt width of time step
             */
            void update_unknowns_y_sweep(double dt);
        
        public:
            Dimensional_Splitting(size_t l_nX, size_t l_nY, double l_dx, double l_dy, double *h, double *hu, double *hv, double *b);
            ~Dimensional_Splitting();

            /**
             * @brief computes numerical fluxes for one time step of the simulation.
             * 
             */
            double compute_numerical_fluxes();
    };
};
#endif