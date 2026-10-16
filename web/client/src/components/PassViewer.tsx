import React from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { Clock, Layers, CheckCircle2 } from 'lucide-react';

export const PassViewer: React.FC = () => {
  const { result } = useCompilerStore();

  if (!result || !result.passes || result.passes.length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-400 text-xs p-6 text-center">
        <Clock className="w-8 h-8 text-zinc-500 mb-2 stroke-[1.5]" />
        <p>No pass timing data recorded yet.</p>
        <p className="text-[11px] text-zinc-400 mt-1">Compile code to inspect pass sequence and durations.</p>
      </div>
    );
  }

  const passes = result.passes;
  const maxDuration = Math.max(...passes.map(p => p.durationMs), 0.001);
  const totalPassTime = passes.reduce((acc, p) => acc + p.durationMs, 0);

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Summary Header */}
      <div className="p-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <Layers className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Compiler Pass Pipeline</span>
          <span className="text-[11px] font-mono text-zinc-400 px-2 py-0.5 rounded bg-zinc-800/80 border border-zinc-700/60">
            {passes.length} Passes Executed
          </span>
        </div>
        <div className="flex items-center space-x-2 font-mono text-[11px]">
          <span className="text-zinc-400">Total Pass Duration:</span>
          <span className="font-semibold text-emerald-400">{totalPassTime.toFixed(3)} ms</span>
        </div>
      </div>

      {/* Waterfall Table */}
      <div className="flex-1 overflow-y-auto p-3 space-y-1.5 custom-scrollbar">
        {passes.map((p, idx) => {
          const pct = Math.max((p.durationMs / maxDuration) * 100, 2);
          const isSlowest = p.durationMs === maxDuration && passes.length > 2;

          return (
            <div
              key={`${p.name}-${idx}`}
              className="group flex items-center justify-between p-2 rounded-md bg-zinc-900/40 hover:bg-zinc-900 border border-zinc-800/50 hover:border-zinc-700/80 transition-all font-mono"
            >
              {/* Pass order & Name */}
              <div className="flex items-center space-x-2.5 min-w-[200px] flex-1">
                <span className="w-5 text-right text-[10px] text-zinc-400 select-none">
                  {idx + 1}
                </span>
                <CheckCircle2 className="w-3.5 h-3.5 text-emerald-500/70 shrink-0" />
                <span className="font-medium text-zinc-200 truncate">
                  {p.name}
                </span>
                {p.target && (
                  <span className="text-[10px] px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-400 border border-zinc-700/40">
                    fn @{p.target}
                  </span>
                )}
              </div>

              {/* Duration bar and value */}
              <div className="flex items-center space-x-3 w-48 sm:w-64 shrink-0">
                <div className="flex-1 h-2 rounded-full bg-zinc-800/80 overflow-hidden">
                  <div
                    className={`h-full rounded-full transition-all duration-300 ${
                      isSlowest ? 'bg-amber-400' : 'bg-emerald-500'
                    }`}
                    style={{ width: `${pct}%` }}
                  />
                </div>
                <span
                  className={`w-16 text-right text-[11px] font-medium ${
                    isSlowest ? 'text-amber-400 font-bold' : 'text-zinc-300'
                  }`}
                >
                  {p.durationMs.toFixed(3)} ms
                </span>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
