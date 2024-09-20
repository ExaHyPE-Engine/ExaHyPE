#ifndef EXASEIS_SCENARIOFACTORY_HEADER
#define EXASEIS_SCENARIOFACTORY_HEADER
#include "Apatite.h"
#include "Easi.h"
#include "GaussianWave.h"
#include "Scenario.h"
// #include "GaussianHill.h"
#include "Hhs1.h"
#include "Hsp1.h"
// #include "Hhs1_Transposed.h"
#include "Loh1.h"
// #include "Loh1_Transposed.h"
#include <unordered_map>

#include "Sinusodial.h"
#include "TPV5.h"
#include "WholeSpaceProblem.h"
#include "Zugspitze.h"
#include "Zugspitze_noTopo.h"

class ScenarioFactory {
public:
  template <template <typename, int> class T, typename V, int B>
  static Scenario<V, B>* create(DomainInformation* info) {
    return new T<V, B>(info);
  }

  template <class VariableShortcuts, int basisSize>
  static Scenario<VariableShortcuts, basisSize>* createScenario(std::string& scenario_string, DomainInformation* info) {

    std::unordered_map<std::string, Scenario<VariableShortcuts, basisSize>* (*)(DomainInformation*)> name2enum{
      {"Apatite", &create<Apatite, VariableShortcuts, basisSize>},
      {"Easi", &create<EasiScen, VariableShortcuts, basisSize>},
      {"GaussianWave", &create<GaussianWave, VariableShortcuts, basisSize>},
      // {"GaussianHill", &create<GaussianHill, VariableShortcuts, basisSize>},
      {"Hsp1", &create<Hsp1, VariableShortcuts, basisSize>},
      {"Hhs1", &create<Hhs1, VariableShortcuts, basisSize>},
      // {"Hhs1_Transposed", &create<Hhs1_Transposed, VariableShortcuts, basisSize>},
      {"Loh1", &create<Loh1, VariableShortcuts, basisSize>},
      // {"Loh1_Transposed", &create<Loh1_Transposed, VariableShortcuts, basisSize>},
      {"Sinusodial", &create<Sinusodial, VariableShortcuts, basisSize>},
      {"TPV5", &create<TPV5, VariableShortcuts, basisSize>},
      {"WholeSpaceProblem", &create<WholeSpaceProblem, VariableShortcuts, basisSize>},
      {"Zugspitze", &create<Zugspitze, VariableShortcuts, basisSize>},
      {"Zugspitze_noTopo", &create<Zugspitze_noTopo, VariableShortcuts, basisSize>}};

    if (name2enum.find(scenario_string) == name2enum.end()) {
      std::stringstream ss;
      ss << "Unknown scenario: " << scenario_string;

      throw std::invalid_argument(ss.str());
    }

    return name2enum[scenario_string](info);
  }
};

#endif
