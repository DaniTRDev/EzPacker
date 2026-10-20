import React, { useMemo } from 'react';
import { useCompilerStore, PassFilterOption } from '../store/compilerStore';
import { CompilerPassStage } from '../types/compiler';
import {
  Layers,
  Sparkles,
  Search,
  CheckCircle2,
  Clock,
  Filter,
  Flame,
  ArrowRight,
  Code2,
} from 'lucide-react';

const STAGE_COLORS: Record<CompilerPassStage, { bg: string; text: string; border: string; label: string }> = {
  Verification: {
    bg: 'bg-zinc-800/80',
    text: 'text-zinc-300',
    border: 'border-zinc-700/60',
    label: 'Verify',
  },
  MiddleEnd: {
    bg: 'bg-sky-950/60',
    text: 'text-sky-300',
    border: 'border-sky-800/50',
    label: 'Middle',
  },
  Legalization: {
    bg: 'bg-amber-950/60',
    text: 'text-amber-300',
    border: 'border-amber-800/50',
    label: 'Legalize',
  },
  TargetLowering: {
    bg: 'bg-purple-950/60',
    text: 'text-purple-300',
    border: 'border-purple-800/50',
    label: 'Lowering',
  },
};

export const PassPipelineNav: React.FC = () => {
  const {
    result,
    selectedPassIndex,
    setSelectedPassIndex,
    passFilter,
    setPassFilter,
    passSearchQuery,
    setPassSearchQuery,
  } = useCompilerStore();

  const passes = result?.pipeline?.passes || [];

  const totalDuration = useMemo(() => {
    return passes.reduce((acc, p) => acc + (p.durationMs || 0), 0);
  }, [passes]);

  const mutatedCount = useMemo(() => {
    return passes.filter((p) => p.modifiedMir).length;
  }, [passes]);

  const filteredPasses = useMemo(() => {
    return passes
      .map((pass, originalIndex) => ({ pass, originalIndex }))
      .filter(({ pass }) => {
        // Filter by stage / mutation
        if (passFilter === 'mutations' && !pass.modifiedMir) {
          return false;
        }
        if (
          passFilter !== 'all' &&
          passFilter !== 'mutations' &&
          pass.stage !== passFilter
        ) {
          return false;
        }

        // Search query filter
        if (passSearchQuery.trim()) {
          const q = passSearchQuery.toLowerCase();
          const matchName = pass.name.toLowerCase().includes(q);
          const matchFn = pass.function ? pass.function.toLowerCase().includes(q) : false;
          const matchStage = pass.stage.toLowerCase().includes(q);
          if (!matchName && !matchFn && !matchStage) {
            return false;
          }
        }

        return true;
      });
  }, [passes, passFilter, passSearchQuery]);

  const filterButtons: Array<{ id: PassFilterOption; label: string; count?: number }> = [
    { id: 'all', label: 'All', count: passes.length },
    { id: 'mutations', label: 'Mutations (Δ)', count: mutatedCount },
    { id: 'Verification', label: 'Verify' },
    { id: 'MiddleEnd', label: 'Middle' },
    { id: 'Legalization', label: 'Legalize' },
    { id: 'TargetLowering', label: 'Lowering' },
  ];

  return (
    <div className="h-full flex flex-col bg-zinc-950 border-r border-zinc-800/80 text-xs select-none">
      {/* Navigator Top Metrics Header */}
      <div className="p-2.5 bg-zinc-900/60 border-b border-zinc-800/80 flex flex-col space-y-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center space-x-1.5 font-semibold text-zinc-200">
            <Layers className="w-4 h-4 text-emerald-400" />
            <span>Pass Pipeline</span>
            <span className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-zinc-800 text-zinc-400 border border-zinc-700/60">
              {passes.length}
            </span>
          </div>

          <div className="flex items-center space-x-2 text-[10px] font-mono text-zinc-400">
            <span className="text-emerald-400 font-semibold">{totalDuration.toFixed(2)} ms</span>
          </div>
        </div>

        {/* Filter Pills */}
        <div className="flex items-center flex-wrap gap-1">
          {filterButtons.map((btn) => {
            const active = passFilter === btn.id;
            return (
              <button
                key={btn.id}
                onClick={() => setPassFilter(btn.id)}
                className={`px-2 py-0.5 rounded text-[10px] font-medium transition-all border ${
                  active
                    ? 'bg-emerald-950/70 text-emerald-300 border-emerald-600/70 font-semibold shadow-sm'
                    : 'bg-zinc-900/60 text-zinc-400 border-zinc-800/80 hover:text-zinc-200 hover:bg-zinc-850'
                }`}
              >
                <span>{btn.label}</span>
                {btn.count !== undefined && (
                  <span className={`ml-1 text-[9px] ${active ? 'text-emerald-400' : 'text-zinc-500'}`}>
                    ({btn.count})
                  </span>
                )}
              </button>
            );
          })}
        </div>

        {/* Search Bar */}
        <div className="relative">
          <Search className="w-3 h-3 text-zinc-500 absolute left-2 top-2 pointer-events-none" />
          <input
            type="text"
            value={passSearchQuery}
            onChange={(e) => setPassSearchQuery(e.target.value)}
            placeholder="Filter passes or @function..."
            className="w-full h-7 pl-6 pr-2 bg-zinc-950/80 border border-zinc-800/80 rounded text-[11px] text-zinc-200 placeholder:text-zinc-600 focus:outline-none focus:ring-1 focus:ring-emerald-500/50"
          />
        </div>
      </div>

      {/* Pass Items List */}
      <div className="flex-1 overflow-y-auto p-1.5 space-y-1 custom-scrollbar">
        {filteredPasses.length === 0 ? (
          <div className="p-4 text-center text-zinc-500 text-[11px]">
            No passes match the active filter.
          </div>
        ) : (
          filteredPasses.map(({ pass, originalIndex }) => {
            const isSelected = selectedPassIndex === originalIndex;
            const stageMeta = STAGE_COLORS[pass.stage] || STAGE_COLORS.Verification;

            return (
              <button
                key={`${pass.name}-${originalIndex}`}
                onClick={() => setSelectedPassIndex(originalIndex)}
                className={`w-full text-left p-2 rounded-md transition-all flex items-start justify-between border ${
                  isSelected
                    ? 'bg-emerald-950/40 border-emerald-500/60 shadow-sm text-zinc-100 ring-1 ring-emerald-500/20'
                    : 'bg-zinc-900/30 hover:bg-zinc-900/70 border-zinc-850 hover:border-zinc-750 text-zinc-300'
                }`}
              >
                <div className="flex flex-col space-y-1 min-w-0 flex-1 pr-2">
                  <div className="flex items-center space-x-1.5 flex-wrap">
                    {/* Index */}
                    <span className="font-mono text-[10px] text-zinc-500 font-bold shrink-0">
                      #{originalIndex + 1 < 10 ? `0${originalIndex + 1}` : originalIndex + 1}
                    </span>

                    {/* Stage Badge */}
                    <span
                      className={`text-[9px] uppercase px-1 py-0.2 rounded border font-mono font-medium shrink-0 ${stageMeta.bg} ${stageMeta.text} ${stageMeta.border}`}
                    >
                      {stageMeta.label}
                    </span>

                    {/* Function Target if function pass */}
                    {pass.function && (
                      <span className="text-[10px] px-1 py-0.2 rounded bg-zinc-800 text-zinc-400 border border-zinc-700/40 font-mono truncate max-w-[110px]">
                        @{pass.function}
                      </span>
                    )}

                    {/* Mutation Badge */}
                    {pass.modifiedMir && (
                      <span className="text-[9px] font-bold font-mono px-1 py-0.2 rounded bg-emerald-950/80 text-emerald-400 border border-emerald-700/60 flex items-center space-x-0.5 shrink-0">
                        <Sparkles className="w-2.5 h-2.5" />
                        <span>Δ MUTATED</span>
                      </span>
                    )}
                  </div>

                  {/* Pass Name */}
                  <span className="font-mono text-[11px] font-medium text-zinc-200 truncate">
                    {pass.name}
                  </span>
                </div>

                {/* Right: Metrics */}
                <div className="flex flex-col items-end space-y-0.5 shrink-0 font-mono text-[10px]">
                  <span className={`font-semibold ${pass.durationMs > 0.5 ? 'text-amber-400' : 'text-zinc-400'}`}>
                    {pass.durationMs.toFixed(3)} ms
                  </span>
                  <span className="text-zinc-500 text-[9px]">
                    {pass.instructionCount} instrs
                  </span>
                </div>
              </button>
            );
          })
        )}
      </div>
    </div>
  );
};
