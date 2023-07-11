#include "SWEAdjoint.hpp"

void inline applications::exahype2::swe::adjoint::SWEAdjoint::set_ghost_layer()
{
    // set ghost layer in 0th row and (l_nY_ + 1)th row
    #pragma omp parallel for schedule(static)
    for(int i = 0; i < l_nX_ + 2; i++)
    {
        h_[i * (l_nY_ + 2)] = h_[i * (l_nY_ + 2) + 1];
        hu_[i * (l_nY_ + 2)] = hu_[i * (l_nY_ + 2) + 1];
        hv_[i * (l_nY_ + 2)] = hv_[i * (l_nY_ + 2) + 1];

        h_[i * (l_nY_+ 2) + (l_nY_ + 1)] = h_[i * (l_nY_+ 2) + l_nY_];
        hu_[i * (l_nY_+ 2) + (l_nY_ + 1)] = hu_[i * (l_nY_+ 2) + l_nY_];
        hv_[i * (l_nY_+ 2) + (l_nY_ + 1)] = hv_[i * (l_nY_+ 2) + l_nY_];
    }

    // set ghost layer in 0th column and (l_nX_ + 1)th column
    #pragma omp parallel for schedule(static)
    for(int j = 0; j < l_nY_ + 2; j++)
    {
        h_[j] = h_[l_nY_ + 2 + j];
        hu_[j] = hu_[l_nY_ + 2 + j];
        hv_[j] = hv_[l_nY_ + 2 + j];

        h_[(l_nX_ + 1) * (l_nY_ + 2) + j] = h_[l_nX_ * (l_nY_ + 2) + j];
        hu_[(l_nX_ + 1) * (l_nY_ + 2) + j] = hu_[l_nX_ * (l_nY_ + 2) + j];
        hv_[(l_nX_ + 1) * (l_nY_ + 2) + j] = hv_[l_nX_ * (l_nY_ + 2) + j];
    }
}

bool inline applications::exahype2::swe::adjoint::SWEAdjoint::hump_criterion_square(double x, double y)
{
    double x_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(x, l_nX_, min_x_bathymetry_, max_x_bathymetry_);
    double y_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(y, l_nY_, min_y_bathymetry_, max_y_bathymetry_);
    double x_min = std::min(area_of_interest_coord_0_, area_of_interest_coord_1_);
    double x_max = std::max(area_of_interest_coord_0_, area_of_interest_coord_1_);
    double y_min = std::min(area_of_interest_coord_2_, area_of_interest_coord_3_);
    double y_max = std::max(area_of_interest_coord_2_, area_of_interest_coord_3_);
    return x_min < x_cdf && x_max > x_cdf && y_min < y_cdf && y_max > y_cdf;
}

bool inline applications::exahype2::swe::adjoint::SWEAdjoint::hump_criterion_circle(double x, double y)
{
    double x_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(x, l_nX_, min_x_bathymetry_, max_x_bathymetry_);
    double y_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(y, l_nY_, min_y_bathymetry_, max_y_bathymetry_);
    double x_center = area_of_interest_coord_0_;
    double y_center = area_of_interest_coord_1_;
    double radius = area_of_interest_coord_2_;
    double distance_x = x_cdf - x_center;
    double distance_y = y_cdf - y_center; 
    return sqrt((distance_x * distance_x) + (distance_y * distance_y)) < radius;
}

void inline applications::exahype2::swe::adjoint::SWEAdjoint::run_simulation()
{
    auto dimensional_splitting = DimensionalSplitting(l_nX_, l_nY_, cell_size_, cell_size_, h_, hu_, hv_, b_);
    double t = 0.0;
    size_t checkpoint = 0;
    netcdf_writer_->writeTimeStep(h_, hu_, hv_, t, checkpoint);
    checkpoint++;
    while(t < end_time_)
    {
        std::cout << "Simulating time: " << t << std::endl;
        t += dimensional_splitting.compute_numerical_fluxes();
        set_ghost_layer();
        if(checkpoints_ == nullptr || checkpoints_[checkpoint] <= t)
        {
            std::cout << "Writing time step to file" << std::endl;
            netcdf_writer_->writeTimeStep(h_, hu_, hv_, t, checkpoint);
            checkpoint++;
        }
    }
}

applications::exahype2::swe::adjoint::SWEAdjoint::SWEAdjoint(
    const char *output_file_name,
    const char *bathymetry_file_path, 
    double width_x, double width_y, 
    double cell_size,
    double end_time, 
    double area_of_interest_coord_0, double area_of_interest_coord_1, double area_of_interest_coord_2, double area_of_interest_coord_3, 
    double start_time, 
    size_t checkpoints, 
    size_t flush,
    const char* nc_key_x, const char* nc_key_y, const char* nc_key_bathymetry
    ):
    width_x_(width_x),
    width_y_(width_y),
    cell_size_(cell_size),
    end_time_(end_time),
    area_of_interest_coord_0_(area_of_interest_coord_0),
    area_of_interest_coord_1_(area_of_interest_coord_1),
    area_of_interest_coord_2_(area_of_interest_coord_2),
    area_of_interest_coord_3_(area_of_interest_coord_3),
    start_time_(start_time),
    number_of_checkpoints_(checkpoints)
{
    // calculate number of cells for each dimension
    l_nX_ = floor(width_x_ / cell_size_);
    l_nY_ = floor(width_y_ / cell_size_);

    topology_parser_ = new applications::exahype2::swe::parser::TopologyParser(bathymetry_file_path, l_nX_, l_nY_, nc_key_x, nc_key_y, nc_key_bathymetry);

    this->max_x_bathymetry_ = topology_parser_->get_max_x_bathymetry();
    this->min_x_bathymetry_ = topology_parser_->get_min_x_bathymetry();
    this->max_y_bathymetry_ = topology_parser_->get_max_y_bathymetry();
    this->min_y_bathymetry_ = topology_parser_->get_min_y_bathymetry();

    // create arrays to hold values of simulation
    h_ = new double[(l_nX_ + 2) * (l_nY_ + 2)];
    hu_ = new double[(l_nX_ + 2) * (l_nY_ + 2)](); // initialize to 0
    hv_ = new double[(l_nX_ + 2) * (l_nY_ + 2)](); // initialize to 0
    b_ = new double[(l_nX_ + 2) * (l_nY_ + 2)];

    // pick the correct hump criterion
    auto hump_criterion = area_of_interest_coord_3_ == INFINITY ? &applications::exahype2::swe::adjoint::SWEAdjoint::hump_criterion_circle : &applications::exahype2::swe::adjoint::SWEAdjoint::hump_criterion_square;

    // initialize bathymetry column wise
    // initialize water height using bathymetry (and hump criterion in area of interest)
    #pragma omp parallel for collapse(2) schedule(static)
    for(int i = 1; i < l_nX_ + 1; i++)
    {
        for(int j = 1; j < l_nY_ + 1; j++)
        {
            double x = i;
            double y = j;
            b_[i * (l_nY_ + 2) + j] = topology_parser_->sample_bathymetry(x, y);
            h_[i * (l_nY_ + 2) + j] = std::max((this->*hump_criterion)(x,y) * 1.0 - b_[i * (l_nY_ + 2) + j], 0.0);
        }
    }

    //python3 swe.py -sc Tsunami -s FVRusanovGlobalAdaptive -amr 0 -et 0.01 -bf netCDFInputs/tohoku_gebco_ucsb3_2000m_hawaii_bath.nc -df netCDFInputs/tohoku_gebco_ucsb3_2000m_hawaii_displ.nc --guided-amr --gamr-area-of-interest [-100000,100000] [-200000,200000]
    //python3 swe.py -sc Tsunami -s FVRusanovGlobalAdaptive -amr 0 -et 0.01 -bf netCDFInputs/gebco_2023_n65.0_s10.0_w-180.0_e-120.0.nc -df netCDFInputs/tohoku_gebco_ucsb3_2000m_hawaii_displ.nc --guided-amr --gamr-area-of-interest [-124.19083,41.74111], [1.0,inf] -nc-key-bath-x lon -nc-key-bath-y lat -nc-key-bath-bath elevation


    // delete topology parser, no longer needed
    delete topology_parser_;

    // set ghost layers for bathymetry
    // for 0th column and (l_nX_ + 1)th column
    #pragma omp parallel for schedule(static)
    for(int j = 0; j < l_nY_ + 2; j++)
    {
        b_[j] = b_[l_nY_ + 2 + j];
        b_[(l_nX_ + 1) * (l_nY_ + 2) + j] = b_[l_nX_ * (l_nY_ + 2) + j];
    }

    // set ghost layer for 0th row and (l_nY_ + 1)th row
    #pragma omp parallel for schedule(static)
    for(int i = 0; i < l_nX_ + 2; i++)
    {
        b_[i * (l_nY_ + 2)] = b_[i * (l_nY_ + 2) + 1];
        b_[i * (l_nY_ + 2) + (l_nY_ + 1)] = b_[i * (l_nY_ + 2) + l_nY_];
    }

    // set ghost layer for all other variables
    set_ghost_layer();

    // calculate checkpoints, if not every timestep results in output
    if(number_of_checkpoints_ > 0)
    {
        checkpoints_ = new double[number_of_checkpoints_ + 1];
        double time_width = end_time_ / number_of_checkpoints_;
        for(int i = 0; i <= number_of_checkpoints_; i++)
            checkpoints_[i] =  time_width * i;
    }
    
    netcdf_writer_ = new applications::exahype2::swe::adjoint::NetCDFWriter(output_file_name, bathymetry_file_path, l_nX_, l_nY_, cell_size_, b_, end_time, start_time_, flush);

    std::cout << "Starting simulation with the following parameters:" << std::endl
                << "width: [" << width_x_ << ", " << width_y_ << "]" << std::endl
                << "dimension: [" << l_nX_ << ", " << l_nY_ << "]" << std::endl
                << "cell_size: " << cell_size_ << std::endl
                << "end_time: " << end_time_ << std::endl
                << "hump_criterion: " << (hump_criterion == &applications::exahype2::swe::adjoint::SWEAdjoint::hump_criterion_circle ? "circle" : "square") << std::endl
                << "output_file_name: " << output_file_name << std::endl
                << "checkpoints: " << (number_of_checkpoints_ > 0 ? "" + number_of_checkpoints_ : "inf") << std::endl;
    this->run_simulation();
    netcdf_writer_->~NetCDFWriter();
    exit(0);
}

applications::exahype2::swe::adjoint::SWEAdjoint::~SWEAdjoint()
{
    delete h_;
    delete hu_;
    delete hv_;
    delete b_;
    delete netcdf_writer_;
    delete checkpoints_;
}