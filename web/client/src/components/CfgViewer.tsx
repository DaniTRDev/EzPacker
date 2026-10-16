import React, { useState, useMemo } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { Network, ArrowRight, Box, Code2, Compass, Layers, GitFork } from 'lucide-react';

export const CfgViewer: React.FC = () => {
  const { result, selectedAnalysisFunc, setSelectedAnalysisFunc } = useCompilerStore();
  const [highlightedBlock, setHighlightedBlock] = useState<string | null>(null);
  const [showInstructions, setShowInstructions] = useState<boolean>(true);

  if (!result || !result.functions || Object.keys(result.functions).length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-400 text-xs p-6 text-center">
        <Network className="w-8 h-8 text-zinc-500 mb-2 stroke-[1.5]" />
        <p>No Control Flow Graph (CFG) topology extracted.</p>
        <p className="text-[11px] text-zinc-400 mt-1">Compile code to inspect function CFG structure and basic blocks.</p>
      </div>
    );
  }

  const funcNames = Object.keys(result.functions);
  const activeFuncName = selectedAnalysisFunc && result.functions[selectedAnalysisFunc]
    ? selectedAnalysisFunc
    : funcNames[0];
  const activeFunc = result.functions[activeFuncName];

  // Stably sort blocks: entry block first, then sorted by blockNumber
  const blocks = useMemo(() => {
    const list = Object.values(activeFunc?.blocks || {});
    return list.sort((a, b) => {
      const isAEntry = a.id === 'entry' || a.id === 'entryPoint';
      const isBEntry = b.id === 'entry' || b.id === 'entryPoint';
      if (isAEntry && !isBEntry) return -1;
      if (!isAEntry && isBEntry) return 1;
      return (a.blockNumber ?? 0) - (b.blockNumber ?? 0);
    });
  }, [activeFunc]);

  // Compute total edges
  const totalEdges = useMemo(() => {
    return blocks.reduce((acc, b) => acc + (b.successors?.length || 0), 0);
  }, [blocks]);

  const scrollToBlock = (id: string) => {
    setHighlightedBlock(id);
    const el = document.getElementById(`cfg-block-${id}`);
    if (el) {
      el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  };

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Header with Function Switcher & Controls */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <Network className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Control Flow Graph (CFG)</span>
          <span className="text-[10px] text-zinc-400 bg-zinc-800/80 px-2 py-0.5 rounded-full font-mono">
            {blocks.length} {blocks.length === 1 ? 'block' : 'blocks'}
          </span>
          <span className="text-[10px] text-zinc-400 bg-zinc-800/80 px-2 py-0.5 rounded-full font-mono">
            {totalEdges} {totalEdges === 1 ? 'branch' : 'branches'}
          </span>
        </div>

        <div className="flex items-center space-x-3">
          {/* Toggle instructions */}
          <button
            onClick={() => setShowInstructions(!showInstructions)}
            className={`flex items-center space-x-1.5 px-2 py-1 rounded border text-[11px] transition-colors ${
              showInstructions
                ? 'bg-zinc-800/90 text-emerald-400 border-zinc-700'
                : 'bg-zinc-950 text-zinc-400 border-zinc-800 hover:text-zinc-200'
            }`}
            title="Toggle MIR instructions display within blocks"
          >
            <Code2 className="w-3.5 h-3.5" />
            <span>Instructions</span>
          </button>

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
                  @{name} ({Object.keys(result.functions[name]?.blocks || {}).length} blks)
                </option>
              ))}
            </select>
          </div>
        </div>
      </div>

      {/* Basic Blocks Topology Visualizer */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4 custom-scrollbar">
        {blocks.length === 0 ? (
          <div className="text-center text-zinc-400 py-10">
            No basic blocks detected for function @{activeFuncName}.
          </div>
        ) : (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {blocks.map((block) => {
              const isEntry = block.id === 'entry' || block.id === 'entryPoint' || (block.predecessors?.length === 0 && blocks.length > 1);
              const isExit = block.successors?.length === 0;
              const isHighlighted = highlightedBlock === block.id;

              return (
                <div
                  id={`cfg-block-${block.id}`}
                  key={block.id}
                  className={`rounded-lg border p-3.5 transition-all flex flex-col justify-between ${
                    isHighlighted
                      ? 'ring-2 ring-emerald-400 border-emerald-400 shadow-[0_0_20px_rgba(52,211,153,0.2)]'
                      : isEntry && !isExit
                      ? 'bg-emerald-950/20 border-emerald-500/40 shadow-[0_0_15px_rgba(34,197,94,0.08)]'
                      : isExit && !isEntry
                      ? 'bg-rose-950/20 border-rose-500/30'
                      : 'bg-zinc-900/40 border-zinc-800 hover:border-zinc-700'
                  }`}
                >
                  {/* Block Header */}
                  <div>
                    <div className="flex items-center justify-between pb-2 border-b border-zinc-800/80">
                      <div className="flex items-center space-x-1.5">
                        <Box className="w-3.5 h-3.5 text-zinc-400" />
                        <span className="font-mono font-bold text-zinc-100 text-sm">
                          %{block.id}
                        </span>
                        {block.blockNumber !== undefined && (
                          <span className="text-[10px] text-zinc-400 font-mono">
                            [id={block.blockNumber}]
                          </span>
                        )}
                      </div>
                      <div className="flex items-center space-x-1 font-mono text-[10px]">
                        {isEntry && (
                          <span className="px-1.5 py-0.5 rounded bg-emerald-500/20 text-emerald-300 font-semibold border border-emerald-500/30">
                            ENTRY
                          </span>
                        )}
                        {isExit && (
                          <span className="px-1.5 py-0.5 rounded bg-rose-500/20 text-rose-300 font-semibold border border-rose-500/30">
                            EXIT
                          </span>
                        )}
                        <span className="px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-400">
                          {block.instructions?.length ?? block.instructionCount ?? 0} instrs
                        </span>
                      </div>
                    </div>

                    {/* Predecessors */}
                    <div className="mt-2.5">
                      <span className="text-[10px] font-mono uppercase tracking-wider text-zinc-400">
                        Predecessors ({block.predecessors?.length || 0}):
                      </span>
                      <div className="flex flex-wrap gap-1 mt-1">
                        {block.predecessors && block.predecessors.length > 0 ? (
                          block.predecessors.map((p) => (
                            <button
                              key={p}
                              onClick={() => scrollToBlock(p)}
                              className="px-2 py-0.5 rounded bg-zinc-800/90 text-zinc-300 hover:text-emerald-400 hover:border-emerald-500/50 font-mono text-[11px] border border-zinc-700/50 transition-colors cursor-pointer"
                              title={`Jump to predecessor %${p}`}
                            >
                              %{p}
                            </button>
                          ))
                        ) : (
                          <span className="text-zinc-400 italic text-[11px]">
                            {isEntry ? 'None (Function Entry)' : 'None'}
                          </span>
                        )}
                      </div>
                    </div>

                    {/* Block Instructions */}
                    {showInstructions && block.instructions && block.instructions.length > 0 && (
                      <div className="mt-3 pt-2.5 border-t border-zinc-800/60">
                        <div className="text-[10px] font-mono uppercase tracking-wider text-zinc-400 mb-1.5 flex items-center justify-between">
                          <span>MIR Instructions:</span>
                          <span className="text-[9px] text-zinc-400 font-normal">{block.instructions.length} total</span>
                        </div>
                        <div className="bg-zinc-950/80 rounded border border-zinc-800/80 p-2 font-mono text-[11px] space-y-0.5 overflow-x-auto max-h-48 custom-scrollbar">
                          {block.instructions.map((inst, idx) => {
                            const trimmed = inst.trim();
                            const isBranch = /^(JMP|BR|RET|CALL|COND_BR)/i.test(trimmed) || /^(jmp|br|ret|call|cond_br)/.test(trimmed);
                            return (
                              <div
                                key={idx}
                                className={`whitespace-pre ${
                                  isBranch ? 'text-amber-300/90 font-semibold' : 'text-zinc-300'
                                }`}
                              >
                                {trimmed}
                              </div>
                            );
                          })}
                        </div>
                      </div>
                    )}
                  </div>

                  {/* Successors */}
                  <div className="mt-4 pt-2.5 border-t border-zinc-800/60">
                    <span className="text-[10px] font-mono uppercase tracking-wider text-zinc-400 flex items-center space-x-1">
                      <ArrowRight className="w-3 h-3 text-emerald-400" />
                      <span>Successor Branches ({block.successors?.length || 0}):</span>
                    </span>
                    <div className="flex flex-wrap gap-1.5 mt-1.5">
                      {block.successors && block.successors.length > 0 ? (
                        block.successors.map((s) => (
                          <button
                            key={s}
                            onClick={() => scrollToBlock(s)}
                            className="px-2 py-0.5 rounded bg-emerald-950/40 text-emerald-300 hover:bg-emerald-900/50 hover:border-emerald-400 font-mono text-[11px] border border-emerald-500/30 font-medium transition-colors cursor-pointer"
                            title={`Jump to successor %${s}`}
                          >
                            ➜ %{s}
                          </button>
                        ))
                      ) : (
                        <span className="text-zinc-400 italic text-[11px]">Terminal Block (RET / EXIT)</span>
                      )}
                    </div>
                  </div>
                </div>
              );
            })}
          </div>
        )}
      </div>
    </div>
  );
};
