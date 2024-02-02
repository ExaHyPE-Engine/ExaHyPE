#include "TaskNumber.h"

#include <sstream>

#include "tarch/Assertions.h"


const swift2::TaskNumber swift2::TaskNumber::Max{
  std::numeric_limits<int>::max(), swift2::TaskNumber::TaskAssociation::NumberOfEntries, MaxSpecies};


const swift2::TaskNumber swift2::TaskNumber::NoDependencies{
  tarch::multicore::NoDependencies, TaskAssociation::TouchVertexFirstTime, 0};


swift2::TaskNumber::TaskNumber(int number0, TaskAssociation number1, int number2):
  _taskCounter(number0),
  _taskAssociation(number1),
  _taskSpecies(number2) {
  if (_taskCounter == ::tarch::multicore::NoDependencies) {
    *this = NoDependencies;
  } else if (number0 != std::numeric_limits<int>::max()) {
    assertion1(number0 >= 0 or number0 == tarch::multicore::NoDependencies, toString());
    assertion1(number1 != TaskAssociation::NumberOfEntries, toString());
    assertion1(number2 >= 0, toString());
    assertion1(number0 <= Max._taskCounter, toString());
    assertion1(number2 <= Max._taskSpecies, toString());
  }
}


bool swift2::TaskNumber::operator<(const TaskNumber& rhs) const {
  return _taskCounter < rhs._taskCounter
         or _taskCounter == rhs._taskCounter and _taskAssociation == TaskAssociation::TouchVertexFirstTime
              and rhs._taskAssociation != TaskAssociation::TouchVertexFirstTime
         or _taskCounter == rhs._taskCounter and _taskAssociation == TaskAssociation::TouchCellFirstTime
              and rhs._taskAssociation == TaskAssociation::TouchVertexLastTime
         or _taskCounter == rhs._taskCounter and _taskAssociation == rhs._taskAssociation
              and _taskSpecies < rhs._taskSpecies;
}


bool swift2::TaskNumber::equals(const TaskNumber& rhs) const {
  return _taskCounter == rhs._taskCounter and _taskAssociation == rhs._taskAssociation
         and _taskSpecies == rhs._taskSpecies;
}


bool operator==(const swift2::TaskNumber& lhs, const swift2::TaskNumber& rhs) {
  return lhs.equals(rhs);
}


bool operator!=(const swift2::TaskNumber& lhs, const swift2::TaskNumber& rhs) {
  return not lhs.equals(rhs);
}


std::string swift2::TaskNumber::toString() const {
  if (_taskCounter == tarch::multicore::NoDependencies) {
    return "no-dep";
  } else {
    std::string result = "(" + std::to_string(_taskCounter) + ",";
    switch (_taskAssociation) {
    case TaskAssociation::TouchVertexFirstTime:
      result += "touch-vertex-first-time(0)";
      break;
    case TaskAssociation::TouchCellFirstTime:
      result += "touch-cell-first-time(1)";
      break;
    case TaskAssociation::TouchVertexLastTime:
      result += "touch-vertex-last-time(2)";
      break;
    default:
      result += "<undef>";
      break;
    }
    result += "," + std::to_string(_taskSpecies) + ")";
    return result;
  }
}


tarch::multicore::TaskNumber swift2::TaskNumber::flatten() const {
  if (_taskCounter == tarch::multicore::NoDependencies) {
    return tarch::multicore::NoDependencies;
  } else {
    return _taskCounter * (Max._taskSpecies * static_cast<int>(Max._taskAssociation))
           + static_cast<int>(_taskAssociation) * (Max._taskSpecies) + _taskSpecies;
  }
}


std::string swift2::toString(const std::set<swift2::TaskNumber>& numbers) {
  std::ostringstream msg;
  msg << "{";
  bool first = true;
  for (auto& p : numbers) {
    if (first) {
      first = false;
    } else {
      msg << ",";
    }
    msg << p.toString();
  }
  msg << "}";
  return msg.str();
}


int swift2::flatten(const TaskNumber& number) { return number.flatten(); }


std::set<int> swift2::flatten(const std::set<TaskNumber>& numbers) {
  std::set<int> result;
  for (auto& p : numbers) {
    if (p != swift2::TaskNumber::NoDependencies) {
      result.insert(p.flatten());
    }
  }
  return result;
}


std::set<::swift2::TaskNumber> swift2::getDependenciesForTask(
  const ::swift2::TaskNumber& task, PendingDependencies& pendingDependencies
) {
  std::set<::swift2::TaskNumber> result;
  PendingDependencies::iterator  p = pendingDependencies.begin();
  while (p != pendingDependencies.end()) {
    if (p->second == task) {
      result.insert(p->first);
      p = pendingDependencies.erase(p);
    } else
      p++;
  }
  return result;
}
