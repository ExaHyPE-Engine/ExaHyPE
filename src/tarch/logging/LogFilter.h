// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_LOGGING_LOG_FILTER_H_
#define _TARCH_LOGGING_LOG_FILTER_H_


#include <string>
#include <set>
#include <stack>


namespace tarch {
  namespace logging {
    class LogFilter;
  }
}


/**
 * Peano4's log filter is used by every single log instance. Please note that
 * the fourth generation of Peano allows users to filter only w.r.t. the class
 * name. You can't filter on the method level anymore.
 */
class tarch::logging::LogFilter {
  private:
  public:
    /**
     * Represents one entry of the filter list. Syntax:
     *
     * <log-filter target="debug" component="component-name" rank="int|*" switch="on|off" />
     */
    struct FilterListEntry {
      static const int AnyRank;

      static const std::string TargetAll;
      static const std::string TargetInfo;
      static const std::string TargetDebug;
      static const std::string TargetTrace;

      static const bool BlackListEntry;
      static const bool WhiteListEntry;

      /**
       * The message type target can be either "debug" or "info". Only messages
       * with corresponding target are filtered by the FilterListEntry.
       *
       * If left blank, all message types are targeted.
       */
      std::string _targetName;

      /**
       * Sometimes, one wants to block all log entries of one namespace of one
       * node (parallel case). In this case _rank holds the corresponding entry,
       * if all entries from one namespace have to be blocked, the value of
       * _rank equals AnyRank.
       */
      int         _rank;

      /**
       * Name of the namespace that should not be logged.
       */
      std::string _namespaceName;

      /**
       * If true, filter list entry is a filter list entry, otherwise white list.
       */
      bool        _isBlackEntry;

      bool operator<(const FilterListEntry& b) const;
      bool operator==(const FilterListEntry& b) const;
      bool operator!=(const FilterListEntry& b) const;

      /**
       * Construct filter list entry for one target without any
       */
      FilterListEntry( const std::string& targetName="", bool isBlackListEntry=false );
      FilterListEntry( const std::string& targetName, int rank, const std::string& className, bool isBlackListEntry );

      std::string toString() const;
    };

    typedef std::set<FilterListEntry> FilterList;

    FilterList  _filterlist;

    #if !defined(SharedMemoryParallelisation) && PeanoDebug>=1
    std::stack<std::string>  _indentTraces;
    #endif


    /**
     * By default, everything is on
     */
    LogFilter();


    /**
     * May not be const as it might write a warning itself
     *
     * Is public as some analysis frameworks check explicitly whether these
     * features are switched on.
     */
    bool filterOut(
      const std::string& targetName,
      const std::string& className
    );

  public:
    static LogFilter& getInstance();

    /**
     * Add one filter list entry
     *
     * If you wanna switch on the logging globally, please add
     *
     * tarch::logging::ChromeTraceFileLogger::getInstance().addFilterListEntry(tarch::logging::ChromeTraceFileLogger::FilterListEntry());
     *
     * to your configuration.
     */
    void addFilterListEntry( const FilterListEntry& entry);
    void addFilterListEntries( const FilterList&    entries);
    void clearFilterList();
    void printFilterListToCout() const;

    bool writeDebug(const std::string& trace);
    bool writeInfo(const std::string& trace);
    bool writeTrace(const std::string& trace);
};


#endif
