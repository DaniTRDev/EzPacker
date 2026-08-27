frame_layout StandardFrame {
    // Dynamic stack-growth arithmetic: adjust SP for the calculated frame size
    PROLOGUE {
        // Step 1: Save FP if frame pointer elimination is disabled / DALLOC is present
        SAVE_FRAME_POINTER {
            emit { PUSH(GPR:rbp); };
            emit { MOV(GPR:rbp, GPR:rsp); };
        };

        // Step 2: Push used callee-saved registers (order resolved automatically)
        SAVE_CALLEE_REGS {
            PUSH($reg);
        };

        // Step 3: Allocate static frame body
        ALLOC_FRAME($frameSize) {
            when { $frameSize > 0; };
            emit {
                // Generates immediate subtraction or a loop/probe for large frames (e.g., __chkstk)
                SUB(GPR:rsp, GPR:rsp, imm(i64):$frameSize);
            };
        };
    };

    EPILOGUE {
        // Step 1: Deallocate frame body
        DEALLOC_FRAME {
            when { hasDynamicAllocations() || hasVariableSizedObjects(); };
            emit {
                // If DALLOC was lowered, collapse SP back to FP
                MOV(GPR:rsp, GPR:rbp);
            };
            otherwise {
                when { $frameSize > 0; };
                emit {
                    ADD(GPR:rsp, GPR:rsp, imm(i64):$frameSize);
                };
            };
        };

        // Step 2: Restore callee-saved registers (in reverse order of prologue)
        RESTORE_CALLEE_REGS {
            POP($reg);
        };

        // Step 3: Restore old FP
        RESTORE_FRAME_POINTER {
            emit { POP(GPR:rbp); };
        };

        // Step 4: Emit Target Return
        RETURN {
            emit { RET(); };
        };
    };
};