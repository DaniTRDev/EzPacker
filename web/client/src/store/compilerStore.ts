import { create } from 'zustand';
import {
  CompileRequest,
  CompileResponse,
  DiagnosticItem,
  ExampleFile,
  TargetInfo,
  DoxygenDocEntity,
  DoxygenStatus,
  CompilerPassStage,
} from '../types/compiler';

export type PassFilterOption = 'all' | 'mutations' | CompilerPassStage;
export type PassInspectorMode = 'snapshot' | 'diff_prev' | 'diff_initial' | 'cfg' | 'liveness';

interface CompilerState {
  code: string;
  targetTriple: string;
  optLevel: 'O0' | 'O1' | 'O2' | 'Os';
  features: string[];
  isPositionIndependent: boolean;
  enableRtti: boolean;
  autoCompile: boolean;

  isCompiling: boolean;
  result: CompileResponse | null;
  error: string | null;

  // View Organization
  viewMode: 'passes' | 'pipeline' | 'analysis' | 'symbols';
  pipelineStage: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm' | 'object';
  analysisTab: 'cfg' | 'liveness' | 'stack' | 'passes';

  // Compiler Developer Workbench State (Pass-by-Pass Inspection)
  selectedPassIndex: number;
  passFilter: PassFilterOption;
  passSearchQuery: string;
  passInspectorMode: PassInspectorMode;
  diffTargetPassIndex: number | null;

  // Legacy tab compatibility
  activeTab: 'asm' | 'stages' | 'diff' | 'passes' | 'cfg' | 'liveness' | 'stack';
  selectedStage: 'mir' | 'legalizedMir' | 'loweredMir';
  diffStageA: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm';
  diffStageB: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm';

  examples: ExampleFile[];
  selectedExampleId: string | null;
  targetInfo: TargetInfo | null;
  selectedAnalysisFunc: string | null;

  // Doxygen and Symbol Inspection
  doxygenStatus: DoxygenStatus | null;
  isGeneratingDocs: boolean;
  selectedSymbolName: string | null;
  selectedDocEntity: DoxygenDocEntity | null;
  symbolSearchQuery: string;

  jumpToLocation: { line: number; col: number } | null;

  // Actions
  setCode: (code: string) => void;
  setTargetTriple: (triple: string) => void;
  setOptLevel: (level: 'O0' | 'O1' | 'O2' | 'Os') => void;
  toggleFeature: (feature: string) => void;
  setIsPositionIndependent: (pic: boolean) => void;
  setEnableRtti: (rtti: boolean) => void;
  setAutoCompile: (auto: boolean) => void;

  setViewMode: (mode: 'passes' | 'pipeline' | 'analysis' | 'symbols') => void;
  setPipelineStage: (stage: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm' | 'object') => void;
  setAnalysisTab: (tab: 'cfg' | 'liveness' | 'stack' | 'passes') => void;

  setSelectedPassIndex: (index: number) => void;
  setPassFilter: (filter: PassFilterOption) => void;
  setPassSearchQuery: (query: string) => void;
  setPassInspectorMode: (mode: PassInspectorMode) => void;
  setDiffTargetPassIndex: (index: number | null) => void;

  setActiveTab: (tab: 'asm' | 'stages' | 'diff' | 'passes' | 'cfg' | 'liveness' | 'stack') => void;
  setSelectedStage: (stage: 'mir' | 'legalizedMir' | 'loweredMir') => void;
  setDiffStages: (a: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm', b: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm') => void;
  setSelectedAnalysisFunc: (funcName: string) => void;
  selectExample: (id: string) => void;
  jumpToDiagnostic: (diag: DiagnosticItem) => void;
  clearJumpLocation: () => void;

  inspectSymbol: (name: string) => Promise<void>;
  clearSelectedSymbol: () => void;
  setSymbolSearchQuery: (query: string) => void;
  generateDocs: () => Promise<void>;
  checkDoxygenStatus: () => Promise<void>;

  compile: () => Promise<void>;
  loadInitialData: () => Promise<void>;
}

let compileDebounceTimeout: any = null;

const DEFAULT_MIR_CODE = `// EzPacker Arithmetic & SSA Optimization Demo
fn @hash_step32(i32 %val, i32 %seed) -> i32 {
entry:
    %c_const = MOV i32 40503;
    %sum = ADD i32 %val, %c_const;
    %prod = IMUL i32 %sum, %seed;
    %diff = SUB i32 %prod, %val;
    RET i32 %diff;
}

fn @main() -> i32 {
entry:
    %arg1 = MOV i32 1024;
    %arg2 = MOV i32 31;
    %h = ADD i32 %arg1, %arg2;
    %bias = MOV i32 7;
    %res = SUB i32 %h, %bias;
    RET i32 %res;
}
`;

export const useCompilerStore = create<CompilerState>((set, get) => ({
  code: DEFAULT_MIR_CODE,
  targetTriple: 'x86_64-pc-windows-msvc',
  optLevel: 'O0',
  features: ['+avx2', '+sse2'],
  isPositionIndependent: false,
  enableRtti: true,
  autoCompile: true,

  isCompiling: false,
  result: null,
  error: null,

  // Primary view organization (defaults to 'passes' for rich compiler workbench)
  viewMode: 'passes',
  pipelineStage: 'asm',
  analysisTab: 'cfg',

  // Compiler Developer Workbench State
  selectedPassIndex: 0,
  passFilter: 'all',
  passSearchQuery: '',
  passInspectorMode: 'snapshot',
  diffTargetPassIndex: null,

  activeTab: 'asm',
  selectedStage: 'mir',
  diffStageA: 'mir',
  diffStageB: 'loweredMir',

  examples: [],
  selectedExampleId: null,
  targetInfo: null,
  selectedAnalysisFunc: null,

  doxygenStatus: null,
  isGeneratingDocs: false,
  selectedSymbolName: null,
  selectedDocEntity: null,
  symbolSearchQuery: '',

  jumpToLocation: null,

  setCode: (code: string) => {
    set({ code });
    const { autoCompile, compile } = get();
    if (autoCompile) {
      if (compileDebounceTimeout) clearTimeout(compileDebounceTimeout);
      compileDebounceTimeout = setTimeout(() => {
        compile();
      }, 350);
    }
  },

  setTargetTriple: (triple: string) => {
    set({ targetTriple: triple });
    get().compile();
  },

  setOptLevel: (optLevel: 'O0' | 'O1' | 'O2' | 'Os') => {
    set({ optLevel });
    get().compile();
  },

  toggleFeature: (feature: string) => {
    const { features } = get();
    const updated = features.includes(feature)
      ? features.filter(f => f !== feature)
      : [...features, feature];
    set({ features: updated });
    get().compile();
  },

  setIsPositionIndependent: (isPositionIndependent: boolean) => {
    set({ isPositionIndependent });
    get().compile();
  },

  setEnableRtti: (enableRtti: boolean) => {
    set({ enableRtti });
    get().compile();
  },

  setAutoCompile: (autoCompile: boolean) => {
    set({ autoCompile });
  },

  setViewMode: (viewMode) => {
    set({ viewMode });
  },

  setPipelineStage: (pipelineStage) => {
    set({ pipelineStage, viewMode: 'pipeline' });
  },

  setAnalysisTab: (analysisTab) => {
    set({ analysisTab, viewMode: 'analysis' });
  },

  setSelectedPassIndex: (selectedPassIndex: number) => {
    set({ selectedPassIndex });
  },

  setPassFilter: (passFilter: PassFilterOption) => {
    set({ passFilter });
  },

  setPassSearchQuery: (passSearchQuery: string) => {
    set({ passSearchQuery });
  },

  setPassInspectorMode: (passInspectorMode: PassInspectorMode) => {
    set({ passInspectorMode });
  },

  setDiffTargetPassIndex: (diffTargetPassIndex: number | null) => {
    set({ diffTargetPassIndex });
  },

  setActiveTab: (activeTab) => {
    set({ activeTab });
  },

  setSelectedStage: (selectedStage) => {
    set({ selectedStage });
  },

  setDiffStages: (diffStageA, diffStageB) => {
    set({ diffStageA, diffStageB });
  },

  setSelectedAnalysisFunc: (selectedAnalysisFunc: string) => {
    set({ selectedAnalysisFunc });
  },

  selectExample: (id: string) => {
    const { examples } = get();
    const ex = examples.find(e => e.id === id);
    if (ex) {
      const extraFeatures = ex.id.includes('avx') ? ['+avx', '+avx2'] : [];
      set({
        selectedExampleId: id,
        code: ex.code,
        features: extraFeatures.length > 0 ? extraFeatures : get().features,
      });
      get().compile();
    }
  },

  jumpToDiagnostic: (diag: DiagnosticItem) => {
    if (diag.line) {
      set({
        jumpToLocation: { line: diag.line, col: diag.col || 1 },
      });
    }
  },

  clearJumpLocation: () => {
    set({ jumpToLocation: null });
  },

  inspectSymbol: async (name: string) => {
    const cleanName = name.replace(/^@/, '').trim();
    const { result } = get();

    // Check pre-matched symbols first
    let docEntity: DoxygenDocEntity | null = result?.matchedSymbols?.[cleanName] || result?.matchedSymbols?.[name] || null;

    if (!docEntity) {
      try {
        const res = await fetch(`/api/doxygen/symbol/${encodeURIComponent(cleanName)}`);
        if (res.ok) {
          docEntity = await res.json();
        }
      } catch {
        // Doc entity not found
      }
    }

    set({
      selectedSymbolName: cleanName,
      selectedDocEntity: docEntity,
      viewMode: 'symbols',
    });
  },

  clearSelectedSymbol: () => {
    set({ selectedSymbolName: null, selectedDocEntity: null });
  },

  setSymbolSearchQuery: (symbolSearchQuery: string) => {
    set({ symbolSearchQuery });
  },

  generateDocs: async () => {
    set({ isGeneratingDocs: true });
    try {
      const res = await fetch('/api/doxygen/generate', { method: 'POST' });
      if (res.ok) {
        const status: DoxygenStatus = await res.json();
        set({ doxygenStatus: status, isGeneratingDocs: false });
        // Re-compile to update symbol correlation
        get().compile();
      } else {
        set({ isGeneratingDocs: false });
      }
    } catch {
      set({ isGeneratingDocs: false });
    }
  },

  checkDoxygenStatus: async () => {
    try {
      const res = await fetch('/api/doxygen/status');
      if (res.ok) {
        const status: DoxygenStatus = await res.json();
        set({ doxygenStatus: status });
      }
    } catch {
      // Server not reachable
    }
  },

  compile: async () => {
    const { code, targetTriple, optLevel, features, isPositionIndependent, enableRtti } = get();
    set({ isCompiling: true, error: null });
    try {
      const isC = !code.includes('fn @') && !code.includes('function @') &&
        (/\b(int|void|char|short|long|float|double|struct|typedef|extern|#include|#define)\b/.test(code));

      const payload: CompileRequest = {
        code,
        language: isC ? 'c' : 'mir',
        filename: isC ? 'source.c' : 'source.mir',
        targetTriple,
        optLevel,
        features,
        isPositionIndependent,
        enableRtti,
      };

      const res = await fetch('/api/compile', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      });

      if (!res.ok) {
        throw new Error(`HTTP ${res.status}: ${res.statusText}`);
      }

      const data: CompileResponse = await res.json();
      const currentSelected = get().selectedAnalysisFunc;
      const availableFuncs = Object.keys(data.functions || {});
      const validFunc = (currentSelected && availableFuncs.includes(currentSelected))
        ? currentSelected
        : (availableFuncs[0] || null);

      const selSym = get().selectedSymbolName;
      const updatedDoc = selSym && data.matchedSymbols?.[selSym]
        ? data.matchedSymbols[selSym]
        : get().selectedDocEntity;

      const passes = data.pipeline?.passes || [];
      let passIdx = get().selectedPassIndex;
      if (passIdx >= passes.length && passes.length > 0) {
        passIdx = passes.length - 1;
      }

      set({
        result: data,
        isCompiling: false,
        selectedPassIndex: passIdx,
        selectedAnalysisFunc: validFunc,
        doxygenStatus: data.doxygenStatus || get().doxygenStatus,
        selectedDocEntity: updatedDoc,
      });
    } catch (err: any) {
      set({
        error: err.message || 'Compilation request failed',
        isCompiling: false,
      });
    }
  },

  loadInitialData: async () => {
    try {
      const [targetsRes, examplesRes] = await Promise.all([
        fetch('/api/targets'),
        fetch('/api/examples'),
      ]);

      if (targetsRes.ok) {
        const targetInfo: TargetInfo = await targetsRes.json();
        set({
          targetInfo,
          targetTriple: targetInfo.defaultTriple || 'x86_64-pc-windows-msvc',
        });
      }

      if (examplesRes.ok) {
        const examples: ExampleFile[] = await examplesRes.json();
        set({ examples });
      }

      // Initial compilation
      get().compile();
      get().checkDoxygenStatus();
    } catch (err: any) {
      console.warn('Initial data load warning:', err.message);
    }
  },
}));
