export interface CompileRequest {
  code: string;
  targetTriple?: string;
  optLevel?: 'O0' | 'O1' | 'O2' | 'Os';
  features?: string[];
  isPositionIndependent?: boolean;
  enableRtti?: boolean;
  diagLevel?: 'error' | 'warning' | 'trace' | 'debug';
  emitObject?: boolean;
}

export interface DiagnosticItem {
  type: 'error' | 'warning' | 'trace' | 'debug' | 'note';
  component: string;
  message: string;
  file?: string;
  line?: number;
  col?: number;
  sourceExcerpt?: string;
  caret?: string;
  notes?: string[];
}

export interface PassTiming {
  name: string;
  target?: string;
  durationMs: number;
}

export interface CfgBlock {
  id: string;
  blockNumber?: number;
  instructionCount?: number;
  instructions?: string[];
  successors: string[];
  predecessors: string[];
}

export interface FunctionAnalysis {
  name: string;
  blocks: Record<string, CfgBlock>;
  liveness: Record<string, {
    def: string[];
    use: string[];
    liveIn: string[];
    liveOut: string[];
  }>;
  frameLayout?: {
    calleeSaveSize?: number;
    totalSize?: number;
  };
}

export interface ObjectSectionInfo {
  name: string;
  size: number;
  offset: number;
  virtualAddress: number;
  characteristics: string;
  hexPreview?: string;
}

export interface ObjectSymbolInfo {
  name: string;
  section: string;
  value: number;
  size: number;
  binding: 'global' | 'local' | 'weak';
  type: 'function' | 'object' | 'notype';
}

export interface ObjectRelocInfo {
  section: string;
  offset: number;
  symbolName: string;
  type: string;
  addend?: number;
}

export interface ObjectAnalysis {
  format: 'COFF' | 'ELF64' | 'Unknown';
  fileSize: number;
  sections: ObjectSectionInfo[];
  symbols: ObjectSymbolInfo[];
  relocations: ObjectRelocInfo[];
  hexDump?: string;
}

export interface DoxygenParamInfo {
  name: string;
  type: string;
  description: string;
}

export interface DoxygenDocEntity {
  id: string;
  name: string;
  qualifiedName: string;
  kind: 'function' | 'struct' | 'class' | 'variable' | 'typedef' | 'enum';
  brief: string;
  detailedDescription: string;
  parameters: DoxygenParamInfo[];
  returnType?: string;
  returnDescription?: string;
  headerFile?: string;
  line?: number;
}

export interface DoxygenStatus {
  available: boolean;
  generated: boolean;
  doxygenPath?: string;
  version?: string;
  entityCount: number;
  lastGenerated?: string;
  status: 'idle' | 'generating' | 'ready' | 'not_installed' | 'invalid_configuration' | 'error';
  errorMessage?: string;
}

export interface CompileResponse {
  success: boolean;
  exitCode: number;
  asm?: string;
  mir?: string;
  legalizedMir?: string;
  loweredMir?: string;
  rawOutput?: string;
  rawStderr?: string;
  passes: PassTiming[];
  diagnostics: DiagnosticItem[];
  functions: Record<string, FunctionAnalysis>;
  objectInfo?: ObjectAnalysis;
  doxygenStatus?: DoxygenStatus;
  matchedSymbols?: Record<string, DoxygenDocEntity>;
  executionDurationMs: number;
}

export interface ExampleFile {
  id: string;
  filename: string;
  title: string;
  description: string;
  code: string;
}

export interface TargetInfo {
  defaultTriple: string;
  availableTriples: Array<{ id: string; label: string; os: string; arch: string }>;
  features: Array<{ id: string; label: string; description: string; default?: boolean }>;
}
