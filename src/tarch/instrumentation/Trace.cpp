// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Trace.h"

#if PEANO_TRACE

#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
  struct TraceScope {
    std::string name;
    long int    startTimeStampMicroseconds;
    long int    childDurationMicroseconds = 0;
  };

  struct TraceKey {
    int                      rank;
    int                      threadId;
    std::vector<std::string> path;

    bool operator<(const TraceKey& other) const {
      if (rank != other.rank) {
        return rank < other.rank;
      }
      if (threadId != other.threadId) {
        return threadId < other.threadId;
      }
      return path < other.path;
    }
  };

  struct TraceSummary {
    long int inclusiveDurationMicroseconds = 0;
    long int exclusiveDurationMicroseconds = 0;
    long int numberOfCalls                 = 0;
  };

  struct ThreadTraceData {
    int                                              rank;
    int                                              threadId;
    std::map<std::vector<std::string>, TraceSummary> entries;
  };

  std::vector<TraceScope>& getTraceStack() {
    thread_local std::vector<TraceScope> traceStack;
    return traceStack;
  }

  std::mutex& getThreadTraceDataRegistryMutex() {
    static std::mutex mutex;
    return mutex;
  }

  std::vector<ThreadTraceData*>& getThreadTraceDataRegistry() {
    static std::vector<ThreadTraceData*> registry;
    return registry;
  }

  ThreadTraceData& getThreadTraceData() {
    // Intentionally kept alive until process shutdown so final trace collection can read data from completed threads.
    thread_local ThreadTraceData* data = []() {
      auto* result = new ThreadTraceData{
        tarch::mpi::Rank::getInstance().getRank(),
        tarch::multicore::Core::getInstance().getCoreNumber(),
        {}
      };

      std::lock_guard<std::mutex> lock(getThreadTraceDataRegistryMutex());
      getThreadTraceDataRegistry().push_back(result);
      return result;
    }();

    return *data;
  }

  void reportTraceWarning(std::string_view message, std::string_view scopeName) {
    std::cerr << "Peano trace warning: " << message << " [" << scopeName << "]" << std::endl;
  }

  void reportTraceWarning(std::string_view message, std::string_view scopeName, std::string_view currentScopeName) {
    std::cerr
      << "Peano trace warning: " << message << " [leave=" << scopeName << ", current=" << currentScopeName << "]"
      << std::endl;
  }

  std::string escapeJson(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    static constexpr char HexDigits[] = "0123456789abcdef";

    for (const char c : value) {
      switch (c) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\b':
        result += "\\b";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          result += "\\u00";
          result += HexDigits[(static_cast<unsigned char>(c) >> 4) & 0x0f];
          result += HexDigits[static_cast<unsigned char>(c) & 0x0f];
        } else {
          result += c;
        }
        break;
      }
    }

    return result;
  }

  void appendJsonString(std::ostringstream& out, std::string_view value) { out << '"' << escapeJson(value) << '"'; }

  void appendMilliseconds(std::ostringstream& out, long int microseconds) {
    out << std::fixed << std::setprecision(3) << static_cast<double>(microseconds) / 1000.0;
  }

  void appendMicrosecondsAverage(std::ostringstream& out, long int microseconds, long int calls) {
    out << std::fixed << std::setprecision(3);
    if (calls == 0) {
      out << 0.0;
    } else {
      out << static_cast<double>(microseconds) / static_cast<double>(calls);
    }
  }

  void appendPath(std::ostringstream& out, const std::vector<std::string>& path) {
    out << '[';
    for (std::size_t i = 0; i < path.size(); i++) {
      if (i > 0) {
        out << ',';
      }
      appendJsonString(out, path[i]);
    }
    out << ']';
  }

  void appendTraceEntryPrefix(
    std::ostringstream&             out,
    const std::vector<std::string>& path,
    const TraceSummary&             summary
  ) {
    out << "{\"depth\":" << path.size() - 1 << ",\"name\":";
    appendJsonString(out, path.back());
    out << ",\"path\":";
    appendPath(out, path);
    out << ",\"calls\":" << summary.numberOfCalls << ",\"inclusive_us\":" << summary.inclusiveDurationMicroseconds
        << ",\"inclusive_ms\":";
    appendMilliseconds(out, summary.inclusiveDurationMicroseconds);
    out << ",\"exclusive_us\":" << summary.exclusiveDurationMicroseconds << ",\"exclusive_ms\":";
    appendMilliseconds(out, summary.exclusiveDurationMicroseconds);
    out << ",\"average_inclusive_us\":";
    appendMicrosecondsAverage(out, summary.inclusiveDurationMicroseconds, summary.numberOfCalls);
    out << ",\"average_exclusive_us\":";
    appendMicrosecondsAverage(out, summary.exclusiveDurationMicroseconds, summary.numberOfCalls);
  }

  double computePercentage(long int value, long int total) {
    return total == 0 ? 0.0 : 100.0 * static_cast<double>(value) / static_cast<double>(total);
  }

  void accumulateSummary(TraceSummary& target, const TraceSummary& source) {
    target.inclusiveDurationMicroseconds += source.inclusiveDurationMicroseconds;
    target.exclusiveDurationMicroseconds += source.exclusiveDurationMicroseconds;
    target.numberOfCalls += source.numberOfCalls;
  }

  class TraceSummaryFile {
  private:
    std::map<TraceKey, TraceSummary> _entries;
    bool                             _isClosed = false;

  public:
    ~TraceSummaryFile() { close(false); }

    void addSummary(TraceKey traceKey, const TraceSummary& summary) {
      if (_isClosed) {
        return;
      }

      TraceSummary& entry = _entries[std::move(traceKey)];
      entry.inclusiveDurationMicroseconds += summary.inclusiveDurationMicroseconds;
      entry.exclusiveDurationMicroseconds += summary.exclusiveDurationMicroseconds;
      entry.numberOfCalls += summary.numberOfCalls;
    }

    std::string toJsonFragment() const {
      std::map<int, std::vector<std::pair<TraceKey, TraceSummary>>> entriesByThread;
      std::map<std::vector<std::string>, TraceSummary>              entriesByPath;
      for (const auto& entry : _entries) {
        entriesByThread[entry.first.threadId].push_back(entry);
        accumulateSummary(entriesByPath[entry.first.path], entry.second);
      }

      const int rank = tarch::mpi::Rank::getInstance().getRank();

      long int topLevelInclusiveMicroseconds = 0;
      long int topLevelCalls                 = 0;
      for (const auto& [path, summary] : entriesByPath) {
        if (path.size() == 1) {
          topLevelInclusiveMicroseconds += summary.inclusiveDurationMicroseconds;
          topLevelCalls += summary.numberOfCalls;
        }
      }
      if (topLevelInclusiveMicroseconds == 0) {
        for (const auto& [path, summary] : entriesByPath) {
          topLevelInclusiveMicroseconds += summary.inclusiveDurationMicroseconds;
          topLevelCalls += summary.numberOfCalls;
        }
      }

      std::vector<std::pair<std::vector<std::string>, TraceSummary>> aggregateEntries;
      aggregateEntries.reserve(entriesByPath.size());
      for (const auto& entry : entriesByPath) {
        if (not entry.first.empty()) {
          aggregateEntries.push_back(entry);
        }
      }
      std::sort(aggregateEntries.begin(), aggregateEntries.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second.exclusiveDurationMicroseconds != rhs.second.exclusiveDurationMicroseconds) {
          return lhs.second.exclusiveDurationMicroseconds > rhs.second.exclusiveDurationMicroseconds;
        }
        if (lhs.second.inclusiveDurationMicroseconds != rhs.second.inclusiveDurationMicroseconds) {
          return lhs.second.inclusiveDurationMicroseconds > rhs.second.inclusiveDurationMicroseconds;
        }
        return lhs.first < rhs.first;
      });

      std::ostringstream out;
      out << "{\"rank\":" << rank << ",\"top_level_us\":" << topLevelInclusiveMicroseconds << ",\"top_level_ms\":";
      appendMilliseconds(out, topLevelInclusiveMicroseconds);
      out << ",\"top_level_calls\":" << topLevelCalls << ",\"summary\":[";

      bool hasAggregateEntry = false;
      for (const auto& [path, summary] : aggregateEntries) {
        if (hasAggregateEntry) {
          out << ',';
        }
        appendTraceEntryPrefix(out, path, summary);
        out
          << ",\"percent_of_top_level\":" << std::fixed << std::setprecision(2)
          << computePercentage(summary.inclusiveDurationMicroseconds, topLevelInclusiveMicroseconds)
          << ",\"self_percent_of_top_level\":" << std::fixed << std::setprecision(2)
          << computePercentage(summary.exclusiveDurationMicroseconds, topLevelInclusiveMicroseconds) << "}";
        hasAggregateEntry = true;
      }

      out << "],\"threads\":[";

      bool hasThread = false;
      for (auto& [threadId, threadEntries] : entriesByThread) {
        std::sort(threadEntries.begin(), threadEntries.end(), [](const auto& lhs, const auto& rhs) {
          return lhs.first.path < rhs.first.path;
        });

        long int threadInclusiveMicroseconds = 0;
        for (const auto& [key, summary] : threadEntries) {
          if (key.path.size() == 1) {
            threadInclusiveMicroseconds += summary.inclusiveDurationMicroseconds;
          }
        }
        if (threadInclusiveMicroseconds == 0) {
          for (const auto& [key, summary] : threadEntries) {
            threadInclusiveMicroseconds += summary.inclusiveDurationMicroseconds;
          }
        }

        if (hasThread) {
          out << ',';
        }
        out
          << "{\"thread\":" << threadId << ",\"inclusive_us\":" << threadInclusiveMicroseconds << ",\"inclusive_ms\":";
        appendMilliseconds(out, threadInclusiveMicroseconds);
        out << ",\"entries\":[";

        bool hasEntry = false;
        for (const auto& [key, summary] : threadEntries) {
          if (key.path.empty()) {
            continue;
          }

          const double percentOfThread     = threadInclusiveMicroseconds == 0
                                               ? 0.0
                                               : 100.0 * static_cast<double>(summary.inclusiveDurationMicroseconds)
                                               / static_cast<double>(threadInclusiveMicroseconds);
          const double selfPercentOfThread = threadInclusiveMicroseconds == 0
                                               ? 0.0
                                               : 100.0 * static_cast<double>(summary.exclusiveDurationMicroseconds)
                                                   / static_cast<double>(threadInclusiveMicroseconds);

          if (hasEntry) {
            out << ',';
          }
          appendTraceEntryPrefix(out, key.path, summary);
          out << ",\"percent_of_thread\":" << std::fixed << std::setprecision(2) << percentOfThread
              << ",\"self_percent_of_thread\":" << std::fixed << std::setprecision(2) << selfPercentOfThread << "}";

          hasEntry = true;
        }

        out << "]}";
        hasThread = true;
      }

      out << "]}";
      return out.str();
    }

    static std::string assembleTraceJson(const std::vector<std::string>& rankFragments) {
      std::ostringstream out;
      out << "{\"schema\":\"peanoTraceSummaryV2\",\"unit\":\"microseconds\",\"ranks\":[\n";

      bool hasRank = false;
      for (const std::string& fragment : rankFragments) {
        if (fragment.empty()) {
          continue;
        }
        if (hasRank) {
          out << ",\n";
        }
        out << fragment;
        hasRank = true;
      }

      out << "\n]}\n";
      return out.str();
    }

    static void writeTraceHtml(std::ostream& out, std::string_view traceJson) {
      out << R"peanoTraceHtml(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Peano Trace Summary</title>
<style>
:root{color-scheme:light;--ink:#17211f;--muted:#60706b;--line:#d9e2df;--bar:#2d7f73;--bar2:#b46a21;--bar-bg:#e8f1ee;--panel:#fbfcfb;--note:#fff7df;--note-line:#e2c56f;}
body{font-family:ui-sans-serif,system-ui,sans-serif;margin:2rem;color:var(--ink);background:#f4f7f5;}
h1{font-size:1.7rem;margin:0 0 .25rem 0;} p{color:var(--muted);margin:.25rem 0 1.5rem 0;max-width:76rem;line-height:1.45;}
section.rank{background:var(--panel);border:1px solid var(--line);border-radius:12px;margin:1rem 0;padding:1rem;box-shadow:0 1px 2px rgba(0,0,0,.04);}
section.thread{border-top:1px solid var(--line);padding:.75rem 0 1rem 0;} section.thread:last-child{padding-bottom:0;}
h2{font-size:1.2rem;margin:.1rem 0 .9rem 0;color:#27413b;} h3{font-size:.95rem;margin:.1rem 0 .6rem 0;color:#36514a;}
table{border-collapse:collapse;width:100%;font-size:.9rem;} th,td{border-bottom:1px solid var(--line);padding:.45rem .55rem;text-align:right;vertical-align:middle;}
th{font-size:.75rem;text-transform:uppercase;letter-spacing:.04em;color:var(--muted);background:#f7faf8;} td.name,th.name{text-align:left;}
details{margin-top:1rem;} summary{cursor:pointer;color:#36514a;font-weight:600;}
.scope{font-family:ui-monospace,SFMono-Regular,Consolas,monospace;white-space:nowrap;} .treeScope{display:inline-block;} .branch{color:var(--muted);margin-right:.35rem;} .path{display:block;color:var(--muted);font-size:.74rem;margin-top:.12rem;white-space:normal;}
.barcell{min-width:10rem;} .barbg{height:.55rem;background:var(--bar-bg);border-radius:99px;overflow:hidden;}
.bar{height:100%;background:linear-gradient(90deg,var(--bar),#7ab89c);border-radius:99px;} .bar.self{background:linear-gradient(90deg,var(--bar2),#d4a15b);}
.traceTree tbody tr:hover{background:#f8fbf9;} .traceTree tr.untraced td{color:var(--muted);} .muted{color:var(--muted);} .hint{margin:.25rem 0 .8rem 0;} .note{background:var(--note);border:1px solid var(--note-line);border-radius:10px;padding:.65rem .8rem;margin:.75rem 0;color:#5a4516;}
</style>
</head>
<body>
<h1>Peano Trace Summary</h1>
<p>The main table is a call tree aggregated over all threads on a rank. Top-level trace scopes are the outermost <span class="scope">traceEnter</span>/<span class="scope">traceLeave</span> pairs observed in the run, wherever the application placed them.</p>
<script type="application/json" id="traceData">
)peanoTraceHtml"
          << traceJson << R"peanoTraceHtml(</script>
<div id="content"></div>
<script>
const data=JSON.parse(document.getElementById('traceData').textContent);
const content=document.getElementById('content');
function num(v){return Number(v||0)}
function fixed(v,d=3){return num(v).toFixed(d)}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
function pct(v){const n=num(v); if(n>0&&n<0.01) return '&lt;0.01%'; return `${n.toFixed(n<10?2:1)}%`}
function msFromUs(us){return num(us)/1000.0}
function keyFor(row){return (row.path||[row.name]).join('\u001f')}
function displayPath(row){return (row.path||[row.name]).join(' / ')}
function normaliseRow(row){
  const path=(row.path&&row.path.length)?row.path:[row.name||''];
  const inclusiveUs=num(row.inclusive_us);
  const exclusiveUs=num(row.exclusive_us);
  const calls=num(row.calls);
  return {
    ...row,
    path,
    name:row.name||path[path.length-1]||'',
    depth:num(row.depth??path.length-1),
    calls,
    inclusive_us:inclusiveUs,
    exclusive_us:exclusiveUs,
    inclusive_ms:('inclusive_ms' in row)?num(row.inclusive_ms):msFromUs(inclusiveUs),
    exclusive_ms:('exclusive_ms' in row)?num(row.exclusive_ms):msFromUs(exclusiveUs),
    average_inclusive_us:('average_inclusive_us' in row)?num(row.average_inclusive_us):(calls?inclusiveUs/calls:0),
    average_exclusive_us:('average_exclusive_us' in row)?num(row.average_exclusive_us):(calls?exclusiveUs/calls:0)
  };
}
function deriveRankRows(rank){
  const map=new Map();
  let topLevelUs=0;
  let topLevelCalls=0;
  for(const thread of rank.threads||[]){
    for(const raw of thread.entries||[]){
      const row=normaliseRow(raw);
      if(num(row.depth)===0){topLevelUs+=num(row.inclusive_us);topLevelCalls+=num(row.calls)}
      const key=keyFor(row);
      if(!map.has(key)){
        map.set(key,{depth:row.depth,name:row.name,path:row.path||[row.name],calls:0,inclusive_us:0,exclusive_us:0});
      }
      const target=map.get(key);
      target.calls+=num(row.calls);
      target.inclusive_us+=num(row.inclusive_us);
      target.exclusive_us+=num(row.exclusive_us);
    }
  }
  if(topLevelUs===0){
    for(const row of map.values()){topLevelUs+=num(row.inclusive_us);topLevelCalls+=num(row.calls)}
  }
  const rows=[...map.values()].map(normaliseRow);
  return {topLevelUs,topLevelCalls,rows};
}
function highFrequencyNote(rows){
  const hot=rows.filter(row=>num(row.calls)>1000000&&num(row.average_inclusive_us)<5);
  if(hot.length===0) return '';
  const names=[...new Set(hot.slice(0,4).map(row=>row.name))].map(esc).join(', ');
  return `<div class="note">High-frequency trace scopes detected (${names}). These rows have millions of sub-microsecond calls, so trace overhead can dominate very small scopes. Treat them as diagnostics; use coarser scopes or a sampling profiler for trustworthy inner-loop percentages.</div>`;
}
function computeRootUs(rows,totalUs){
  const given=num(totalUs);
  if(given>0) return given;
  const roots=rows.filter(row=>(row.path||[]).length<=1);
  const rootUs=roots.reduce((sum,row)=>sum+num(row.inclusive_us),0);
  return rootUs||rows.reduce((sum,row)=>sum+num(row.inclusive_us),0);
}
function buildTree(rows){
  const root={children:[]};
  const map=new Map();
  for(const raw of rows){
    const row=normaliseRow(raw);
    const key=keyFor(row);
    if(!map.has(key)){
      map.set(key,{...row,children:[]});
    } else {
      const target=map.get(key);
      target.calls+=num(row.calls);
      target.inclusive_us+=num(row.inclusive_us);
      target.exclusive_us+=num(row.exclusive_us);
      target.inclusive_ms=msFromUs(target.inclusive_us);
      target.exclusive_ms=msFromUs(target.exclusive_us);
      target.average_inclusive_us=target.calls?target.inclusive_us/target.calls:0;
      target.average_exclusive_us=target.calls?target.exclusive_us/target.calls:0;
    }
  }
  const ordered=[...map.values()].sort((a,b)=>a.path.length-b.path.length||displayPath(a).localeCompare(displayPath(b)));
  for(const node of ordered){
    const parentPath=node.path.slice(0,-1);
    const parentKey=parentPath.join('\u001f');
    const parent=parentKey?map.get(parentKey):root;
    (parent||root).children.push(node);
  }
  function sortChildren(node){
    node.children.sort((a,b)=>num(b.inclusive_us)-num(a.inclusive_us)||displayPath(a).localeCompare(displayPath(b)));
    for(const child of node.children){sortChildren(child)}
  }
  sortChildren(root);
  return root.children;
}
function appendUntracedRow(tbody,node,rootUs,level){
  if(num(node.exclusive_us)<=0) return;
  const topLevelPercent=num(rootUs)?100*num(node.exclusive_us)/num(rootUs):0;
  const tr=document.createElement('tr');
  const branch=level>0?'<span class="branch">|_</span>':'';
  tr.className='untraced';
  tr.innerHTML=`<td class="name"><span class="scope treeScope" title="Time inside ${esc(displayPath(node))} not covered by traced children" style="padding-left:${fixed(level*1.25,2)}rem">${branch}Untraced in this scope</span></td><td>${node.calls}</td><td>${fixed(node.exclusive_ms)}</td><td>${fixed(node.exclusive_ms)}</td><td>${pct(topLevelPercent)}</td><td>${fixed(node.average_exclusive_us)}</td><td class="barcell"><div class="barbg"><div class="bar self" style="width:${Math.min(100,num(topLevelPercent))}%"></div></div></td>`;
  tbody.appendChild(tr);
}
function appendTraceNode(tbody,node,rootUs,level){
  const topLevelPercent=num(rootUs)?100*num(node.inclusive_us)/num(rootUs):0;
  const tr=document.createElement('tr');
  const branch=level>0?'<span class="branch">|_</span>':'';
  tr.innerHTML=`<td class="name"><span class="scope treeScope" title="${esc(displayPath(node))}" style="padding-left:${fixed(level*1.25,2)}rem">${branch}${esc(node.name)}</span></td><td>${node.calls}</td><td>${fixed(node.inclusive_ms)}</td><td>${fixed(node.exclusive_ms)}</td><td>${pct(topLevelPercent)}</td><td>${fixed(node.average_inclusive_us)}</td><td class="barcell"><div class="barbg"><div class="bar" style="width:${Math.min(100,num(topLevelPercent))}%"></div></div></td>`;
  tbody.appendChild(tr);

  const childItems=node.children.map(child=>({kind:'child',node:child,weight:num(child.inclusive_us)}));
  if(node.children.length>0&&num(node.exclusive_us)>0){
    childItems.push({kind:'untraced',node,weight:num(node.exclusive_us)});
  }
  childItems.sort((a,b)=>num(b.weight)-num(a.weight)||(a.kind==='untraced'?1:0)-(b.kind==='untraced'?1:0));
  for(const item of childItems){
    if(item.kind==='untraced'){
      appendUntracedRow(tbody,item.node,rootUs,level+1);
    } else {
      appendTraceNode(tbody,item.node,rootUs,level+1);
    }
  }
}
function appendTreeRows(tbody,nodes,rootUs,level){
  for(const node of nodes){
    appendTraceNode(tbody,node,rootUs,level);
  }
}
function callTreeTable(rows,totalUs){
  const normalised=rows.map(normaliseRow);
  const rootUs=computeRootUs(normalised,totalUs);
  const tree=buildTree(normalised);
  const table=document.createElement('table');
  table.className='traceTree';
  table.innerHTML='<thead><tr><th class="name">Trace scope</th><th>Calls</th><th>Inclusive ms</th><th>Self ms</th><th>% top-level</th><th>Avg incl. us</th><th>Top-level share</th></tr></thead>';
  const tbody=document.createElement('tbody');
  appendTreeRows(tbody,tree,rootUs,0);
  table.appendChild(tbody);
  return table;
}
for(const rank of data.ranks||[]){
  const derived=deriveRankRows(rank);
  const rows=((rank.summary&&rank.summary.length)?rank.summary:derived.rows).map(normaliseRow);
  const topLevelUs=num(rank.top_level_us)||derived.topLevelUs;
  const topLevelCalls=num(rank.top_level_calls)||derived.topLevelCalls;
  const rankSection=document.createElement('section');
  rankSection.className='rank';
  rankSection.innerHTML=`<h2>Rank ${rank.rank} <span class="muted">(${(rank.threads||[]).length} thread(s), ${fixed(msFromUs(topLevelUs))} ms accumulated top-level trace work, ${topLevelCalls} top-level calls)</span></h2><h3>Trace call tree</h3><p class="hint">Rows are sorted by inclusive time among siblings. Percentages and bars are always relative to the accumulated top-level trace work. For any expanded parent, its children plus the <span class="scope">Untraced in this scope</span> row add up to the parent's top-level share.</p>${highFrequencyNote(rows)}`;
  rankSection.appendChild(callTreeTable(rows,topLevelUs));
  const details=document.createElement('details');
  details.innerHTML='<summary>Per-thread call trees</summary>';
  for(const thread of rank.threads||[]){
    const section=document.createElement('section');
    section.className='thread';
    const threadUs=num(thread.inclusive_us);
    const threadMs=('inclusive_ms' in thread)?num(thread.inclusive_ms):msFromUs(threadUs);
    const threadRows=(thread.entries||[]).map(normaliseRow);
    section.innerHTML=`<h3>Thread ${thread.thread} <span class="muted">(${fixed(threadMs)} ms top-level trace work)</span></h3>`;
    section.appendChild(callTreeTable(threadRows,threadUs));
    details.appendChild(section);
  }
  rankSection.appendChild(details);
  content.appendChild(rankSection);
}
</script>
</body>
</html>
)peanoTraceHtml";
    }

    static std::string traceFileStem(bool useCollective) {
#ifdef PEANO_WITH_MPI
      int mpiInitialised = 0;
      MPI_Initialized(&mpiInitialised);

      if (not useCollective and mpiInitialised and tarch::mpi::Rank::getInstance().isInitialised()
          and tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
        return "Trace-rank-" + std::to_string(tarch::mpi::Rank::getInstance().getRank());
      }
#else
      (void)useCollective;
#endif

      return "Trace";
    }

    static void writeTraceFiles(std::string_view traceJson, bool useCollective) {
      const std::string fileStem = traceFileStem(useCollective);
      {
        std::ofstream stream(fileStem + ".json", std::ios::out | std::ios::trunc);
        if (stream.is_open()) {
          stream << traceJson;
        }
      }
      {
        std::ofstream stream(fileStem + ".html", std::ios::out | std::ios::trunc);
        if (stream.is_open()) {
          writeTraceHtml(stream, traceJson);
        }
      }
    }

    static std::vector<std::string> gatherFragmentsToGlobalMaster(
      const std::string& localFragment,
      bool               useCollective
    ) {
#ifdef PEANO_WITH_MPI
      int mpiInitialised = 0;
      int mpiFinalised   = 0;
      MPI_Initialized(&mpiInitialised);
      if (mpiInitialised) {
        MPI_Finalized(&mpiFinalised);
      }

      if (useCollective and mpiInitialised and not mpiFinalised and tarch::mpi::Rank::getInstance().isInitialised()
          and tarch::mpi::Rank::getInstance().getRank() >= 0
          and tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
        const int localSize = static_cast<int>(localFragment.size());
        const int root      = tarch::mpi::Rank::getGlobalMasterRank();
        const int rank      = tarch::mpi::Rank::getInstance().getRank();
        const int ranks     = tarch::mpi::Rank::getInstance().getNumberOfRanks();

        std::vector<int> sizes(rank == root ? ranks : 0, 0);
        MPI_Gather(
          &localSize,
          1,
          MPI_INT,
          sizes.data(),
          1,
          MPI_INT,
          root,
          tarch::mpi::Rank::getInstance().getCommunicator()
        );

        std::vector<int>  displacements;
        std::vector<char> gatheredCharacters;
        if (rank == root) {
          displacements.resize(ranks, 0);
          int totalSize = 0;
          for (int i = 0; i < ranks; i++) {
            displacements[i] = totalSize;
            totalSize += sizes[i];
          }
          gatheredCharacters.resize(totalSize);
        }

        MPI_Gatherv(
          localFragment.data(),
          localSize,
          MPI_CHAR,
          gatheredCharacters.data(),
          sizes.data(),
          displacements.data(),
          MPI_CHAR,
          root,
          tarch::mpi::Rank::getInstance().getCommunicator()
        );

        if (rank == root) {
          std::vector<std::string> fragments;
          fragments.reserve(ranks);
          for (int i = 0; i < ranks; i++) {
            fragments.emplace_back(gatheredCharacters.data() + displacements[i], sizes[i]);
          }
          return fragments;
        }

        return {};
      }
#endif

      return {localFragment};
    }

    void close(bool useCollective) {
      if (_isClosed) {
        return;
      }

      if (useCollective or not _entries.empty()) {
        const std::string              localFragment = _entries.empty() ? std::string{} : toJsonFragment();
        const std::vector<std::string> rankFragments = gatherFragmentsToGlobalMaster(localFragment, useCollective);
        const bool                     hasTraceData  = std::any_of(
          rankFragments.begin(),
          rankFragments.end(),
          [](const std::string& fragment) { return not fragment.empty(); }
        );
        if (hasTraceData) {
          writeTraceFiles(assembleTraceJson(rankFragments), useCollective);
        }
      }

      _isClosed = true;
    }
  };

  TraceSummaryFile& getTraceSummaryFile() {
    static TraceSummaryFile traceSummaryFile;
    return traceSummaryFile;
  }
} // namespace

std::string tarch::instrumentation::Log::
  makeTraceScopeName(std::string_view functionSignature, std::string_view functionName, std::string_view fileName) {
  return makeScopeName(functionSignature, functionName, fileName);
}

std::string tarch::instrumentation::Log::makeTraceScopeName(
  std::string_view functionSignature,
  std::string_view functionName,
  std::string_view fileName,
  std::string_view traceScopeName
) {
  return traceScopeName.empty() ? makeScopeName(functionSignature, functionName, fileName) : std::string(traceScopeName);
}

void tarch::instrumentation::Log::enterTimedScope(std::string scopeName) {
  getTraceStack().push_back({std::move(scopeName), getTimeStamp() / 1000});
}

void tarch::instrumentation::Log::leaveTimedScope(std::string_view scopeName) {
  auto& traceStack = getTraceStack();

  if (traceStack.empty()) {
    reportTraceWarning("traceLeave() without matching traceEnter() ignored", scopeName);
    return;
  }

  if (traceStack.back().name != scopeName) {
    reportTraceWarning("traceLeave() scope mismatch ignored", scopeName, traceStack.back().name);
    return;
  }

  TraceScope scope = std::move(traceStack.back());
  traceStack.pop_back();

  const long int durationMicroseconds = getTimeStamp() / 1000 - scope.startTimeStampMicroseconds;
  if (durationMicroseconds < 0) {
    reportTraceWarning("trace scope has negative duration and was ignored", scope.name);
    return;
  }

  const long int accountedChildDurationMicroseconds = std::min(scope.childDurationMicroseconds, durationMicroseconds);
  const long int exclusiveDurationMicroseconds      = durationMicroseconds - accountedChildDurationMicroseconds;

  std::vector<std::string> path;
  path.reserve(traceStack.size() + 1);
  for (const TraceScope& parent : traceStack) {
    path.push_back(parent.name);
  }
  path.push_back(scope.name);

  if (not traceStack.empty()) {
    traceStack.back().childDurationMicroseconds += durationMicroseconds;
  }

  TraceSummary& summary = getThreadTraceData().entries[std::move(path)];
  summary.inclusiveDurationMicroseconds += durationMicroseconds;
  summary.exclusiveDurationMicroseconds += exclusiveDurationMicroseconds;
  summary.numberOfCalls++;
}

void tarch::instrumentation::Log::closeTrace(bool useCollective) {
  std::lock_guard<std::mutex> lock(getThreadTraceDataRegistryMutex());
  TraceSummaryFile&           traceSummaryFile = getTraceSummaryFile();
  for (const ThreadTraceData* threadData : getThreadTraceDataRegistry()) {
    for (const auto& [path, summary] : threadData->entries) {
      traceSummaryFile.addSummary({threadData->rank, threadData->threadId, path}, summary);
    }
  }
  traceSummaryFile.close(useCollective);
}

#endif
