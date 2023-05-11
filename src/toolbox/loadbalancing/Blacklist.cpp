#include "Blacklist.h"
#include "peano4/parallel/SpacetreeSet.h"

#include <sstream>


tarch::logging::Log toolbox::loadbalancing::Blacklist::_log( "toolbox::loadbalancing::Blacklist" );


toolbox::loadbalancing::Blacklist::BlacklistData::BlacklistData(int treeNumber):
  lifetime(3),
  numberOfUnrefinedCells( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(treeNumber).getNumberOfLocalUnrefinedCells() ),
  numberOfSplits(1),
  numberOfDegeneratedChildren(0) {
}


toolbox::loadbalancing::Blacklist::Blacklist() {
}


std::string toolbox::loadbalancing::Blacklist::toString() const {
  std::ostringstream msg;
  msg << "{";
  if (_blacklist.empty()) {
    msg << "blacklist is empty";
  }
  else {
    for (auto p: _blacklist) {
      msg << "(#" << p.first << ":";

      if (p.second.lifetime>0) {
        msg << "lifetime=" << p.second.lifetime
            << ",#cells=" << p.second.numberOfUnrefinedCells;
      }
      else {
        msg << "inactive";
      }

      msg << ",#splits=" << p.second.numberOfSplits
          << ",#deg-children=" << p.second.numberOfDegeneratedChildren
          << ")";
    }
  }
  msg << "}";

  return msg.str();
}


bool toolbox::loadbalancing::Blacklist::isBlacklisted( int treeNumber ) const {
  return _blacklist.count(treeNumber)>0
     and _blacklist.at(treeNumber).lifetime>0;
}


void toolbox::loadbalancing::Blacklist::update() {
  for ( auto& p: _blacklist ) {
    if ( peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p.first).getRemovedEmptySubtree() ) {
      logInfo(
        "update()",
        "tree " << p.first << " has degenerated child. Set/keep on blacklist"
      );
      p.second.numberOfDegeneratedChildren++;
      p.second.lifetime++;
    }
    else if ( p.second.lifetime>0 ) {
      const int currentNumberOfCells = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p.first).getNumberOfLocalUnrefinedCells();
      if ( currentNumberOfCells==p.second.numberOfUnrefinedCells ) {
        logInfo( "updateBlacklist()", "tree " << p.first << " is on blacklist but seems to have failed to fork off cells successfully, as number of cells did not decrease since last split. Keep on blacklist" );
      }
      else if (p.second.lifetime==1) {
        logInfo( "updateBlacklist()", "remove tree " << p.first << " from blacklist (keep but set at inactive)" );
        p.second.lifetime = 0;
      }
      else if (p.second.lifetime>0) {
        p.second.lifetime--;
      }
    }
  }
}


void toolbox::loadbalancing::Blacklist::triggeredSplit( int newParent ) {
  if ( _blacklist.count(newParent)==0 ) {
    _blacklist.insert( std::pair<int,BlacklistData>(
      newParent,
      BlacklistData(newParent)
    ));
  }
  else if ( _blacklist.at(newParent).lifetime==0 ) {
    _blacklist.at(newParent) = BlacklistData(newParent);
  }
  else {
    logDebug(
      "triggerSplit()",
      "split local rank " << newParent << " though it had been on the blacklist. This happens usually if and only if a tree split multiple times in one iteration"
    );
    _blacklist.at(newParent).numberOfSplits++;
  }
}
