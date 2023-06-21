#ifndef EXASEIS_SCENARIOEASI_HEADER
#define EXASEIS_SCENARIOEASI_HEADER
#include "Scenario.h"
#include "easi/ResultAdapter.h"
#include "easi/YAMLParser.h"
// #include "../include/easi/reader/asagi_reader.h"
#include "../easi/reader/asagi_reader.h"

struct ElasticMaterial {
  double lambda;
  double mu;
  double rho;
};

template <class Shortcuts, int basisSize>
class EasiScen: public Scenario<Shortcuts, basisSize> {

public:
  EasiScen(DomainInformation* info):
    Scenario<Shortcuts, basisSize>(info) {
    asagiReader = new AsagiReader("");
    easi::YAMLParser parser(3, asagiReader);
    model = parser.parse("material.yaml");
  };

  void initUnknownsPointwise(
    const double* const                          x,
    const tarch::la::Vector<Dimensions, double>& center,
    const double                                 t,
    const double                                 dt,
    double*                                      Q
  ) {
    Shortcuts s;

    ElasticMaterial                              material[1];
    easi::ArrayOfStructsAdapter<ElasticMaterial> adapter(material);
    adapter.addBindingPoint("rho", &ElasticMaterial::rho);
    adapter.addBindingPoint("mu", &ElasticMaterial::mu);
    adapter.addBindingPoint("lambda", &ElasticMaterial::lambda);

    easi::Query query(1, 3);
    query.group(0) = 0;
    query.x(0, 0)  = x[0];
    query.x(0, 1)  = x[1];
    query.x(0, 2)  = x[2];

    model->evaluate(query, adapter);

    Q[s.rho] = material[0].rho;
    Q[s.cp]  = sqrt((material[0].lambda + 2 * material[0].mu) / material[0].rho);
    Q[s.cs]  = sqrt(material[0].mu / material[0].rho);
  }
  void refinementCriteria(
    exahype2::solvers::aderdg::Solver* solver, std::vector<Refinement::RefinementCriterion<Shortcuts>*>& criteria
  ) override {}

private:
  AsagiReader*     asagiReader;
  easi::Component* model;
};
#endif
