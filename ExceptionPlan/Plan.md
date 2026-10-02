I want you to create a full implementation design for a new feature in the compiler.

This feature is exception support using C's std exeception library:

- I want you to define new IR instructions (try, catch).
- I want you to think on different RTTI implementations so that I can support rich exceptions wuth source references.
  The RTTI implementation must allow be deactivated from CLI options.
- Add a new target called EzLinker which internally just calls system's preferred linker (lld, ...) with the given
  argument list. I want this option so I can automatically link against C std for
  exception and compiler legalizer helpers.

After each phase, 2 things will be done:

- Commit the changes to the new branch.
- Update the affected's code/project documentation.

Before:
Create a new branch (as a clone of fixups) and push all the work here. After everything's good to go make a pull request
to incorporate the changes into main.

Allowed:
You can use graphiffyy if you please, just remember to update it before using it.