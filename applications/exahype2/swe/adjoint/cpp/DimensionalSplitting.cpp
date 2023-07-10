#include "DimensionalSplitting.hpp"

applications::exahype2::swe::adjoint::DimensionalSplitting::DimensionalSplitting(size_t l_nX, size_t l_nY, double l_dx, double l_dy, double *h, double *hu, double *hv, double *b) :
l_nX_(l_nX),
l_nY_(l_nY),
l_dx_(l_dx),
l_dy_(l_dy)
{
    this->h_ = h;
    this->hu_ = hu;
    this->hv_ = hv;
    this->b_ = b;

    this->h_net_updates_above_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
    this->h_net_updates_below_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
    this->h_net_updates_left_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
    this->h_net_updates_right_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();

    this->hu_net_updates_left_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
    this->hu_net_updates_right_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
    this->hv_net_updates_above_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
    this->hv_net_updates_below_ = new double[(l_nX_ + 2)*(l_nY_ * 2)]();
}

applications::exahype2::swe::adjoint::DimensionalSplitting::~DimensionalSplitting()
{
    delete this->h_net_updates_above_;
    delete this->h_net_updates_below_;
    delete this->h_net_updates_left_;
    delete this->h_net_updates_right_;

    delete this->hu_net_updates_left_;
    delete this->hu_net_updates_right_;
    delete this->hv_net_updates_above_;
    delete this->hv_net_updates_below_;
}

double applications::exahype2::swe::adjoint::DimensionalSplitting::compute_numerical_fluxes()
{
    double max_wave_speed = 0.0;

    double max_x_wave_speed = 0.0;
    // for all columns
    #pragma omp parallel for reduction(max: max_x_wave_speed) schedule(runtime)
    for (int i = 1; i < l_nX_ + 2; i++)
    {
        // for all rows
        for (int j = 0; j < l_nY_ + 2; j++)
        {
            double max_edge_speed = 0.0;

            applications::exahype2::swe::adjoint::Adjoint_Solver::compute_net_updates(
                h_[(i - 1) * (l_nY_ + 2) + j], h_[i * (l_nY_ + 2) + j],
                hu_[(i - 1) * (l_nY_ + 2) + j], hu_[i * (l_nY_ + 2) + j],
                b_[(i - 1) * (l_nY_ + 2) + j], b_[i * (l_nY_ + 2) + j],
                h_net_updates_left_[(i - 1) * (l_nY_ + 2) + j], h_net_updates_right_[(i - 1) * (l_nY_ + 2) + j],
                hu_net_updates_left_[(i - 1) * (l_nY_ + 2) + j], hu_net_updates_right_[(i - 1) * (l_nY_ + 2) + j],
                max_edge_speed);

            //update the maximum wave speed of the x-sweep
            max_x_wave_speed = std::max(max_x_wave_speed, max_edge_speed);
        }
    }

    // Steady states in x dimension?
    if (max_x_wave_speed > 0)
        update_unknowns_x_sweep(0.4 * l_dx_ / max_x_wave_speed);

    double max_y_wave_speed = 0.0;
    // for all rows
    #pragma omp parallel for reduction(max: max_y_wave_speed) schedule(runtime)
    for (int j = 1; j < l_nY_ + 2; j++)
    {
        // for all columns except most left and right ones (ghost layer)
        for (int i = 1; i < l_nX_ + 1; i++)
        {
            double max_edge_speed = 0.0;

            applications::exahype2::swe::adjoint::Adjoint_Solver::compute_net_updates(
                h_[i * (l_nY_ + 2) + (j - 1)], h_[i * (l_nY_ + 2) + j],
                hv_[i * (l_nY_ + 2) + (j - 1)], hv_[i * (l_nY_ + 2) + j],
                b_[i * (l_nY_ + 2) + (j - 1)], b_[i * (l_nY_ + 2) + j],
                h_net_updates_below_[i * (l_nY_ + 2) + (j - 1)], h_net_updates_above_[i * (l_nY_ + 2) + (j - 1)],
                hv_net_updates_below_[i * (l_nY_ + 2) + (j - 1)], hv_net_updates_above_[i * (l_nY_ + 2) + (j - 1)],
                max_edge_speed);

            //update the maximum wave speed of the x-sweep
            max_y_wave_speed = std::max(max_y_wave_speed, max_edge_speed);
        }
    }

    max_wave_speed = std::max(max_x_wave_speed, max_y_wave_speed);
    if(max_wave_speed > 0.00001)
    {
        double max_time_step = 0.4 * l_dx_ / max_wave_speed;
        update_unknowns_y_sweep(max_time_step);
        return max_time_step;
    }
    return max_wave_speed;
}

void applications::exahype2::swe::adjoint::DimensionalSplitting::update_unknowns_x_sweep(double dt)
{
    // update cells x-sweep
    const double factordx = dt / l_dx_;
    // for all inner columns
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 1; i < l_nX_ + 1; i++)
    {
        // for all rows
        for (int j = 0; j < l_nY_ + 2; j++)
        {
            // -dx/dt * (Update of left cell to the right + Update of right cell to the left)
            h_[i * (l_nY_ + 2) + j] -= factordx * (h_net_updates_right_[(i - 1) * (l_nY_ + 2) + j] + h_net_updates_left_[i * (l_nY_ + 2) + j]);
            hu_[i * (l_nY_ + 2) + j] -= factordx * (hu_net_updates_right_[(i-1) * (l_nY_ + 2) + j] + hu_net_updates_left_[i * (l_nY_ + 2) + j]);
        }
    }
}

void applications::exahype2::swe::adjoint::DimensionalSplitting::update_unknowns_y_sweep(double dt)
{
    // update cells x-sweep
    const double factor_dy = dt / l_dy_;
    // for all inner columns
    #pragma omp parallel for collapse(2) schedule(static)
     for (int j = 1; j < l_nY_ + 1; j++)
    {
        // for all inner columns
        for (int i = 1; i < l_nY_ + 1; i++)
        {
            // -dy/dt * (Update of cell above to below + Update of cell below to above)
            h_[i * (l_nY_ + 2) + j] -= factor_dy * (h_net_updates_above_[i * (l_nY_ + 2) + (j - 1)] + h_net_updates_below_[i * (l_nY_ + 2) + j]);
            hv_[i * (l_nY_ + 2) + j] -= factor_dy * (hv_net_updates_above_[i * (l_nY_ + 2) + (j - 1)] + hv_net_updates_below_[i * (l_nY_ + 2) + j]);
        }
    }
}