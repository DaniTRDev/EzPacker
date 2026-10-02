I want you to design and plan a rework of the x86_64 target. Before anything, create a new branch, (clone from master)
and checkout to it.

What to do:

- Improve the x86_Target by removing all the legacy code. Reorder files so they actually make sense.
- Look for legacy code and get rid of it.
- Further extend the legalization action table (.lad) and the legalize rules (.lrd) (look at EzLinker so
  you know what compiler-rt functions you're allowed to use).
- Ensure full x86_64 support, this includes regular ISA, SSE and AVX.

Follow:

- If you need information that does not require the exact code but just what it does, query graphifyy please. Try to use
  it unless you need the exact code to write more code.
- If you modify any code, ensure to update the internal documentation and the external (under /docs).
- Group changes related and make a commit after each step.
- After everything's done, push it onto the origin.
- Update graphifyy after each commit.