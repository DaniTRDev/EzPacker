import React, { useState } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { DiagnosticItem } from '../types/compiler';
import { stripAnsi } from '../utils/ansi';
import { Terminal, AlertCircle, AlertTriangle, Info, ChevronUp, ChevronDown, CheckCircle2 } from 'lucide-react';

export const DiagnosticsDrawer: React.FC = () => {
  const { result, jumpToDiagnostic, doxygenStatus } = useCompilerStore();
  const [isExpanded, setIsExpanded] = useState<boolean>(true);
  const [filter, setFilter] = useState<'all' | 'error' | 'warning' | 'trace'>('all');

  const diagnostics = [...(result?.diagnostics || [])];
  if (doxygenStatus && (doxygenStatus.status === 'not_installed' || doxygenStatus.status === 'error' || doxygenStatus.status === 'invalid_configuration')) {
    diagnostics.push({
      type: 'warning',
      component: 'DoxygenEngine',
      message: doxygenStatus.errorMessage || 'Doxygen documentation offline',
      notes: doxygenStatus.status === 'not_installed' ? ['Install Doxygen or set DOXYGEN_BIN environment variable to enable documentation inspection'] : [],
    });
  }

  const errorCount = diagnostics.filter(d => d.type === 'error').length;
  const warningCount = diagnostics.filter(d => d.type === 'warning').length;
  const traceCount = diagnostics.filter(d => d.type === 'trace' || d.type === 'debug').length;

  const filtered = diagnostics.filter(d => {
    if (filter === 'all') return true;
    if (filter === 'trace') return d.type === 'trace' || d.type === 'debug';
    return d.type === filter;
  });

  return (
    <div
      className={`border-t border-zinc-800 bg-zinc-950 flex flex-col transition-all duration-200 select-none ${
        isExpanded ? 'h-48' : 'h-8'
      }`}
    >
      {/* Drawer Header Bar */}
      <div
        onClick={() => setIsExpanded(!isExpanded)}
        className="h-8 px-3 bg-zinc-900/80 border-b border-zinc-800/60 flex items-center justify-between text-xs cursor-pointer hover:bg-zinc-850"
      >
        <div className="flex items-center space-x-3">
          <div className="flex items-center space-x-1.5 font-medium text-zinc-300">
            <Terminal className="w-3.5 h-3.5 text-zinc-400" />
            <span>Compiler Output & Diagnostics</span>
          </div>

          {/* Severity Counters */}
          <div className="flex items-center space-x-1.5 text-[11px] font-mono">
            {errorCount > 0 ? (
              <span className="flex items-center space-x-1 px-1.5 py-0.2 rounded bg-rose-950/60 text-rose-400 border border-rose-800/50">
                <AlertCircle className="w-3 h-3" />
                <span>{errorCount} {errorCount === 1 ? 'Error' : 'Errors'}</span>
              </span>
            ) : result?.success ? (
              <span className="flex items-center space-x-1 text-emerald-400">
                <CheckCircle2 className="w-3 h-3" />
                <span>Clean</span>
              </span>
            ) : null}

            {warningCount > 0 && (
              <span className="flex items-center space-x-1 px-1.5 py-0.2 rounded bg-amber-950/60 text-amber-400 border border-amber-800/50">
                <AlertTriangle className="w-3 h-3" />
                <span>{warningCount} {warningCount === 1 ? 'Warning' : 'Warnings'}</span>
              </span>
            )}

            {traceCount > 0 && (
              <span className="text-zinc-400">
                {traceCount} {traceCount === 1 ? 'trace' : 'traces'}
              </span>
            )}
          </div>
        </div>

        <div className="flex items-center space-x-2" onClick={(e) => e.stopPropagation()}>
          {/* Filter Chips */}
          {isExpanded && (
            <div className="flex items-center space-x-1 bg-zinc-950 p-0.5 rounded border border-zinc-800/80 text-[10px]">
              <button
                onClick={() => setFilter('all')}
                className={`px-2 py-0.5 rounded transition-colors ${
                  filter === 'all' ? 'bg-zinc-800 text-zinc-100 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                All ({diagnostics.length})
              </button>
              <button
                onClick={() => setFilter('error')}
                className={`px-2 py-0.5 rounded transition-colors ${
                  filter === 'error' ? 'bg-rose-950/80 text-rose-300 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                Errors ({errorCount})
              </button>
              <button
                onClick={() => setFilter('warning')}
                className={`px-2 py-0.5 rounded transition-colors ${
                  filter === 'warning' ? 'bg-amber-950/80 text-amber-300 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                Warnings ({warningCount})
              </button>
              <button
                onClick={() => setFilter('trace')}
                className={`px-2 py-0.5 rounded transition-colors ${
                  filter === 'trace' ? 'bg-zinc-800 text-zinc-100 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                Traces ({traceCount})
              </button>
            </div>
          )}

          <button
            onClick={() => setIsExpanded(!isExpanded)}
            className="p-1 text-zinc-400 hover:text-zinc-200"
          >
            {isExpanded ? <ChevronDown className="w-3.5 h-3.5" /> : <ChevronUp className="w-3.5 h-3.5" />}
          </button>
        </div>
      </div>

      {/* Diagnostics List */}
      {isExpanded && (
        <div className="flex-1 overflow-y-auto p-2 space-y-1.5 custom-scrollbar text-xs font-mono select-text">
          {filtered.length === 0 ? (
            <div className="text-zinc-400 text-center py-6 text-xs font-sans">
              No diagnostic entries to display for the current filter.
            </div>
          ) : (
            filtered.map((diag, idx) => {
              const isError = diag.type === 'error';
              const isWarning = diag.type === 'warning';

              return (
                <div
                  key={idx}
                  onClick={() => jumpToDiagnostic(diag)}
                  className={`p-2 rounded border cursor-pointer transition-all ${
                    isError
                      ? 'bg-rose-950/15 border-rose-900/40 hover:border-rose-700/60 hover:bg-rose-950/25'
                      : isWarning
                      ? 'bg-amber-950/15 border-amber-900/40 hover:border-amber-700/60 hover:bg-amber-950/25'
                      : 'bg-zinc-900/30 border-zinc-800/40 hover:border-zinc-700/60 hover:bg-zinc-900/60'
                  }`}
                >
                  <div className="flex items-start justify-between">
                    <div className="flex items-center space-x-2">
                      {isError ? (
                        <AlertCircle className="w-3.5 h-3.5 text-rose-400 shrink-0 mt-0.5" />
                      ) : isWarning ? (
                        <AlertTriangle className="w-3.5 h-3.5 text-amber-400 shrink-0 mt-0.5" />
                      ) : (
                        <Info className="w-3.5 h-3.5 text-zinc-400 shrink-0 mt-0.5" />
                      )}

                      <span className="text-[10px] uppercase font-bold px-1.5 py-0.5 rounded bg-zinc-800/80 text-zinc-300">
                        {diag.component}
                      </span>

                      <span className={`font-medium ${isError ? 'text-rose-200' : isWarning ? 'text-amber-200' : 'text-zinc-300'}`}>
                        {stripAnsi(diag.message)}
                      </span>
                    </div>

                    {diag.line !== undefined && (
                      <span className="text-[11px] text-zinc-400 font-mono shrink-0 ml-2 hover:underline">
                        Line {diag.line}:{diag.col || 1}
                      </span>
                    )}
                  </div>

                  {diag.sourceExcerpt && (
                    <pre className="mt-1.5 p-1.5 rounded bg-zinc-950/90 text-[11px] text-zinc-400 border border-zinc-800/60 overflow-x-auto whitespace-pre font-mono leading-relaxed">
                      {stripAnsi(diag.sourceExcerpt)}
                    </pre>
                  )}
                </div>
              );
            })
          )}
        </div>
      )}
    </div>
  );
};
