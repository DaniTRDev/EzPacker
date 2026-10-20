export interface CompileRequest {
  code: string;
  language?: 'mir' | 'c';
  filename?: string;
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

// ---------------------------------------------------------------------------
// Native Pipeline Telemetry Types (emitted by ezc --dump-pipeline-json)
// ---------------------------------------------------------------------------

export type CompilerPassStage = 'Verification' | 'MiddleEnd' | 'Legalization' | 'TargetLowering';

export interface PassTelemetryRecord {
  index: number;
  name: string;
  stage: CompilerPassStage;
  function?: string;
  type: 'FunctionPass' | 'ModulePass';
  durationMs: number;
  modifiedMir: boolean;
  instructionCount: number;
  blockCount: number;
  mirDump: string;
}

export interface CfgBlockTelemetry {
  id: number | string;
  name: string;
  instructionCount?: number;
  predecessors: string[];
  successors: string[];
  instructions: string[];
}

export interface LivenessInfo {
  def: string[];
  use: string[];
  liveIn: string[];
  liveOut: string[];
}

export interface StackSlotTelemetry {
  name: string;
  offset: number;
  size: number;
}

export interface FrameLayoutTelemetry {
  totalSize?: number;
  totalFrameSize?: number;
  calleeSaveSize?: number;
  calleeSavedSize?: number;
  stackSlots?: StackSlotTelemetry[];
}

export interface FunctionTelemetry {
  name: string;
  blocks: CfgBlockTelemetry[];
  liveness: Record<string, LivenessInfo>;
  frameLayout?: FrameLayoutTelemetry;
}

export interface PipelineDiagnostic {
  severity: 'error' | 'warning' | 'trace' | 'debug';
  component: string;
  message: string;
  file: string;
  line: number;
  col: number;
}

export interface PipelineTelemetryJson {
  target: string;
  optLevel: string;
  inputFile: string;
  initialMir: string;
  assembly: string;
  passes: PassTelemetryRecord[];
  diagnostics: PipelineDiagnostic[];
  functions: Record<string, FunctionTelemetry>;
}

// ---------------------------------------------------------------------------
// Top-Level Compile Response
// ---------------------------------------------------------------------------

export interface CompileResponse {
  success: boolean;
  exitCode: number;
  pipeline?: PipelineTelemetryJson;
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
