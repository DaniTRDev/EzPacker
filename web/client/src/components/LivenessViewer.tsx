import React, { useState, useMemo } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { Activity, Box, Search, Layers } from 'lucide-react';

export const LivenessViewer: React.FC = () => {
  const { result, selectedAnalysisFunc, setSelectedAnalysisFunc } = useCompilerStore();
  const [searchTerm, setSearchTerm] = useState<string>('');

  if (!result || !result.functions || Object.keys(result.functions).length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-400 text-xs p-6 text-center">
        <Activity className="w-8 h-8 text-zinc-500 mb-2 stroke-[1.5]" />
        <p>No liveness analysis available.</p>
        <p className="text-[11px] text-zinc-400 mt-1">Compile code to evaluate live intervals and register allocation sets.</p>
      </div>
    );
  }

  const funcNames = Object.keys(result.functions);
  const activeFuncName = selectedAnalysisFunc && result.functions[selectedAnalysisFunc]
    ? selectedAnalysisFunc
    : funcNames[0];
  const activeFunc = result.functions[activeFuncName];

  const liveness = activeFunc?.liveness || {};

  // Combine blocks from both CFG blocks and liveness maps to guarantee full coverage
  const allBlockNames = useMemo(() => {
    if (!activeFunc) return [];
    const blockKeys = Object.keys(activeFunc.blocks || {});
    const liveKeys = Object.keys(activeFunc.liveness || {});
    const combined = Array.from(new Set([...blockKeys, ...liveKeys]));
    return combined.sort((a, b) => {
      if (a === 'entry' || a === 'entryPoint') return -1;
      if (b === 'entry' || b === 'entryPoint') return 1;
      const idA = activeFunc.blocks?.[a]?.blockNumber ?? 9999;
      const idB = activeFunc.blocks?.[b]?.blockNumber ?? 9999;
      return idA - idB;
    });
  }, [activeFunc]);

  const filteredBlockNames = useMemo(() => {
    if (!searchTerm.trim()) return allBlockNames;
    const term = searchTerm.toLowerCase();
    return allBlockNames.filter((bName) => {
      const data = liveness[bName] || { def: [], use: [], liveIn: [], liveOut: [] };
      return (
        bName.toLowerCase().includes(term) ||
        data.def?.some(r => r.toLowerCase().includes(term)) ||
        data.use?.some(r => r.toLowerCase().includes(term)) ||
        data.liveIn?.some(r => r.toLowerCase().includes(term)) ||
        data.liveOut?.some(r => r.toLowerCase().includes(term))
      );
    });
  }, [allBlockNames, searchTerm, liveness]);

  // Total registers tracked
  const totalRegsTracked = useMemo(() => {
    let count = 0;
    for (const bName of allBlockNames) {
      const data = liveness[bName];
      if (data) {
        count += (data.def?.length || 0) + (data.use?.length || 0) + (data.liveIn?.length || 0) + (data.liveOut?.length || 0);
      }
    }
    return count;
  }, [allBlockNames, liveness]);

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Header */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <Activity className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Register Liveness Analysis (`LivenessAnalysisPass`)</span>
          <span className="text-[10px] text-zinc-400 bg-zinc-800/80 px-2 py-0.5 rounded-full font-mono">
            {allBlockNames.length} {allBlockNames.length === 1 ? 'block' : 'blocks'}
          </span>
          {totalRegsTracked > 0 && (
            <span className="text-[10px] text-emerald-400/90 bg-emerald-950/40 border border-emerald-500/30 px-2 py-0.5 rounded-full font-mono">
              {totalRegsTracked} reg intervals
            </span>
          )}
        </div>

        <div className="flex items-center space-x-3">
          {/* Search/filter register */}
          <div className="relative flex items-center">
            <Search className="w-3.5 h-3.5 text-zinc-500 absolute left-2 pointer-events-none" />
            <input
              type="text"
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              placeholder="Filter blocks or registers..."
              className="h-7 pl-7 pr-2 font-mono text-xs bg-zinc-950 border border-zinc-800 rounded text-zinc-200 placeholder-zinc-500 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 w-44"
            />
          </div>

          {/* Function Selector */}
          <div className="flex items-center space-x-2">
            <span className="text-[11px] text-zinc-400">Function:</span>
            <select
              value={activeFuncName}
              onChange={(e) => setSelectedAnalysisFunc(e.target.value)}
              className="h-7 px-2 font-mono text-xs font-medium bg-zinc-950 border border-zinc-800 rounded text-zinc-200 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 cursor-pointer"
            >
              {funcNames.map((name) => (
                <option key={name} value={name}>
                  @{name}
                </option>
              ))}
            </select>
          </div>
        </div>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-4 custom-scrollbar">
        {/* Legend */}
        <div className="flex flex-wrap items-center gap-3 px-3 py-2 rounded-lg bg-zinc-900/40 border border-zinc-800/80 text-[11px]">
          <span className="text-zinc-400 font-semibold uppercase tracking-wider text-[10px]">Liveness Sets:</span>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded bg-amber-400/80"></span>
            <span className="text-zinc-300"><span className="font-semibold text-amber-300">Def</span> (Registers written in block)</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded bg-sky-400/80"></span>
            <span className="text-zinc-300"><span className="font-semibold text-sky-300">Use</span> (Registers read before written)</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded bg-emerald-400/80"></span>
            <span className="text-zinc-300"><span className="font-semibold text-emerald-300">LiveIn</span> (Live upon block entry)</span>
          </div>
          <div className="flex items-center space-x-1.5">
            <span className="w-2.5 h-2.5 rounded bg-purple-400/80"></span>
            <span className="text-zinc-300"><span className="font-semibold text-purple-300">LiveOut</span> (Live upon block exit)</span>
          </div>
        </div>

        {/* Liveness Cards per Block */}
        <div className="space-y-3">
          {filteredBlockNames.length === 0 ? (
            <div className="text-center text-zinc-400 py-10">
              {searchTerm ? 'No basic blocks or registers match your filter query.' : `No register intervals computed for @${activeFuncName}.`}
            </div>
          ) : (
            filteredBlockNames.map((bName) => {
              const rawLive = liveness[bName] || { def: [], use: [], liveIn: [], liveOut: [] };
              // Deduplicate lists for clean display
              const def = Array.from(new Set(rawLive.def || []));
              const use = Array.from(new Set(rawLive.use || []));
              const liveIn = Array.from(new Set(rawLive.liveIn || []));
              const liveOut = Array.from(new Set(rawLive.liveOut || []));

              return (
                <div
                  key={bName}
                  className="rounded-lg border border-zinc-800/80 bg-zinc-900/30 overflow-hidden hover:border-zinc-700/80 transition-colors"
                >
                  <div className="px-3.5 py-2 bg-zinc-900/80 border-b border-zinc-800/80 flex items-center justify-between">
                    <div className="flex items-center space-x-2">
                      <Box className="w-3.5 h-3.5 text-zinc-400" />
                      <span className="font-mono font-bold text-zinc-100 text-sm">%{bName}</span>
                    </div>
                    <div className="flex items-center space-x-2 font-mono text-[10px] text-zinc-400">
                      <span>{def.length} def</span>
                      <span>•</span>
                      <span>{use.length} use</span>
                      <span>•</span>
                      <span>{liveIn.length} in</span>
                      <span>•</span>
                      <span>{liveOut.length} out</span>
                    </div>
                  </div>

                  <div className="p-3.5 grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3 text-xs">
                    {/* Def */}
                    <div className="space-y-1.5 p-2 rounded bg-zinc-950/50 border border-zinc-800/50">
                      <div className="flex items-center justify-between">
                        <span className="text-[10px] font-mono uppercase text-amber-400 font-bold">
                          Def ({def.length})
                        </span>
                        <span className="text-[9px] text-zinc-500 font-mono">Written</span>
                      </div>
                      <div className="flex flex-wrap gap-1 font-mono text-[11px] min-h-[24px] items-center">
                        {def.length > 0 ? (
                          def.map((r, i) => (
                            <span key={i} className="px-1.5 py-0.5 rounded bg-amber-950/50 text-amber-300 border border-amber-800/50">
                              {r}
                            </span>
                          ))
                        ) : (
                          <span className="text-zinc-500 italic text-[11px]">Ø empty</span>
                        )}
                      </div>
                    </div>

                    {/* Use */}
                    <div className="space-y-1.5 p-2 rounded bg-zinc-950/50 border border-zinc-800/50">
                      <div className="flex items-center justify-between">
                        <span className="text-[10px] font-mono uppercase text-sky-400 font-bold">
                          Use ({use.length})
                        </span>
                        <span className="text-[9px] text-zinc-500 font-mono">Read</span>
                      </div>
                      <div className="flex flex-wrap gap-1 font-mono text-[11px] min-h-[24px] items-center">
                        {use.length > 0 ? (
                          use.map((r, i) => (
                            <span key={i} className="px-1.5 py-0.5 rounded bg-sky-950/50 text-sky-300 border border-sky-800/50">
                              {r}
                            </span>
                          ))
                        ) : (
                          <span className="text-zinc-500 italic text-[11px]">Ø empty</span>
                        )}
                      </div>
                    </div>

                    {/* LiveIn */}
                    <div className="space-y-1.5 p-2 rounded bg-zinc-950/50 border border-zinc-800/50">
                      <div className="flex items-center justify-between">
                        <span className="text-[10px] font-mono uppercase text-emerald-400 font-bold">
                          LiveIn ({liveIn.length})
                        </span>
                        <span className="text-[9px] text-zinc-500 font-mono">Entry</span>
                      </div>
                      <div className="flex flex-wrap gap-1 font-mono text-[11px] min-h-[24px] items-center">
                        {liveIn.length > 0 ? (
                          liveIn.map((r, i) => (
                            <span key={i} className="px-1.5 py-0.5 rounded bg-emerald-950/50 text-emerald-300 border border-emerald-800/50">
                              {r}
                            </span>
                          ))
                        ) : (
                          <span className="text-zinc-500 italic text-[11px]">Ø empty</span>
                        )}
                      </div>
                    </div>

                    {/* LiveOut */}
                    <div className="space-y-1.5 p-2 rounded bg-zinc-950/50 border border-zinc-800/50">
                      <div className="flex items-center justify-between">
                        <span className="text-[10px] font-mono uppercase text-purple-400 font-bold">
                          LiveOut ({liveOut.length})
                        </span>
                        <span className="text-[9px] text-zinc-500 font-mono">Exit</span>
                      </div>
                      <div className="flex flex-wrap gap-1 font-mono text-[11px] min-h-[24px] items-center">
                        {liveOut.length > 0 ? (
                          liveOut.map((r, i) => (
                            <span key={i} className="px-1.5 py-0.5 rounded bg-purple-950/50 text-purple-300 border border-purple-800/50">
                              {r}
                            </span>
                          ))
                        ) : (
                          <span className="text-zinc-500 italic text-[11px]">Ø empty</span>
                        )}
                      </div>
                    </div>
                  </div>
                </div>
              );
            })
          )}
        </div>
      </div>
    </div>
  );
};
