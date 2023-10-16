#include "CostMetrics.h"

#include "peano4/parallel/SpacetreeSet.h"


toolbox::loadbalancing::CostMetrics::CostMetrics():
  _localRankWeight(0.0),
  _globalWeight(0.0),
  _minimumOfMaximumOfRankWeights(0.0),
  _globalWeightIn(0.0),
  _globalWeightOut(0.0),
  _minimumOfMaximumOfRankWeightsIn(0.0),
  _minimumOfMaximumOfRankWeightsOut(0.0) {
  #ifdef Parallel
  _globalWeightRequest                  = nullptr;
  _lightestRankRequest                  = nullptr;
  _minimumOfMaximumOfRankWeightsRequest = nullptr;
  #endif
}


std::string toolbox::loadbalancing::CostMetrics::toString(const std::string& metricName) const {
  std::ostringstream msg;
  msg << "("
      << metricName
      << ",local-rank-weight=" << _localRankWeight
      << ",global-weight="     << _globalWeight
      << ",lightest-rank="     << _lightestRank._rank
      << ",lightest-ranks-weight=" << _lightestRank._weight
      << ",min-of-max-of-rank-weights=" << _minimumOfMaximumOfRankWeights
      << ")";
  return msg.str();
}


double toolbox::loadbalancing::CostMetrics::getCostOfLocalRank() const {
  double localCost = 0;
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();
  for (auto p: idsOfLocalSpacetrees) {
      localCost += getCostOfLocalTree(p);
  }
  return localCost;
}


void toolbox::loadbalancing::CostMetrics::waitForGlobalDataExchange() {
  #ifdef Parallel
  if (_globalSumRequest != nullptr ) {
    MPI_Wait( _globalWeightRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _lightestRankRequest, MPI_STATUS_IGNORE );
    MPI_Wait( _minimumOfMaximumOfRankWeightsRequest, MPI_STATUS_IGNORE );

    delete _globalWeightRequest;
    delete _lightestRankRequest;
    delete _minimumOfMaximumOfRankWeightsRequest;

    _globalWeightRequest                  = nullptr;
    _lightestRankRequest                  = nullptr;
    _minimumOfMaximumOfRankWeightsRequest = nullptr;
  }
  #endif
}


void toolbox::loadbalancing::CostMetrics::updateGlobalView() {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks()<=1) {
    _globalWeight           = _localRankWeight;
    _lightestRank._rank    = 0;
    _lightestRank._weight  = getCostOfLocalRank();
    _minimumOfMaximumOfRankWeights                = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size();
  }
  else {
    #ifdef Parallel
    waitForGlobalDataExchange();

    _globalWeight                      = _globalWeightIn;
    _lightestRank._rank                = _lightestRankIn._weight < _localRankWeight ? _lightestRankIn._rank : tarch::mpi::Rank::getInstance().getRank();
    _lightestRank._weight              = _lightestRankIn._weight;
    _minimumOfMaximumOfRankWeights     = _minimumOfMaximumOfRankWeightsIn;

    if ( _globalWeight < _localRankWeight ) {
      logInfo(
        "updateGlobalView()",
        "local number of cells (" << _localRankWeight << ") is bigger than global cell count (" << _globalWeight <<
        "). This usually happens if a forking tree has some pending refinement events and cannot refine anymore, as it has already spawned cells. Statistics might have inconsistent view of world"
      );
      _globalWeight       = _localRankWeight;
      _lightestRank._rank = tarch::mpi::Rank::getInstance().getRank();
    }

    _globalSumRequest                          = new MPI_Request();
    _globalLightestRankRequest                 = new MPI_Request();
    _globalMinimumOfMaximumTreesPerRankRequest = new MPI_Request();

    _globalWeightOut               = _localRankWeight;
    _lightestRankOut._weight       = _localRankWeight;
    _lightestRankOut._rank         = tarch::mpi::Rank::getInstance().getRank();

    MPI_Iallreduce(
      &_globalWeightOut,  // send
      &_globalWeightIn,   // receive
      1,             // count
      MPI_DOUBLE,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalSumRequest
    );
    MPI_Iallreduce(
      &_lightestRankOut,   // send
      &_lightestRankIn,    // receive
      1,                         // count
      MPI_DOUBLE_INT,
      MPI_MINLOC,
      tarch::mpi::Rank::getInstance().getCommunicator(),
      _globalLightestRankRequest
    );
    #endif
  }
}

double toolbox::loadbalancing::CostMetrics::getGlobalCost() const {
  return _globalWeight;
}


int toolbox::loadbalancing::CostMetrics::getLightestRank() const {
  assertion1( _lightestRank._rank>=0, toString() );
  assertion1( _lightestRank._rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(), toString() );
  return _lightestRank._rank;
}


double toolbox::loadbalancing::CostMetrics::getMinimumOfMaximumRankWeights() const {
  return _minimumOfMaximumOfRankWeights;
}
