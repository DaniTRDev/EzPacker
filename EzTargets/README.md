# EzTargets

`EzTargets` is the home of the architecture-specific compiler targets. Every target is a
self-contained subproject that bundles:

- the runtime target implementation (target descriptor, lowering, frame lowering, register
  allocation, instruction selection, binary descriptors, relocation resolver);
- the machine-code emitter/encoder for the ISA;
- the declarative DSL inputs (`*.ezcc`, `*.lad`, `*.lrd`, `*.idf`, `*.isf`) that drive code
  generation;
- the build-time DSL plugin (encoding dialect + encoding codegen backend) used to compile
  those inputs with `EzDslCli`;
- the target factory registration that plugs the target into `EzCompiler::TargetResolver`.

Generic, target-agnostic infrastructure remains in the core projects (`EzMir`, `EzTriple`,
`EzCodeEmitter`, `EzDsl`, `EzCompiler`). This project only depends on them; it does not
require the compiler to know about any particular ISA.

## Targets

| Target | Folder | Runtime library | DSL plugin library |
| --- | --- | --- | --- |
| x86-64 | `X86_64/` | `EzTargetsX86_64` | `EzTargetsX86_64Dsl` |

## CMake helpers

`CMake/` holds the `EzDslGen*` functions that bind a DSL input file to a target library by
running `EzDslCli`. They are reused by every target and take a `NAMESPACE_ROOT` so generated
tables land in the owning target's C++ namespace.

## Adding a target

1. Create `<Target>/` with `include/`, `src/`, `targets/`, and optional `Dsl/`.
2. Define the runtime library and run the `EzDslGen*` helpers in `<Target>/CMakeLists.txt`.
3. Register the target factory and (if needed) link the DSL plugin into `EzDslCli`.
