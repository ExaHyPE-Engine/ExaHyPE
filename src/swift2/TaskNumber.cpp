#include "TaskNumber.h"
#include "tarch/Assertions.h"
#include <sstream>


const swift2::TaskNumber swift2::TaskNumber::Max{
  std::numeric_limits<int>::max(),
  swift2::TaskNumber::TaskAssociation::NumberOfEntries,
  MaxSpecies
};


swift2::TaskNumber::TaskNumber(int number0, TaskAssociation number1, int number2):
  _taskCounter(number0),
  _taskAssociation(number1),
  _taskSpecies(number2) {
  assertion1( number0>=0 or number0==tarch::multicore::NoDependencies, toString() );
//  assertion1( number1!=TaskAssociation::NumberOfEntries, toString() );
  assertion1( number2>=0, toString() );
  assertion1( number0<=Max._taskCounter, toString() );
  assertion1( number2<=Max._taskSpecies, toString() );
}


swift2::TaskNumber::TaskNumber(int number):
  TaskNumber(tarch::multicore::NoDependencies,TaskAssociation::TouchVertexFirstTime,0) {
  assertionEquals( number, tarch::multicore::NoDependencies );
}


bool swift2::TaskNumber::operator<(const TaskNumber& rhs) const {
  return _taskCounter < rhs._taskCounter
      or _taskCounter == rhs._taskCounter and _taskAssociation==TaskAssociation::TouchVertexFirstTime and rhs._taskAssociation!=TaskAssociation::TouchVertexFirstTime
      or _taskCounter == rhs._taskCounter and _taskAssociation==TaskAssociation::TouchCellFirstTime   and rhs._taskAssociation==TaskAssociation::TouchVertexLastTime
      or _taskCounter == rhs._taskCounter and _taskAssociation==rhs._taskAssociation and _taskSpecies < rhs._taskSpecies;
}


std::string swift2::TaskNumber::toString() const {
  if (_taskCounter==tarch::multicore::NoDependencies) {
    return "no-dep";
  }
  else {
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
  if (_taskCounter==tarch::multicore::NoDependencies) {
    return tarch::multicore::NoDependencies;
  }
  else {
    return _taskCounter * (Max._taskSpecies*static_cast<int>(Max._taskAssociation)) +
           static_cast<int>(_taskAssociation) * (Max._taskSpecies) +
           _taskSpecies;
  }
}


std::string swift2::toString( const std::set<swift2::TaskNumber>&  numbers ) {
  std::ostringstream msg;
  msg << "{";
  bool first = true;
  for (auto& p: numbers) {
    if (first) {
      first = false;
    }
    else {
      msg << ",";
    }
    msg << p.toString();
  }
  msg << "}";
  return msg.str();
}


int swift2::flatten( const TaskNumber&  number ) {
  return number.flatten();
}


std::set<int> swift2::flatten( const std::set<TaskNumber>&  numbers ) {
  std::set<int> result;
  for (auto& p: numbers) {
    result.insert( p.flatten() );
  }
  return result;
}
