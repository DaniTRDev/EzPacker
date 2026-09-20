# EzTargets hosts architecture-specific targets as self-contained projects.
#
# Each target subdirectory owns its runtime target description, its DSL inputs, the
# generated tables and any build-time plugin (encoding dialect / codegen backend) that
# those inputs require. Generic infrastructure (EzMir, EzTriple, EzCodeEmitter, EzDsl)
# stays target-agnostic; only the target project references concrete ISAs.
#
# Add target subdirectories here as they are introduced.
add_subdirectory(X86_64)
