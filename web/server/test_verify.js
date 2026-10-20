import { CompilerRunner } from './dist/compiler.js';
import { loadExamples } from './dist/examples.js';

const ANSI_REGEX = /[\u001b\x1b]\[[0-9;]*[a-zA-Z]|[\u001b\x1b]\([a-zA-Z]|[\u001b\x1b][=>]/;

function assertNoAnsi(str, name) {
  if (ANSI_REGEX.test(str)) {
    throw new Error(`Output '${name}' contains ANSI console colors/escape sequences! Found:\n${str.slice(0, 200)}`);
  }
}

async function runTests() {
  console.log('=== Running Compiler Bridge Unit Tests ===');
  const runner = new CompilerRunner();
  console.log('Compiler binary detected:', runner.getCompilerPath());

  // 1. Test loadExamples
  const examples = loadExamples();
  console.log(`[PASS] Loaded ${examples.length} bundled example .mir files.`);
  if (examples.length === 0) {
    throw new Error('Expected at least 1 example file');
  }

  // 2. Test compiling arithmetic_32bit.mir
  const ex = examples.find(e => e.id === 'arithmetic_32bit') || examples[0];
  console.log(`Compiling example: ${ex.filename}...`);
  const res = await runner.compile({
    code: ex.code,
    targetTriple: 'x86_64-pc-windows-msvc',
    optLevel: 'O2',
    features: ['+avx2'],
  });

  if (!res.success) {
    console.error('Compilation failed unexpectedly:', res.diagnostics);
    throw new Error('Compilation should succeed on bundled example');
  }

  // Assert ANSI-free outputs
  assertNoAnsi(res.asm || '', 'asm');
  assertNoAnsi(res.mir || '', 'mir');
  assertNoAnsi(res.legalizedMir || '', 'legalizedMir');
  assertNoAnsi(res.loweredMir || '', 'loweredMir');
  assertNoAnsi(res.rawOutput || '', 'rawOutput');
  assertNoAnsi(res.rawStderr || '', 'rawStderr');

  // Verify that MIR starts directly with function/declare and does not contain trace logs
  if (!res.mir.startsWith('function @') && !res.mir.startsWith('declare @')) {
    throw new Error(`Generic MIR does not start with function definition! Found:\n${res.mir.slice(0, 150)}`);
  }
  if (!res.legalizedMir.startsWith('function @') && !res.legalizedMir.startsWith('declare @')) {
    throw new Error(`Legalized MIR does not start with function definition! Found:\n${res.legalizedMir.slice(0, 150)}`);
  }
  if (!res.loweredMir.startsWith('function @') && !res.loweredMir.startsWith('declare @')) {
    throw new Error(`Lowered MIR does not start with function definition! Found:\n${res.loweredMir.slice(0, 150)}`);
  }
  if (!res.asm.startsWith('.globl') && !res.asm.startsWith('.extern') && !res.asm.startsWith('.')) {
    throw new Error(`Assembly does not start with directive! Found:\n${res.asm.slice(0, 150)}`);
  }

  console.log(`[PASS] Verified 0 ANSI color escape sequences in all IR and Assembly outputs.`);
  console.log(`[PASS] Verified clean MIR and Assembly formatting without trace logs.`);
  console.log(`[PASS] Compilation succeeded in ${res.executionDurationMs}ms.`);
  console.log(`[PASS] Assembly generated: ${res.asm?.length} chars.`);
  console.log(`[PASS] Passes tracked: ${res.passes.length} passes.`);

  // 3. Test multi-function CFG, Liveness, and Stack on branch_control_flow.mir
  const branchEx = examples.find(e => e.id === 'branch_control_flow');
  if (branchEx) {
    console.log(`\nCompiling multi-function test: ${branchEx.filename}...`);
    const branchRes = await runner.compile({
      code: branchEx.code,
      targetTriple: 'x86_64-pc-windows-msvc',
      optLevel: 'O0',
    });

    if (!branchRes.success) {
      throw new Error(`Failed to compile ${branchEx.filename}`);
    }

    const funcs = Object.keys(branchRes.functions);
    console.log(`[PASS] Detected functions: ${funcs.join(', ')}`);
    if (funcs.length < 3) {
      throw new Error(`Expected at least 3 functions in ${branchEx.filename}, got ${funcs.length}`);
    }

    const FORBIDDEN = new Set(['note', 'Params', 'Blocks', 'Stack Frame', 'empty']);

    for (const fName of funcs) {
      const fInfo = branchRes.functions[fName];
      const blockNames = Object.keys(fInfo.blocks);
      const blockCount = blockNames.length;
      const livenessCount = Object.keys(fInfo.liveness).length;
      console.log(`   Function @${fName}: ${blockCount} CFG blocks, ${livenessCount} liveness blocks, frame:`, fInfo.frameLayout);

      if (blockCount === 0) {
        throw new Error(`Function @${fName} has 0 CFG blocks! Multi-function CFG mapping failed.`);
      }
      if (livenessCount === 0) {
        throw new Error(`Function @${fName} has 0 liveness entries! Multi-function liveness mapping failed.`);
      }
      if (!fInfo.frameLayout) {
        throw new Error(`Function @${fName} is missing stack frameLayout analysis!`);
      }

      // Check no fake blocks
      for (const bName of blockNames) {
        if (FORBIDDEN.has(bName)) {
          throw new Error(`Function @${fName} contains forbidden fake block %${bName}!`);
        }
      }

      // Check instructions in blocks
      for (const [bName, b] of Object.entries(fInfo.blocks)) {
        if (!b.instructions || b.instructions.length === 0) {
          throw new Error(`Function @${fName} block %${bName} has no parsed instructions!`);
        }
      }

      // Check real registers in liveness sets
      let totalLiveRegs = 0;
      for (const [bName, lData] of Object.entries(fInfo.liveness)) {
        totalLiveRegs += lData.def.length + lData.use.length + lData.liveIn.length + lData.liveOut.length;
      }
      if (totalLiveRegs === 0) {
        throw new Error(`Function @${fName} has 0 total registers across all liveness sets!`);
      }
    }
    console.log('[PASS] Multi-function CFG, Liveness, Instructions, and Stack layout verified successfully!');
  }

  // 4. Test non-entry multi-function test: multiple_returns.mir
  const multiEx = examples.find(e => e.id === 'multiple_returns');
  if (multiEx) {
    console.log(`\nCompiling non-entry multi-function test: ${multiEx.filename}...`);
    const multiRes = await runner.compile({
      code: multiEx.code,
      targetTriple: 'x86_64-pc-windows-msvc',
      optLevel: 'O0',
    });

    if (!multiRes.success) {
      throw new Error(`Failed to compile ${multiEx.filename}`);
    }

    const funcs = Object.keys(multiRes.functions);
    console.log(`[PASS] Detected functions in multiple_returns: ${funcs.join(', ')}`);
    if (!funcs.includes('calculate_dual_path') || !funcs.includes('multi_exit_32bit')) {
      throw new Error(`Expected calculate_dual_path and multi_exit_32bit`);
    }

    const fn1 = multiRes.functions['calculate_dual_path'];
    const fn2 = multiRes.functions['multi_exit_32bit'];

    if (!fn1.blocks['path_alpha'] || !fn1.blocks['path_beta']) {
      throw new Error(`calculate_dual_path missing path_alpha or path_beta blocks!`);
    }
    if (!fn2.blocks['route_add'] || !fn2.blocks['route_sub'] || !fn2.blocks['route_mul']) {
      throw new Error(`multi_exit_32bit missing route blocks!`);
    }
    console.log('[PASS] Successfully verified non-entry block names mapped to respective functions!');
  }

  // 5. Test Error reporting on invalid MIR
  console.log('\nTesting invalid syntax handling...');
  const badRes = await runner.compile({
    code: 'fn @broken() { entry: %x = ADD i32; }',
    targetTriple: 'x86_64-pc-windows-msvc',
    optLevel: 'O0',
  });

  if (badRes.success) {
    throw new Error('Expected compilation to fail for syntax error');
  }

  for (const d of badRes.diagnostics) {
    assertNoAnsi(d.message, 'diagnostic.message');
    if (d.sourceExcerpt) assertNoAnsi(d.sourceExcerpt, 'diagnostic.sourceExcerpt');
  }

  console.log(`[PASS] Diagnostics are completely clean of ANSI sequences.`);
  const err = badRes.diagnostics.find(d => d.type === 'error');
  if (!err) {
    throw new Error('Expected an error diagnostic');
  }
  console.log(`   Error message: [${err.component}] ${err.message}`);
  console.log(`   Location: line ${err.line}:${err.col}`);

  // 6. Test Object Analysis on arithmetic_32bit
  console.log('\nTesting Object file inspection...');
  if (!res.objectInfo) {
    throw new Error('Expected objectInfo to be generated during compilation!');
  }
  console.log(`[PASS] Object format: ${res.objectInfo.format}, size: ${res.objectInfo.fileSize} bytes.`);
  console.log(`[PASS] Sections parsed: ${res.objectInfo.sections.map(s => s.name).join(', ')}`);
  console.log(`[PASS] Symbols parsed: ${res.objectInfo.symbols.map(s => s.name).join(', ')}`);
  if (!res.objectInfo.sections.some(s => s.name === '.text')) {
    throw new Error('Expected .text section in objectInfo');
  }
  if (!res.objectInfo.symbols.some(s => s.name === 'main')) {
    throw new Error('Expected main function in objectInfo.symbols');
  }
  if (!res.objectInfo.hexDump || res.objectInfo.hexDump.length === 0) {
    throw new Error('Expected formatted hex dump in objectInfo');
  }

  // 6b. Test RISC-V compilation & ELF64 object inspection
  console.log('\nTesting RISC-V 64 compilation and ELF64 Object file inspection...');
  const riscvRes = await runner.compile({
    code: ex.code,
    targetTriple: 'riscv64-unknown-linux-gnu',
    optLevel: 'O2',
  });
  if (!riscvRes.success) {
    console.error('RISC-V compilation failed unexpectedly:', riscvRes.diagnostics);
    throw new Error('RISC-V compilation should succeed on bundled example');
  }
  assertNoAnsi(riscvRes.asm || '', 'riscv_asm');
  if (!riscvRes.objectInfo) {
    throw new Error('Expected objectInfo for RISC-V compilation');
  }
  if (riscvRes.objectInfo.format !== 'ELF64') {
    throw new Error(`Expected ELF64 format for RISC-V object, got: ${riscvRes.objectInfo.format}`);
  }
  console.log(`[PASS] RISC-V Object format: ${riscvRes.objectInfo.format}, size: ${riscvRes.objectInfo.fileSize} bytes.`);
  console.log(`[PASS] RISC-V Sections: ${riscvRes.objectInfo.sections.map(s => s.name).join(', ')}`);
  console.log(`[PASS] RISC-V Symbols: ${riscvRes.objectInfo.symbols.map(s => s.name).join(', ')}`);
  if (!riscvRes.objectInfo.sections.some(s => s.name === '.text')) {
    throw new Error('Expected .text section in RISC-V objectInfo');
  }
  if (!riscvRes.objectInfo.symbols.some(s => s.name === 'main')) {
    throw new Error('Expected main in RISC-V symbols');
  }

  // 7. Test DoxygenRunner & XML parsing
  console.log('\nTesting DoxygenRunner documentation extraction...');
  const { DoxygenRunner } = await import('./dist/doxygen.js');
  const dox = new DoxygenRunner();
  const initStatus = dox.getStatus();
  console.log(`[PASS] Doxygen status detected: available=${initStatus.available}, path=${initStatus.doxygenPath}`);

  if (initStatus.available) {
    console.log('Generating documentation with Doxygen...');
    const genStatus = await dox.generateDocumentation(true);
    console.log(`[PASS] Doxygen generation succeeded: ${genStatus.entityCount} entities indexed.`);

    if (genStatus.entityCount === 0) {
      throw new Error('Expected at least one indexed Doxygen entity');
    }

    // Verify __ez_throw documentation from EzExceptionRuntime.h
    const ezThrow = dox.getSymbol('__ez_throw');
    if (!ezThrow) {
      throw new Error('Failed to find __ez_throw in Doxygen symbol index!');
    }
    console.log(`[PASS] Retrieved __ez_throw doc entity:`, {
      name: ezThrow.name,
      qualifiedName: ezThrow.qualifiedName,
      params: ezThrow.parameters.map(p => `${p.type} ${p.name}`),
      brief: ezThrow.brief || ezThrow.detailedDescription.slice(0, 50),
    });

    if (!ezThrow.parameters.some(p => p.name === 'payload')) {
      throw new Error('Expected "payload" parameter in __ez_throw docs');
    }

    // Verify struct EzExceptionFrame
    const frameStruct = dox.getSymbol('EzExceptionFrame');
    if (!frameStruct) {
      throw new Error('Failed to find EzExceptionFrame in Doxygen symbol index!');
    }
    console.log(`[PASS] Retrieved EzExceptionFrame struct doc entity.`);

    // Verify search
    const searchResults = dox.searchSymbols('catch', 5);
    console.log(`[PASS] Search 'catch' returned ${searchResults.length} results: ${searchResults.map(s => s.name).join(', ')}`);

    // 8. Test end-to-end symbol correlation with exception_handling.mir
    const exHandling = examples.find(e => e.id === 'exception_handling');
    if (exHandling) {
      console.log('\nCompiling exception_handling.mir with Doxygen symbol correlation...');
      const runnerWithDox = new CompilerRunner(dox);
      const exRes = await runnerWithDox.compile({
        code: exHandling.code,
        targetTriple: 'x86_64-pc-windows-msvc',
        optLevel: 'O0',
      });

      if (!exRes.success) {
        throw new Error('Compilation failed on exception_handling.mir');
      }

      console.log(`[PASS] Compiled exception_handling.mir successfully.`);
      console.log(`[PASS] Matched symbols:`, Object.keys(exRes.matchedSymbols || {}));
      if (!exRes.matchedSymbols || !exRes.matchedSymbols['__ez_throw']) {
        throw new Error('Expected __ez_throw to be correlated in matchedSymbols!');
      }
      console.log(`[PASS] Verified automatic Doxygen correlation for @__ez_throw call site.`);
    }
  }

  // 9. Test Error Handling & Resilience when Doxygen is missing
  console.log('\nTesting Doxygen error handling resilience...');
  process.env.DOXYGEN_BIN = 'C:\\non_existent_path\\doxygen_fake.exe';
  const missingDox = new DoxygenRunner();
  const missingStatus = missingDox.getStatus();
  if (missingStatus.available) {
    throw new Error('Expected missing Doxygen to report available: false');
  }
  const failedGen = await missingDox.generateDocumentation(true);
  if (failedGen.status !== 'not_installed') {
    throw new Error(`Expected not_installed status, got: ${failedGen.status}`);
  }
  console.log(`[PASS] Verified graceful non-crashing status when Doxygen is not installed: ${failedGen.errorMessage}`);
  // 10. Test Direct C11 Compilation via chibicc frontend
  console.log('\nTesting direct C11 compilation via embedded chibicc frontend...');
  const cExample = examples.find(e => e.filename === 'c_yarpgen_kernel.c') || examples.find(e => e.filename.endsWith('.c'));
  if (cExample) {
    console.log(`Compiling C example: ${cExample.filename}...`);
    const cRes = await runner.compile({
      code: cExample.code,
      filename: cExample.filename,
      language: 'c',
      targetTriple: 'x86_64-pc-windows-msvc',
      optLevel: 'O2',
    });

    if (!cRes.success) {
      console.error('C compilation failed:', cRes.diagnostics);
      throw new Error(`Expected C11 compilation to succeed for ${cExample.filename}`);
    }

    assertNoAnsi(cRes.asm || '', 'c_asm');
    assertNoAnsi(cRes.mir || '', 'c_mir');
    const funcNames = Object.keys(cRes.functions || {});
    if (funcNames.length === 0) {
      throw new Error('Expected at least one function to be parsed in function analysis!');
    }
    const testFunc = funcNames.includes('kernel_compute') ? 'kernel_compute' : funcNames[0];

    console.log(`[PASS] Compiled C11 source ${cExample.filename} successfully in ${cRes.executionDurationMs}ms.`);
    console.log(`[PASS] Generated clean MIR: ${cRes.mir?.length} chars, Assembly: ${cRes.asm?.length} chars.`);
    console.log(`[PASS] Detected C function CFG blocks:`, Object.keys(cRes.functions[testFunc].blocks || {}));
  } else {
    throw new Error('Expected at least one C example file to be discovered');
  }

  console.log('\n=== All Server Bridge Tests PASSED Successfully! ===');
}

runTests().catch(err => {
  console.error('Test failed:', err);
  process.exit(1);
});
